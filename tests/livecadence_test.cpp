#include "helpers/settings.h"
#include "livecadence.h"

#include <QCoreApplication>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

qint16 tagAt(qint64 index)
{
    const qint64 segment = index / LiveCapture::kMaxSegmentSamples;
    return static_cast<qint16>(1000 * (segment + 1));
}

bool containsValue(const std::vector<float>& samples, float value)
{
    return std::find(samples.begin(), samples.end(), value) != samples.end();
}

struct Feed {
    std::vector<qint64> publishedAt;
    std::vector<qint64> closedAt;
    std::vector<std::size_t> closedLengths;
    std::vector<int> speechSeconds;
    std::vector<qint64> speechAtMs;
    qint64 checkpointMs = 0;
    LiveCapture::Reading checkpoint;
    bool sawCheckpoint = false;
};

void note(Feed& feed, const LiveCapture::Update& update, qint64 nowMs)
{
    if (update.hasReading) {
        feed.publishedAt.push_back(update.reading.atMs);
        if (nowMs == feed.checkpointMs) {
            feed.checkpoint = update.reading;
            feed.sawCheckpoint = true;
        }
    }
    for (const LiveCapture::ClosedSegment& closed : update.closed) {
        feed.closedAt.push_back(nowMs);
        feed.closedLengths.push_back(closed.samples.size());
    }
}

void noteSpeech(Feed& feed, const LiveCapture& take, qint64 nowMs)
{
    feed.speechSeconds.push_back(take.speechClockSeconds());
    feed.speechAtMs.push_back(nowMs);
}

// stepMs must divide the first-reading mark, the cadence, and the 15 s cut.
Feed feedContinuous(LiveCapture& take, int durationMs, int stepMs, qint64 checkpointMs)
{
    Feed feed;
    feed.checkpointMs = checkpointMs;
    const int count = LiveCapture::kSampleRate * stepMs / 1000;
    qint64 cursor = 0;
    for (qint64 now = stepMs; now <= durationMs; now += stepMs) {
        std::vector<qint16> chunk(static_cast<std::size_t>(count));
        for (int index = 0; index < count; ++index)
            chunk[static_cast<std::size_t>(index)] = tagAt(cursor + index);
        take.append(chunk.data(), count);
        cursor += count;
        noteSpeech(feed, take, now);
        note(feed, take.advanceContinuous(now), now);
        take.trim();
    }
    return feed;
}

Feed feedDetected(LiveCapture& take, int durationMs, int stepMs, qint64 checkpointMs)
{
    Feed feed;
    feed.checkpointMs = checkpointMs;
    const int count = LiveCapture::kSampleRate * stepMs / 1000;
    qint64 cursor = 0;
    for (qint64 now = stepMs; now <= durationMs; now += stepMs) {
        std::vector<qint16> chunk(static_cast<std::size_t>(count));
        for (int index = 0; index < count; ++index)
            chunk[static_cast<std::size_t>(index)] = tagAt(cursor + index);
        const qint64 frameStart = take.fed();
        take.append(chunk.data(), count);
        const qint64 frameEnd = take.fed();
        cursor += count;
        noteSpeech(feed, take, now);
        note(feed, take.advanceSpeech(frameStart, frameEnd, now), now);
        take.trim();
    }
    return feed;
}

std::string listOf(const std::vector<qint64>& values)
{
    std::string text;
    for (qint64 value : values) {
        if (!text.empty())
            text += ", ";
        text += std::to_string(value);
    }
    return text;
}

// Speech that starts at sample 0 still counts the 300 ms leading pad toward
// the 1.5 s gate, even though that pad is clamped off. The first snapshot is
// therefore at 1.2 s, and every later one is one cadence interval after it.
qint64 firstReadingMs()
{
    const qint64 samples = LiveCapture::kMinWindowSamples - LiveCapture::kPadSamples;
    return samples * 1000 / LiveCapture::kSampleRate;
}

std::vector<qint64> expectedPublishes(int updatesPerMinute, int durationMs)
{
    const int interval = liveUpdateIntervalMs(updatesPerMinute);
    std::vector<qint64> times;
    for (qint64 at = firstReadingMs(); at <= durationMs; at += interval)
        times.push_back(at);
    return times;
}

void expectSpeechTicks(const Feed& feed, const char* label)
{
    expect(feed.speechSeconds.size() == feed.speechAtMs.size(),
        std::string(label) + " records a speech-time sample on every step");
    for (std::size_t index = 0; index < feed.speechAtMs.size(); ++index) {
        const qint64 elapsed = feed.speechAtMs[index];
        if (elapsed % 1000 != 0)
            continue;
        const int expected = static_cast<int>(elapsed / 1000);
        expect(feed.speechSeconds[index] == expected,
            std::string(label) + " speech time is " + std::to_string(expected)
                + " s at " + std::to_string(elapsed) + " ms, got "
                + std::to_string(feed.speechSeconds[index]));
    }
}

void expectFeed(const Feed& feed, int updatesPerMinute, int durationMs, int windowSec,
    const std::vector<float>& tags, const char* label)
{
    const std::vector<qint64> expected = expectedPublishes(updatesPerMinute, durationMs);
    expect(feed.publishedAt == expected,
        std::string(label) + " publishes at " + listOf(expected)
            + " ms, got " + listOf(feed.publishedAt));

    const std::vector<qint64> cuts { 15000, 30000, 45000, 60000 };
    expect(feed.closedAt == cuts,
        std::string(label) + " stores a segment at 15 s boundaries, got " + listOf(feed.closedAt));
    expect(feed.closedLengths.size() == cuts.size(), std::string(label) + " closed-segment count");
    for (std::size_t length : feed.closedLengths) {
        expect(length == static_cast<std::size_t>(LiveCapture::kMaxSegmentSamples),
            std::string(label) + " keeps each cut at 15 s of audio");
    }

    const int interval = liveUpdateIntervalMs(updatesPerMinute);
    for (qint64 cut : cuts) {
        expect(std::find(feed.publishedAt.begin(), feed.publishedAt.end(), cut) == feed.publishedAt.end(),
            std::string(label) + " does not publish on the " + std::to_string(cut) + " ms cut");
        const std::size_t index = static_cast<std::size_t>(cut / 100) - 1;
        if (index < feed.speechSeconds.size()) {
            expect(feed.speechSeconds[index] == static_cast<int>(cut / 1000),
                std::string(label) + " speech time still ticks at the "
                    + std::to_string(cut) + " ms cut");
        }
    }
    if (feed.publishedAt.size() >= 2) {
        expect(feed.publishedAt[1] - feed.publishedAt[0] == interval,
            std::string(label) + " waits the full cadence across the first cut");
    }

    expectSpeechTicks(feed, label);

    expect(feed.sawCheckpoint,
        std::string(label) + " has the checkpoint reading at " + std::to_string(feed.checkpointMs) + " ms");
    if (feed.sawCheckpoint) {
        const std::size_t windowSamples = static_cast<std::size_t>(windowSec) * LiveCapture::kSampleRate;
        expect(feed.checkpoint.samples.size() == windowSamples,
            std::string(label) + " reading covers the " + std::to_string(windowSec) + " s analysis window");
        for (float tag : tags) {
            expect(containsValue(feed.checkpoint.samples, tag),
                std::string(label) + " window still holds audio tagged " + std::to_string(static_cast<int>(tag)));
        }
        expect(feed.checkpoint.openSeconds > 1.19 && feed.checkpoint.openSeconds < 1.21,
            std::string(label) + " open tail is the audio since the last cut");
    }
}

void testCadenceAcrossCuts(int updatesPerMinute, int windowSec, qint64 checkpointMs,
    const std::vector<float>& tags)
{
    constexpr int kDurationMs = 65000;
    constexpr int kStepMs = 100;

    LiveCapture continuous;
    continuous.start(updatesPerMinute, windowSec);
    const Feed fromContinuous = feedContinuous(continuous, kDurationMs, kStepMs, checkpointMs);
    expectFeed(fromContinuous, updatesPerMinute, kDurationMs, windowSec, tags, "continuous");

    const int publishesBeforeStop = static_cast<int>(fromContinuous.publishedAt.size());
    const LiveCapture::Update stopped = continuous.closeForStop();
    expect(!stopped.hasReading, "Stop does not publish a live reading");
    expect(stopped.closed.size() == 1, "Stop keeps the open tail");
    if (stopped.closed.size() == 1) {
        expect(stopped.closed.front().samples.size() == static_cast<std::size_t>(5 * LiveCapture::kSampleRate),
            "Stop keeps the 5 s left after the 60 s cut");
    }
    expect(static_cast<int>(fromContinuous.publishedAt.size()) == publishesBeforeStop,
        "Stop does not add a publish time");

    LiveCapture detected;
    detected.start(updatesPerMinute, windowSec);
    const Feed fromDetected = feedDetected(detected, kDurationMs, kStepMs, checkpointMs);
    expectFeed(fromDetected, updatesPerMinute, kDurationMs, windowSec, tags, "detected");
}

void testSegmentEndDoesNotPublish()
{
    LiveCapture take;
    take.start(2, 30);
    Feed feed = feedContinuous(take, 20000, 100, firstReadingMs() + 30000);
    expect(feed.publishedAt.size() == 1 && feed.publishedAt.front() == firstReadingMs(),
        "20 s of speech with 2 updates/min publishes only the first reading");
    expect(feed.closedAt.size() == 1 && feed.closedAt.front() == 15000,
        "the 15 s cut is stored during that 20 s");

    qint64 now = 20000;
    for (int frame = 0; frame < 250; ++frame) {
        std::vector<qint16> hop(static_cast<std::size_t>(LiveCapture::kHop), 0);
        take.append(hop.data(), static_cast<int>(hop.size()));
        now += 8;
        const LiveCapture::Update update = take.advanceSilence(2000);
        note(feed, update, now);
        take.trim();
    }
    expect(now == 22000, "two seconds of silence advance the clock");
    expect(!take.phraseSpeaking(), "a 2 s pause closes the segment");
    expect(feed.publishedAt.size() == 1, "closing the segment does not publish");
    expect(feed.closedAt.size() == 2 && feed.closedAt.back() == 22000,
        "the pause stores the open tail without a reading");

    while (now < 32000) {
        now += 100;
        const int count = LiveCapture::kSampleRate / 10;
        std::vector<qint16> chunk(static_cast<std::size_t>(count));
        const qint64 frameStart = take.fed();
        for (int index = 0; index < count; ++index)
            chunk[static_cast<std::size_t>(index)] = tagAt(frameStart + index);
        take.append(chunk.data(), count);
        note(feed, take.advanceSpeech(frameStart, take.fed(), now), now);
        take.trim();
    }
    expect(feed.publishedAt.size() == 2 && feed.publishedAt.back() == firstReadingMs() + 30000,
        "speech after a pause waits out the 30 s cadence, got " + listOf(feed.publishedAt));
    expect(feed.sawCheckpoint, "the post-pause reading is the snapshot one cadence later");
    if (feed.sawCheckpoint) {
        expect(containsValue(feed.checkpoint.samples, 1000.f),
            "the post-pause window still contains the earlier segment");
    }
}

void testShortPhraseIsNotAReading()
{
    LiveCapture take;
    take.start(60, 30);
    qint64 now = 0;
    for (int step = 0; step < 5; ++step) {
        now += 100;
        std::vector<qint16> chunk(static_cast<std::size_t>(LiveCapture::kSampleRate / 10), 7);
        const qint64 frameStart = take.fed();
        take.append(chunk.data(), static_cast<int>(chunk.size()));
        const LiveCapture::Update update = take.advanceSpeech(frameStart, take.fed(), now);
        expect(!update.hasReading, "a phrase under 1.5 s does not publish");
        expect(update.closed.empty(), "a phrase under 1 s is not stored on the way in");
        take.trim();
    }
    for (int frame = 0; frame < 250; ++frame) {
        std::vector<qint16> hop(static_cast<std::size_t>(LiveCapture::kHop), 0);
        take.append(hop.data(), static_cast<int>(hop.size()));
        now += 8;
        const LiveCapture::Update update = take.advanceSilence(2000);
        expect(!update.hasReading, "dropping a short phrase does not publish");
        expect(update.closed.empty(), "a short phrase is not stored");
        take.trim();
    }
    expect(take.dropped(), "a short phrase is marked dropped");

    bool published = false;
    LiveCapture::Reading reading;
    for (int step = 0; step < 20; ++step) {
        now += 100;
        std::vector<qint16> chunk(static_cast<std::size_t>(LiveCapture::kSampleRate / 10), 9);
        const qint64 frameStart = take.fed();
        take.append(chunk.data(), static_cast<int>(chunk.size()));
        const LiveCapture::Update update = take.advanceSpeech(frameStart, take.fed(), now);
        if (update.hasReading) {
            published = true;
            reading = update.reading;
            break;
        }
        take.trim();
    }
    expect(published, "later speech still publishes once the window is ready");
    if (published) {
        expect(!containsValue(reading.samples, 7.f),
            "a dropped phrase is left out of the analysis window");
        expect(containsValue(reading.samples, 9.f),
            "the next phrase is what the window measures");
    }
}

void testFirstBuffer()
{
    const int count = 2 * LiveCapture::kSampleRate;
    std::vector<qint16> chunk(static_cast<std::size_t>(count), 1);

    LiveCapture continuous;
    continuous.start(2, 30);
    continuous.append(chunk.data(), count);
    const LiveCapture::Update whole = continuous.advanceContinuous(2000);
    expect(whole.hasReading, "continuous mode publishes once the first buffer is long enough");
    expect(whole.closed.empty(), "2 s does not cut a segment");
    expect(continuous.speechClockSeconds() == 0,
        "samples below the loudness floor are not speech time");

    LiveCapture detected;
    detected.start(2, 30);
    detected.append(chunk.data(), count);
    const LiveCapture::Update opened = detected.advanceSpeech(0, count, 2000);
    expect(!opened.hasReading, "the frame that opens a phrase does not publish by itself");
    expect(opened.began, "that frame does open the phrase");

    std::vector<qint16> hop(static_cast<std::size_t>(LiveCapture::kHop), 1);
    const qint64 frameStart = detected.fed();
    detected.append(hop.data(), static_cast<int>(hop.size()));
    const LiveCapture::Update next = detected.advanceSpeech(frameStart, detected.fed(), 2008);
    expect(next.hasReading, "the next speech frame publishes as soon as the window is ready");
}

void testSpeechClockIsItsOwnTick()
{
    LiveCapture quiet;
    quiet.start(1, 60);
    for (int step = 0; step < 30; ++step) {
        std::vector<qint16> chunk(static_cast<std::size_t>(LiveCapture::kSampleRate / 10), 0);
        quiet.append(chunk.data(), static_cast<int>(chunk.size()));
    }
    expect(quiet.speechClockSeconds() == 0, "three seconds of silence stay at 0 speech time");

    LiveCapture take;
    take.start(1, 60);
    take.setPauseLimitMs(2000);
    const auto feedValue = [&](int milliseconds, qint16 value) {
        const int steps = milliseconds / 100;
        for (int step = 0; step < steps; ++step) {
            std::vector<qint16> chunk(static_cast<std::size_t>(LiveCapture::kSampleRate / 10), value);
            take.append(chunk.data(), static_cast<int>(chunk.size()));
        }
    };
    feedValue(1000, 3000);
    expect(take.speechClockSeconds() == 1, "speech time reaches 1 s without a gauge publish");
    feedValue(2000, 3000);
    expect(take.speechClockSeconds() == 3, "speech time reaches 3 s on its own tick");
    feedValue(2000, 0);
    expect(take.speechClockSeconds() == 5, "a pause within Pause (s) still counts");
    feedValue(3000, 0);
    expect(take.speechClockSeconds() == 5, "silence past Pause (s) does not extend speech time");
}

void testScreenPolicy()
{
    expect(!segmentClosePaintsScreen(true, false),
        "a segment stored while recording does not paint gauges or the chart");
    expect(segmentClosePaintsScreen(true, true),
        "Stop still paints the summary");
    expect(segmentClosePaintsScreen(false, false),
        "a segment that arrives after the session is idle can paint");
    expect(segmentClosePaintsScreen(false, true),
        "a segment that arrives while stopping can paint");
    expect(liveUpdateIntervalMs(2) == 30000, "2 updates per minute wait 30 s");
    expect(liveUpdateIntervalMs(60) == 1000, "60 updates per minute wait 1 s");
    expect(liveUpdateIntervalMs(1) == 60000, "1 update per minute waits 60 s");
    expect(liveUpdateIntervalMs(60000) == 1, "the wait is never shorter than 1 ms");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testScreenPolicy();
    testSpeechClockIsItsOwnTick();
    testFirstBuffer();
    const std::vector<float> thirtySecondTags { 1000.f, 2000.f, 3000.f };
    testCadenceAcrossCuts(2, 30, firstReadingMs() + 30000, thirtySecondTags);
    testCadenceAcrossCuts(60, 30, firstReadingMs() + 30000, thirtySecondTags);
    // Boris, Oct 23: 60 s window and 1 update per minute must not step at 15 s.
    const std::vector<float> sixtySecondTags { 1000.f, 2000.f, 3000.f, 4000.f, 5000.f };
    testCadenceAcrossCuts(1, 60, firstReadingMs() + 60000, sixtySecondTags);
    testSegmentEndDoesNotPublish();
    testShortPhraseIsNotAReading();
    if (failures == 0) {
        std::cout << "live cadence tests passed\n";
        return 0;
    }
    std::cerr << failures << " failure(s)\n";
    return 1;
}
