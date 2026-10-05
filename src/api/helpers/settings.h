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
    int vadCalibrationDurationMs = 2000;
    int autoStopSilenceDuration = 2000;
    int vadMethod = 0; // 0: energy, 1: autocorr, 2: hybrid
    double vadThreshold = 10000.0;
    double autoCorrThreshold = 0.3;
    double autoCorrThresholdK = 1.0;
    double autoCorrMinF0 = 80.0;
    double autoCorrMaxF0 = 300.0;
    double autoCorrEnergyThreshold = 0.02;
};

class Settings {
public:
    static AppSettings loadSettings();
    static void saveSettings(const AppSettings& settings);
    static AppSettings getDefaultSettings();

    /**
     * Desktop: directory of the executable.
     * Android: writable AppDataLocation, where startup extracts bundled assets.
     */
    static QString getAppDataDir();

private:
    static QString getSettingsFilePath();
};

#endif // SETTINGS_H
