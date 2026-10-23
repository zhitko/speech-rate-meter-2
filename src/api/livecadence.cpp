#include "livecadence.h"

#include "helpers/settings.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

void LiveCapture::start(int updatesPerMinute, int analysisWindowSec)
{
    m_pcm.clear();
    m_recent.clear();
    m_origin = 0;
    m_fed = 0;
    m_phraseSpeaking = false;
    m_padLeading = true;
    m_dropped = false;
    m_speechStart = 0;
    m_speechEnd = 0;
    m_silenceFrames = 0;
    m_epoch = 0;
    m_clockArmed = false;
    m_lastPublishMs = 0;
    m_pauseLimitMs = 2000;
    m_clockCounted = 0;
    m_clockRunStart = -1;
    m_clockLastSpeech = 0;
    m_clockCursor = 0;
    m_clockFloor = -1;
    m_clockFed = 0;
    setRates(updatesPerMinute, analysisWindowSec);
}

void LiveCapture::setPauseLimitMs(int pauseLimitMs)
{
    m_pauseLimitMs = std::max(0, pauseLimitMs);
}

void LiveCapture::setRates(int updatesPerMinute, int analysisWindowSec)
{
    m_intervalMs = liveUpdateIntervalMs(updatesPerMinute);
    m_windowSamples = static_cast<qint64>(clampAnalysisWindowSec(analysisWindowSec)) * kSampleRate;
}

void LiveCapture::append(const qint16* samples, int count)
{
    if (!samples || count <= 0)
        return;
    m_pcm.insert(m_pcm.end(), samples, samples + count);
    m_fed += count;
    // Count speech before any later trim drops the samples just stored.
    // This clock does not publish a gauge or a chart point.
    advanceSpeechClock();
}

LiveCapture::Update LiveCapture::advanceContinuous(qint64 nowMs)
{
    Update update;
    if (!m_phraseSpeaking)
        begin(0, m_fed, update);
    else
        m_speechEnd = m_fed;
    cut(update);
    publish(nowMs, update);
    finish(update);
    return update;
}

LiveCapture::Update LiveCapture::advanceSpeech(qint64 frameStart, qint64 frameEnd, qint64 nowMs)
{
    Update update;
    m_silenceFrames = 0;
    if (!m_phraseSpeaking) {
        begin(frameStart, frameEnd, update);
    } else {
        m_speechEnd = std::max(m_speechEnd, frameEnd);
        cut(update);
        publish(nowMs, update);
    }
    finish(update);
    return update;
}

LiveCapture::Update LiveCapture::advanceSilence(int silenceLimitMs)
{
    Update update;
    if (m_phraseSpeaking) {
        ++m_silenceFrames;
        const int silenceMs = (m_silenceFrames * kHop * 1000) / kSampleRate;
        if (silenceMs >= silenceLimitMs)
            closePhrase(true, update);
    }
    finish(update);
    return update;
}

LiveCapture::Update LiveCapture::closeForStop()
{
    Update update;
    if (m_phraseSpeaking)
        closePhrase(false, update);
    finish(update);
    return update;
}

bool LiveCapture::windowReady() const
{
    return static_cast<qint64>(m_recent.size()) + openSamples() >= kMinWindowSamples;
}

int LiveCapture::phraseSeconds() const
{
    if (!m_phraseSpeaking)
        return 0;
    return static_cast<int>(span() / kSampleRate);
}

void LiveCapture::begin(qint64 start, qint64 end, Update& update)
{
    m_phraseSpeaking = true;
    m_padLeading = true;
    m_dropped = false;
    m_speechStart = start;
    m_speechEnd = end;
    m_silenceFrames = 0;
    ++m_epoch;
    update.began = true;
    // Leave the cadence clock alone. Start() is what disarms it, so the
    // first reading still waits only for 1.5 s of audio. A later phrase
    // keeps the wait that is already running.
}

void LiveCapture::cut(Update& update)
{
    while (m_phraseSpeaking && span() >= kMaxSegmentSamples) {
        const qint64 at = m_speechStart + kMaxSegmentSamples;
        ClosedSegment closed;
        closed.samples = extract(leadingStart(), at);
        closed.epoch = m_epoch;
        if (!closed.samples.empty()) {
            remember(closed.samples);
            update.closed.push_back(std::move(closed));
        }
        m_speechStart = at;
        m_padLeading = false;
        if (m_speechEnd < m_speechStart)
            m_speechEnd = m_speechStart;
        // A new segment id drops a live job that still describes the
        // pre-cut buffer. It must not arm a new reading: the clock stays
        // on the updates-per-minute marks.
        ++m_epoch;
    }
}

void LiveCapture::closePhrase(bool pause, Update& update)
{
    const bool keep = span() >= kMinPhraseSamples;
    if (keep) {
        const qint64 end = m_speechEnd + kPadSamples;
        ClosedSegment closed;
        closed.samples = extract(leadingStart(), end);
        closed.epoch = m_epoch;
        if (!closed.samples.empty()) {
            remember(closed.samples);
            update.closed.push_back(std::move(closed));
        }
    } else if (pause) {
        m_dropped = true;
    }
    m_phraseSpeaking = false;
    m_speechStart = 0;
    m_speechEnd = 0;
    m_silenceFrames = 0;
    ++m_epoch;
}

void LiveCapture::publish(qint64 nowMs, Update& update)
{
    if (!m_phraseSpeaking || !windowReady())
        return;
    if (m_clockArmed && (nowMs - m_lastPublishMs) < m_intervalMs)
        return;
    m_clockArmed = true;
    m_lastPublishMs = nowMs;

    const std::vector<float> open = extract(leadingStart(), m_speechEnd);
    const std::size_t window = static_cast<std::size_t>(std::max<qint64>(0, m_windowSamples));
    Reading reading;
    if (open.size() >= window && window > 0) {
        reading.samples.assign(open.end() - static_cast<std::ptrdiff_t>(window), open.end());
    } else {
        const std::size_t fromRecent = window > open.size()
            ? std::min(m_recent.size(), window - open.size())
            : 0;
        reading.samples.reserve(fromRecent + open.size());
        if (fromRecent > 0) {
            reading.samples.insert(reading.samples.end(),
                m_recent.end() - static_cast<std::ptrdiff_t>(fromRecent), m_recent.end());
        }
        reading.samples.insert(reading.samples.end(), open.begin(), open.end());
    }
    reading.openSeconds = static_cast<double>(open.size()) / kSampleRate;
    reading.epoch = m_epoch;
    reading.atMs = nowMs;
    update.reading = std::move(reading);
    update.hasReading = true;
}

void LiveCapture::remember(const std::vector<float>& samples)
{
    m_recent.insert(m_recent.end(), samples.begin(), samples.end());
    const qint64 cap = m_windowSamples;
    if (cap > 0 && static_cast<qint64>(m_recent.size()) > cap) {
        m_recent.erase(m_recent.begin(),
            m_recent.end() - static_cast<std::ptrdiff_t>(cap));
    }
}

std::vector<float> LiveCapture::extract(qint64 start, qint64 end) const
{
    if (start < m_origin)
        start = m_origin;
    const qint64 available = m_origin + static_cast<qint64>(m_pcm.size());
    if (end > available)
        end = available;
    if (end < start)
        end = start;
    std::vector<float> samples;
    samples.reserve(static_cast<std::size_t>(end - start));
    for (qint64 index = start; index < end; ++index)
        samples.push_back(static_cast<float>(m_pcm[static_cast<std::size_t>(index - m_origin)]));
    return samples;
}

qint64 LiveCapture::span() const
{
    return std::max<qint64>(0, m_speechEnd - m_speechStart);
}

qint64 LiveCapture::openSamples() const
{
    return std::max<qint64>(0, m_speechEnd - leadingStart());
}

qint64 LiveCapture::leadingStart() const
{
    if (!m_padLeading)
        return m_speechStart;
    return m_speechStart - kPadSamples;
}

void LiveCapture::finish(Update& update)
{
    update.epoch = m_epoch;
}

void LiveCapture::advanceSpeechClock()
{
    while (m_clockFed + kHop <= m_fed) {
        noteSpeechClock(m_clockFed, m_clockFed + kHop);
        m_clockFed += kHop;
    }
}

void LiveCapture::noteSpeechClock(qint64 frameStart, qint64 frameEnd)
{
    const bool speech = loudSpeech(frameStart, frameEnd);
    if (frameEnd > m_clockCursor)
        m_clockCursor = frameEnd;
    const qint64 limit = pauseLimitSamples();
    if (speech) {
        if (m_clockRunStart < 0) {
            m_clockRunStart = frameStart;
            m_clockLastSpeech = frameEnd;
        } else {
            m_clockLastSpeech = std::max(m_clockLastSpeech, frameEnd);
        }
    } else if (m_clockRunStart >= 0 && frameEnd - m_clockLastSpeech > limit) {
        m_clockCounted += std::max<qint64>(0, m_clockLastSpeech + limit - m_clockRunStart);
        m_clockRunStart = -1;
    }
}

qint64 LiveCapture::speechClockSamples() const
{
    if (m_clockRunStart < 0)
        return m_clockCounted;
    qint64 end = m_clockCursor;
    if (end > m_clockLastSpeech) {
        const qint64 gap = end - m_clockLastSpeech;
        const qint64 limit = pauseLimitSamples();
        if (gap > limit)
            end = m_clockLastSpeech + limit;
    }
    return m_clockCounted + std::max<qint64>(0, end - m_clockRunStart);
}

int LiveCapture::speechClockSeconds() const
{
    return static_cast<int>(speechClockSamples() / kSampleRate);
}

qint64 LiveCapture::pauseLimitSamples() const
{
    return static_cast<qint64>(std::max(0, m_pauseLimitMs)) * kSampleRate / 1000;
}

bool LiveCapture::loudSpeech(qint64 frameStart, qint64 frameEnd)
{
    qint64 start = frameStart;
    qint64 end = frameEnd;
    if (start < m_origin)
        start = m_origin;
    const qint64 available = m_origin + static_cast<qint64>(m_pcm.size());
    if (end > available)
        end = available;
    if (end <= start)
        return false;
    double sum = 0;
    for (qint64 index = start; index < end; ++index)
        sum += std::abs(static_cast<int>(m_pcm[static_cast<std::size_t>(index - m_origin)]));
    const double level = sum / static_cast<double>(end - start);

    // A frame this quiet is background. Anything louder counts as speech
    // until a floor has been learned, and then it must also clear that floor.
    constexpr double kQuiet = 200.0;
    if (m_clockFloor < 0) {
        if (level < kQuiet)
            m_clockFloor = level;
        return level >= kQuiet;
    }
    if (level < m_clockFloor)
        m_clockFloor = level;
    else if (level < m_clockFloor * 2.0)
        m_clockFloor += (level - m_clockFloor) * 0.02;
    const double gate = std::max(kQuiet, m_clockFloor * 3.0 + 120.0);
    return level >= gate;
}

void LiveCapture::trim()
{
    // Keep a second of audio before the 300 ms pad so a late VAD decision
    // can still include the leading pad.
    qint64 keep = m_fed - (kPadSamples + kSampleRate);
    if (m_phraseSpeaking)
        keep = leadingStart();
    if (keep < m_origin)
        keep = m_origin;
    const qint64 drop = keep - m_origin;
    if (drop <= 0 || drop > static_cast<qint64>(m_pcm.size()))
        return;
    m_pcm.erase(m_pcm.begin(), m_pcm.begin() + static_cast<std::ptrdiff_t>(drop));
    m_origin = keep;
}
