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

    bool autoStopRecording = true;
    bool autoCalibrate = false;
    // When true, each phrase WAV stays in data/records after the app closes.
    // When false, the WAV stays until the app closes so it can be played.
    bool keepRecordingFiles = false;
    int vadCalibrationDurationMs = 2000;
    int autoStopSilenceDuration = 2000;
    // Whole-recording phrasal pauses: silent/unvoiced runs at least this long.
    // Matches speechrate::kDefaultPhrasalPauseMs.
    int phrasalPauseMs = 150;
    int analysisWindowSec = 10; // live metrics cover this much recent speech, 3…30
    int updatesPerMinute = 60; // live recalculations per minute, 6…240
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
    double speechRateK1 = 0.71;
    double speechRateMin = 70;
    double speechRateMax = 210;
    double articulationK2 = 1.2;
    double articulationMin = 70;
    double articulationMax = 210;
    double pausesK3 = 0.30;
    int intensityFrame = 240;
    int intensityShift = 120;
    int intensitySmooth = 120;
    int segmentMinLengthMs = 5;
    double fillerK4 = 100;
    double fillerMin = 120;
    double fillerMax = 240;
};

class Settings {
public:
    static AppSettings loadSettings();
    static void saveSettings(const AppSettings& settings);
    static AppSettings getDefaultSettings();

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

#endif // SETTINGS_H
