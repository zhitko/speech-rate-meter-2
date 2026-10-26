#include "helpers/settings.h"
#include "livecadence.h"
#include "speechrateanalysis.h"
#include "temposeries.h"
#include "wavloader.h"

#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <deque>
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

void expectNear(double actual, double expected, const std::string& message)
{
    if (!(std::fabs(actual - expected) <= 1e-6)) {
        std::cerr << "FAIL: " << message << " actual=" << actual << " expected=" << expected << '\n';
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

// Same middle-value rule as tempoSeries and the gauge.
double medianOf(const std::deque<double>& window)
{
    if (window.empty())
        return 0;
    std::vector<double> sorted(window.begin(), window.end());
    std::sort(sorted.begin(), sorted.end());
    const std::size_t mid = sorted.size() / 2;
    if (sorted.size() % 2 == 1)
        return sorted[mid];
    return (sorted[mid - 1] + sorted[mid]) / 2.0;
}

double pushMedian(std::deque<double>& window, double rate, int count, bool keepAll)
{
    window.push_back(rate);
    if (!keepAll) {
        const int keep = std::clamp(count, 1, 30);
        while (static_cast<int>(window.size()) > keep)
            window.pop_front();
    }
    return medianOf(window);
}

struct PacePoint {
    double seconds = 0;
    double speech = 0;
    double articulation = 0;
};

QString testsDir()
{
    QStringList roots;
#ifdef SRM_SOURCE_DIR
    roots << QString::fromUtf8(SRM_SOURCE_DIR);
#endif
    roots << QDir::currentPath();
    roots << QCoreApplication::applicationDirPath();
    for (const QString& root : roots) {
        QDir dir(root);
        for (int depth = 0; depth < 7; ++depth) {
            const QString candidate = dir.filePath(QStringLiteral("data/tests"));
            if (QDir(candidate).exists())
                return QDir(candidate).absolutePath();
            if (!dir.cdUp())
                break;
        }
    }
    return {};
}

QString findWav(const QString& dirPath, const QString& namePart)
{
    const QStringList names = QDir(dirPath).entryList(
        { QStringLiteral("*.wav"), QStringLiteral("*.WAV") }, QDir::Files, QDir::Name);
    for (const QString& name : names) {
        if (name.contains(namePart))
            return QDir(dirPath).filePath(name);
    }
    return {};
}

std::vector<float> loadWav(const QString& path)
{
    const WavLoadResult loaded = loadAnalysisWav(path);
    expect(loaded.ok && !loaded.samples.empty(),
        "load " + path.toStdString());
    return loaded.samples;
}

qint16 clampPcm(float sample)
{
    const long rounded = std::lround(static_cast<double>(sample));
    return static_cast<qint16>(std::clamp(rounded, -32768L, 32767L));
}

// Points the microphone would publish for this file when the clock is the
// sample position and advanceContinuous is called on each update mark, plus
// the file-end point Open File keeps when that mark has not come yet.
// Speech rate and articulation are smoothed separately.
std::vector<PacePoint> liveFilePoints(const std::vector<float>& samples,
    const speechrate::Config& cfg,
    int updatesPerMinute,
    int windowSec,
    int chartWindowSec,
    int gaugeAverageCount,
    bool useAllReadings)
{
    std::vector<PacePoint> points;
    const qint64 total = static_cast<qint64>(samples.size());
    if (total < LiveCapture::kMinWindowSamples)
        return points;

    const int intervalMs = liveUpdateIntervalMs(updatesPerMinute);
    const qint64 step = static_cast<qint64>(intervalMs) * LiveCapture::kSampleRate / 1000;
    const qint64 windowSamples = static_cast<qint64>(clampAnalysisWindowSec(windowSec)) * LiveCapture::kSampleRate;
    const int historySec = std::clamp(chartWindowSec, 30, 1800);
    const qint64 regionStart = std::max<qint64>(
        0, total - static_cast<qint64>(historySec) * LiveCapture::kSampleRate);

    LiveCapture take;
    take.start(updatesPerMinute, windowSec);
    std::deque<double> speechWindow;
    std::deque<double> articulationWindow;
    qint64 fed = 0;

    const auto record = [&](qint64 end, const std::vector<float>& audio) {
        if (end < LiveCapture::kMinWindowSamples || end < regionStart || end > total)
            return;
        const qint64 start = (windowSamples > 0 && end > windowSamples) ? end - windowSamples : 0;
        expect(static_cast<qint64>(audio.size()) == end - start,
            "live window at " + std::to_string(end) + " samples has the analysis-window length");
        const speechrate::Measurement measured = speechrate::measure(audio, cfg);
        if (!measured.metrics.valid)
            return;
        PacePoint point;
        point.seconds = static_cast<double>(end) / LiveCapture::kSampleRate;
        point.speech = pushMedian(speechWindow, measured.metrics.speechRate, gaugeAverageCount, useAllReadings);
        point.articulation = pushMedian(articulationWindow, measured.metrics.articulationRate, gaugeAverageCount, useAllReadings);
        points.push_back(point);
    };

    const auto feedTo = [&](qint64 next, bool keepFileEnd) {
        if (next <= fed)
            return;
        std::vector<qint16> chunk(static_cast<std::size_t>(next - fed));
        for (qint64 index = fed; index < next; ++index)
            chunk[static_cast<std::size_t>(index - fed)] = clampPcm(samples[static_cast<std::size_t>(index)]);
        take.append(chunk.data(), static_cast<int>(chunk.size()));
        const qint64 nowMs = next * 1000 / LiveCapture::kSampleRate;
        const LiveCapture::Update update = take.advanceContinuous(nowMs);
        take.trim();
        fed = next;
        if (update.hasReading) {
            record(next, update.reading.samples);
        } else if (keepFileEnd) {
            const qint64 start = (windowSamples > 0 && next > windowSamples) ? next - windowSamples : 0;
            record(next, std::vector<float>(samples.begin() + static_cast<std::ptrdiff_t>(start),
                samples.begin() + static_cast<std::ptrdiff_t>(next)));
        }
    };

    for (qint64 end = step; end < total; end += step)
        feedTo(end, false);
    if (fed < total)
        feedTo(total, true);
    return points;
}

void expectSeriesMatches(const QVariantList& series, const std::vector<PacePoint>& live, const char* label)
{
    expect(series.size() == static_cast<int>(live.size()),
        std::string(label) + " point count " + std::to_string(series.size())
            + " vs live " + std::to_string(live.size()));
    const int count = std::min(static_cast<int>(series.size()), static_cast<int>(live.size()));
    for (int index = 0; index < count; ++index) {
        const QVariantMap point = series.at(index).toMap();
        const std::string where = std::string(label) + " point " + std::to_string(index);
        expectNear(point.value(QStringLiteral("seconds")).toDouble(), live[static_cast<std::size_t>(index)].seconds,
            where + " time");
        expectNear(point.value(QStringLiteral("speechRate")).toDouble(), live[static_cast<std::size_t>(index)].speech,
            where + " speech rate");
        expectNear(point.value(QStringLiteral("articulationRate")).toDouble(), live[static_cast<std::size_t>(index)].articulation,
            where + " articulation rate");
    }
}

void expectFileMatchesLive(const std::vector<float>& samples, const char* label,
    int windowSec, int updatesPerMinute, int chartWindowSec, int gaugeAverageCount, bool useAllReadings)
{
    const speechrate::Config cfg;
    const QVariantList series = tempoSeries(samples, cfg, windowSec, chartWindowSec,
        gaugeAverageCount, useAllReadings, updatesPerMinute);
    const std::vector<PacePoint> live = liveFilePoints(samples, cfg, updatesPerMinute, windowSec,
        chartWindowSec, gaugeAverageCount, useAllReadings);
    expectSeriesMatches(series, live, label);
    if (!series.isEmpty()) {
        const double duration = static_cast<double>(samples.size()) / LiveCapture::kSampleRate;
        expectNear(series.last().toMap().value(QStringLiteral("seconds")).toDouble(), duration,
            std::string(label) + " keeps a point at the end of the file");
        const double oldest = duration - std::clamp(chartWindowSec, 30, 1800);
        for (const QVariant& item : series) {
            expect(item.toMap().value(QStringLiteral("seconds")).toDouble() + 1e-9 >= oldest,
                std::string(label) + " stays inside the chart window");
        }
    }
}

void testOpenFileTempoMatchesLive()
{
    const QString dir = testsDir();
    expect(!dir.isEmpty(), "data/tests is present");
    if (dir.isEmpty())
        return;

    const QString fastPath = findWav(dir, QStringLiteral("1 English_Fast"));
    const QString slowPath = findWav(dir, QStringLiteral("2 English_Slow"));
    const QString shortPath = findWav(dir, QStringLiteral("Advatasment"));
    const QString slidingPath = findWav(dir, QStringLiteral("110"));
    const QString longPath = findWav(dir, QStringLiteral("Helena"));
    expect(!fastPath.isEmpty() && !slowPath.isEmpty() && !shortPath.isEmpty()
            && !slidingPath.isEmpty() && !longPath.isEmpty(),
        "the Open File comparison recordings are in data/tests");
    if (fastPath.isEmpty() || slowPath.isEmpty() || shortPath.isEmpty()
        || slidingPath.isEmpty() || longPath.isEmpty())
        return;

    const std::vector<float> fast = loadWav(fastPath);
    const std::vector<float> slow = loadWav(slowPath);
    const std::vector<float> brief = loadWav(shortPath);
    const std::vector<float> sliding = loadWav(slidingPath);
    const std::vector<float> longTake = loadWav(longPath);
    if (fast.empty() || slow.empty() || brief.empty() || sliding.empty() || longTake.empty())
        return;

    constexpr int kWindowSec = 60;
    constexpr int kUpdates = 12;
    constexpr int kChartSec = 300;

    // Shorter than the 1.5 s gate: no chart sample. The whole-file gauge is
    // filled by the caller, not by tempoSeries.
    expect(fast.size() > static_cast<std::size_t>(LiveCapture::kSampleRate), "English Fast is longer than 1 s");
    const std::vector<float> oneSecond(fast.begin(), fast.begin() + LiveCapture::kSampleRate);
    expect(tempoSeries(oneSecond, speechrate::Config(), kWindowSec, kChartSec, 1, false, kUpdates).isEmpty(),
        "a file shorter than 1.5 s yields no tempo point");

    // Longer than 1.5 s, shorter than the 5 s step, and shorter than the
    // 60 s analysis window: one point, the whole prefix.
    const std::vector<float> twoSeconds(fast.begin(), fast.begin() + 2 * LiveCapture::kSampleRate);
    const speechrate::Measurement twoMeasured = speechrate::measure(twoSeconds, speechrate::Config());
    expect(twoMeasured.metrics.valid, "two seconds of English Fast is measurable");
    const QVariantList twoSeries = tempoSeries(twoSeconds, speechrate::Config(), kWindowSec, kChartSec, 1, true, kUpdates);
    expect(twoSeries.size() == 1, "a file shorter than the update step is one point at the end");
    if (twoSeries.size() == 1) {
        expectNear(twoSeries.first().toMap().value(QStringLiteral("seconds")).toDouble(), 2.0,
            "the short-file point is stamped at the end");
        expectNear(twoSeries.first().toMap().value(QStringLiteral("speechRate")).toDouble(),
            twoMeasured.metrics.speechRate, "the short-file point is the whole prefix");
        expectNear(twoSeries.first().toMap().value(QStringLiteral("articulationRate")).toDouble(),
            twoMeasured.metrics.articulationRate, "the short-file articulation is the whole prefix");
    }

    // Window larger than the file: every point uses a prefix, not a full 60 s.
    expect(brief.size() < static_cast<std::size_t>(kWindowSec * LiveCapture::kSampleRate),
        "the advertisement is shorter than the 60 s window");
    expectFileMatchesLive(brief, "window larger than the file, gauge 1", kWindowSec, kUpdates, kChartSec, 1, false);
    expectFileMatchesLive(brief, "window larger than the file, all readings", kWindowSec, kUpdates, kChartSec, 3, true);

    // The analysis window actually slides. Chart window still holds the file.
    expect(sliding.size() > static_cast<std::size_t>(kWindowSec * LiveCapture::kSampleRate),
        "the 110 s file is longer than the analysis window");
    expect(sliding.size() < static_cast<std::size_t>(kChartSec * LiveCapture::kSampleRate),
        "the 110 s file fits in the chart window");
    expectFileMatchesLive(sliding, "sliding window, gauge 1", kWindowSec, kUpdates, kChartSec, 1, false);
    expectFileMatchesLive(sliding, "sliding window, gauge median 3", kWindowSec, kUpdates, kChartSec, 3, false);
    expectFileMatchesLive(sliding, "sliding window, all readings", kWindowSec, kUpdates, kChartSec, 3, true);

    const QVariantList raw = tempoSeries(sliding, speechrate::Config(), kWindowSec, kChartSec, 1, false, kUpdates);
    const QVariantList averaged = tempoSeries(sliding, speechrate::Config(), kWindowSec, kChartSec, 3, false, kUpdates);
    const QVariantList allReadings = tempoSeries(sliding, speechrate::Config(), kWindowSec, kChartSec, 1, true, kUpdates);
    expect(raw.size() > 10, "12 updates per minute on a 110 s file is more than a handful of points");
    expect(raw.size() == averaged.size() && raw.size() == allReadings.size(),
        "smoothing does not change how many points are kept");
    bool gaugeDiffers = false;
    bool allDiffers = false;
    for (int index = 0; index < raw.size(); ++index) {
        const double speech = raw.at(index).toMap().value(QStringLiteral("speechRate")).toDouble();
        if (std::fabs(speech - averaged.at(index).toMap().value(QStringLiteral("speechRate")).toDouble()) > 1e-6)
            gaugeDiffers = true;
        if (std::fabs(speech - allReadings.at(index).toMap().value(QStringLiteral("speechRate")).toDouble()) > 1e-6)
            allDiffers = true;
    }
    expect(gaugeDiffers, "gauge median 3 is not the raw reading on this file");
    expect(allDiffers, "use all readings is not the raw reading on this file");

    // A take longer than the chart window keeps only the last 300 s, and
    // those points still match the live windows (gauge median 1 needs no
    // earlier readings).
    expect(longTake.size() > static_cast<std::size_t>(kChartSec * LiveCapture::kSampleRate),
        "Helena is longer than the chart window");
    expectFileMatchesLive(longTake, "chart window trims a long file", kWindowSec, kUpdates, kChartSec, 1, false);
    expectFileMatchesLive(slow, "English Slow, gauge median 4", kWindowSec, kUpdates, kChartSec, 4, false);
    expectFileMatchesLive(slow, "English Slow, all readings", kWindowSec, kUpdates, kChartSec, 1, true);
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
    testOpenFileTempoMatchesLive();
    if (failures == 0) {
        std::cout << "live cadence tests passed\n";
        return 0;
    }
    std::cerr << failures << " failure(s)\n";
    return 1;
}
