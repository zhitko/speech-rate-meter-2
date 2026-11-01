#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <string>

/*
 * AppSettings holds the preferences used by the shell and the recorder.
 * DSP services keep their own parameters; they are not configured from here.
 */
struct AppSettings {
    std::string language = "ru";
    std::string languageTitle = "Русский";
    std::string theme = "light";
    double fontSizeMultiplier = 1.0;
    std::string primaryColor = "blue";
    bool showNavigationMenu = false;
    // 0: standard preset, the shipped settings.ini (long window, few updates).
    // 1: fast preset, settings_fast.ini (short window, frequent updates).
    int preset = 0;

    bool autoStopRecording = true;
    bool autoCalibrate = false;
    // When true, each phrase WAV stays in data/records after the app closes.
    // When false, the WAV stays until the app closes so it can be played.
    bool keepRecordingFiles = false;
    int vadCalibrationDurationMs = 2000;
    int autoStopSilenceDuration = 2000;
    // Whole-recording phrasal pauses: silent/unvoiced runs at least this long.
    // Matches speechrate::kDefaultPhrasalPauseMs.
    int phrasalPauseMs = 200;
    // 0: direct pauses from the signal. 1: legacy gap-distribution estimate.
    // 2: window articulation, T_window minus the same pauses.
    int articulationMethod = 0;
    // 0: speech rate over the utterance span. 1: speech rate over the whole buffer.
    int speechRateMethod = 0;
    // Session summary after Stop. Matches speechrate::SummaryMethod.
    // 2 is the median.
    int summaryMethod = 2;
    int analysisWindowSec = 10; // live metrics, and each Open File chart point, cover this much recent speech; at least 3, no upper limit
    int updatesPerMinute = 60; // live recalculations and Open File chart points per minute; at least 1, no upper limit
    // Tempo chart on Home. A point is stored on each live update.
    // The chart draws the last this many seconds. 30…1800.
    int tempoChartWindowSec = 300;
    // Tempo chart. True: each point is the median of every live reading since Start.
    // False: each point is the gauge median. The arcs always use the gauge median.
    bool chartUseAllReadings = true;
    int gaugeAverageCount = 3; // each pace on the gauge shows the median of this many latest readings, 1…30
    int gaugeMode = 0; // 0: speech rate, 1: articulation rate, 2: both
    // Tiles beside the gauge. A pace already drawn on the gauge is not shown again.
    bool showSpeechRateTile = true;
    bool showArticulationRateTile = true;
    bool showFillersTile = false;
    bool showPausesTile = true;
    bool showSpeechTile = true;
    bool showWholeRecordingTile = true;
    int vadMethod = 0; // 0: energy, 1: autocorr, 2: hybrid
    double vadThreshold = 10000.0;
    double autoCorrThreshold = 0.3;
    double autoCorrThresholdK = 1.0;
    double autoCorrMinF0 = 80.0;
    double autoCorrMaxF0 = 300.0;
    double autoCorrEnergyThreshold = 0.02;

    int meanValueDegry = 3;
    double speechRateK1 = 0.41;
    double speechRateMin = 70;
    double speechRateMax = 210;
    double articulationK2 = 1.2;
    double articulationMin = 70;
    double articulationMax = 210;
    double pausesK3 = 0.30;
    int intensityFrame = 64;
    int intensityShift = 32;
    int intensitySmooth = 64;
    int segmentMinLengthMs = 8;
    double fillerK4 = 100;
    // Window articulation scale. Stored separately from K1.
    double windowK5 = 0.71;
    double fillerMin = 120;
    double fillerMax = 240;
};

class Settings {
public:
    static AppSettings loadSettings();
    static void saveSettings(const AppSettings& settings);
    static AppSettings getDefaultSettings();
    // Fills settings from the preset file. Does not change the live settings.ini.
    // 0 reads the shipped standard file. 1 reads settings_fast.ini beside the
    // program, or the copy bundled in the binary when that file is absent.
    static bool readPreset(int preset, AppSettings* settings);

    /**
     * Unpackaged desktop build: directory of the executable.
     * AppImage: directory containing the .AppImage file. The mounted payload
     * cannot store settings or sessions. If that directory cannot be written,
     * AppDataLocation is used.
     * Android: writable AppDataLocation, where startup extracts bundled assets.
     */
    static QString getAppDataDir();

    /** Overrides the application data root for isolated backend tests. */
    static void setAppDataDirForTests(const QString& path);
    static void clearAppDataDirForTests();

private:
    static QString getSettingsFilePath();
};

// Analysis window has no upper bound. Fewer than 3 s is raised to 3 s.
inline int clampAnalysisWindowSec(int seconds)
{
    return seconds < 3 ? 3 : seconds;
}

// One update a minute is the slowest cadence. Zero would make the wait undefined.
inline int clampUpdatesPerMinute(int count)
{
    return count < 1 ? 1 : count;
}

// Milliseconds between live publishes. Never 0, so the timer cannot spin.
// 1 per minute waits 60 s. Above 60000 per minute the wait stays 1 ms.
inline int liveUpdateIntervalMs(int updatesPerMinute)
{
    const int perMinute = clampUpdatesPerMinute(updatesPerMinute);
    const int interval = 60000 / perMinute;
    return interval < 1 ? 1 : interval;
}

// Speech the first live reading needs in the window. LiveCapture publishes
// once this much audio is buffered (1.5 s), then again one update interval
// later. The countdown uses this minimum plus one interval per later reading.
inline constexpr int kLiveReadingMinSpeechMs = 1500;

// Home keeps “Everything is working” until this many live readings.
inline constexpr int kLiveChartReadings = 5;

// Seconds still to show until the fifth live reading, and whether that
// estimate has already run out. `seconds` is rounded up and is never
// negative. With fewer than five readings, a spent estimate is `almost`
// (the neutral “a little longer” line) instead of 0.
struct LiveChartWait {
    int seconds = 0;
    bool almost = false;
};

inline LiveChartWait estimateLiveChartWait(int updatesPerMinute, int collectedSpeechMs, int liveReadings)
{
    LiveChartWait wait;
    if (liveReadings >= kLiveChartReadings)
        return wait;
    const int elapsed = collectedSpeechMs < 0 ? 0 : collectedSpeechMs;
    const int laterReadings = kLiveChartReadings - 1;
    const int target = kLiveReadingMinSpeechMs
        + laterReadings * liveUpdateIntervalMs(updatesPerMinute);
    const int left = target - elapsed;
    if (left <= 0) {
        wait.almost = true;
        return wait;
    }
    wait.seconds = (left + 999) / 1000;
    return wait;
}

// Keep a countdown from rising. A slower update rate, or a clock that has
// not moved, must not replace a shown number with a larger one, and the
// neutral line must not flip back to a count. Five readings clear it.
inline LiveChartWait holdLiveChartWait(const LiveChartWait& shown, const LiveChartWait& next)
{
    if (next.seconds == 0 && !next.almost)
        return next;
    if (!shown.almost && shown.seconds <= 0)
        return next;
    if (shown.almost)
        return shown;
    if (next.almost)
        return next;
    if (next.seconds > shown.seconds)
        return shown;
    return next;
}

#endif // SETTINGS_H
