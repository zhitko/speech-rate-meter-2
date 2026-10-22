#ifndef LIVECADENCE_H
#define LIVECADENCE_H

#include <QtGlobal>

#include <deque>
#include <vector>

// Live readings while a session is open.
//
// Continuous speech is stored as 15 s segments. A cut, a pause that ends a
// segment, and the phrase that starts after that pause do not publish a
// reading and do not reset the updates-per-minute clock. The clock is armed
// on the first reading (once the window holds 1.5 s) and then only
// liveUpdateIntervalMs() can release the next one.
//
// The analysis window is still the last N seconds of kept segment audio
// followed by the open segment, so a reading after a cut describes speech
// from the segments already stored.
//
// Speech time is a separate clock. It advances once a second of detected
// speech and does not wait for the gauge cadence or for a segment cut.
class LiveCapture {
public:
    static constexpr int kSampleRate = 8000;
    static constexpr int kHop = 64;
    static constexpr int kVadFrame = 128;
    static constexpr qint64 kPadSamples = static_cast<qint64>(kSampleRate) * 300 / 1000;
    static constexpr qint64 kMinPhraseSamples = kSampleRate;
    static constexpr qint64 kMinWindowSamples = static_cast<qint64>(kSampleRate) * 3 / 2;
    static constexpr qint64 kMaxSegmentSamples = static_cast<qint64>(kSampleRate) * 15;

    struct ClosedSegment {
        std::vector<float> samples;
        int epoch = 0;
    };

    struct Reading {
        std::vector<float> samples;
        double openSeconds = 0;
        int epoch = 0;
        qint64 atMs = 0;
    };

    struct Update {
        bool began = false;
        bool hasReading = false;
        Reading reading;
        std::vector<ClosedSegment> closed;
        int epoch = 0;
    };

    void start(int updatesPerMinute, int analysisWindowSec);
    void setRates(int updatesPerMinute, int analysisWindowSec);
    // Pause (s), in milliseconds. A pause longer than this stops the speech clock.
    void setPauseLimitMs(int pauseLimitMs);

    void append(const qint16* samples, int count);
    void trim();

    // Integer seconds counted as speech. Advances on its own 1 s tick.
    int speechClockSeconds() const;

    // No speech detection: the whole take is one phrase. Publishes only when
    // the cadence is due and the window is long enough.
    Update advanceContinuous(qint64 nowMs);

    // One speech frame. The frame that opens a phrase does not publish.
    Update advanceSpeech(qint64 frameStart, qint64 frameEnd, qint64 nowMs);

    // One non-speech frame. May close the phrase. Never publishes.
    Update advanceSilence(int silenceLimitMs);

    // Stop. Keeps the open phrase when it is long enough. Never publishes.
    Update closeForStop();

    bool phraseSpeaking() const { return m_phraseSpeaking; }
    bool dropped() const { return m_dropped; }
    bool windowReady() const;
    int phraseSeconds() const;
    int epoch() const { return m_epoch; }
    qint64 fed() const { return m_fed; }

private:
    void begin(qint64 start, qint64 end, Update& update);
    void cut(Update& update);
    void closePhrase(bool pause, Update& update);
    void publish(qint64 nowMs, Update& update);
    void remember(const std::vector<float>& samples);
    std::vector<float> extract(qint64 start, qint64 end) const;
    qint64 span() const;
    qint64 openSamples() const;
    qint64 leadingStart() const;
    void finish(Update& update);
    void advanceSpeechClock();
    void noteSpeechClock(qint64 frameStart, qint64 frameEnd);
    qint64 speechClockSamples() const;
    qint64 pauseLimitSamples() const;
    bool loudSpeech(qint64 frameStart, qint64 frameEnd);

    std::vector<qint16> m_pcm;
    std::deque<float> m_recent;
    qint64 m_origin = 0;
    qint64 m_fed = 0;
    bool m_phraseSpeaking = false;
    bool m_padLeading = true;
    bool m_dropped = false;
    qint64 m_speechStart = 0;
    qint64 m_speechEnd = 0;
    int m_silenceFrames = 0;
    int m_epoch = 0;
    // False until the first live reading. A cut or a new phrase must not
    // clear this; that was publishing a reading on every 15 s boundary.
    bool m_clockArmed = false;
    qint64 m_lastPublishMs = 0;
    int m_intervalMs = 1000;
    qint64 m_windowSamples = static_cast<qint64>(kSampleRate) * 10;
    int m_pauseLimitMs = 2000;
    qint64 m_clockCounted = 0;
    qint64 m_clockRunStart = -1;
    qint64 m_clockLastSpeech = 0;
    qint64 m_clockCursor = 0;
    double m_clockFloor = -1;
    qint64 m_clockFed = 0;
};

// A stored segment updates the session collection immediately. While
// recording is still running, that close does not paint the gauges or add a
// tempo-chart point. Speech time keeps its own 1 s clock. Stop still paints
// the summary, including a segment that arrives as the session is ending.
inline bool segmentClosePaintsScreen(bool sessionActive, bool stopPending)
{
    return !sessionActive || stopPending;
}

#endif
