#ifndef SETTINGSAPI_H
#define SETTINGSAPI_H

#include "src/api/helpers/settings.h"
#include <QObject>
#include <QTranslator>

/**
 * Settings exposed to the home, settings, guide, and privacy pages,
 * including the voice-activity options used while recording.
 */
class SettingsApi : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString languageTitle READ languageTitle WRITE setLanguageTitle NOTIFY languageTitleChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(double fontSizeMultiplier READ fontSizeMultiplier WRITE setFontSizeMultiplier NOTIFY fontSizeMultiplierChanged)
    Q_PROPERTY(QString primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY primaryColorChanged)
    Q_PROPERTY(bool showNavigationMenu READ showNavigationMenu WRITE setShowNavigationMenu NOTIFY showNavigationMenuChanged)
    Q_PROPERTY(bool autoStopRecording READ autoStopRecording WRITE setAutoStopRecording NOTIFY autoStopRecordingChanged)
    Q_PROPERTY(bool autoCalibrate READ autoCalibrate WRITE setAutoCalibrate NOTIFY autoCalibrateChanged)
    Q_PROPERTY(int vadCalibrationDurationMs READ vadCalibrationDurationMs WRITE setVadCalibrationDurationMs NOTIFY vadCalibrationDurationMsChanged)
    Q_PROPERTY(int autoStopSilenceDuration READ autoStopSilenceDuration WRITE setAutoStopSilenceDuration NOTIFY autoStopSilenceDurationChanged)
    Q_PROPERTY(int vadMethod READ vadMethod WRITE setVadMethod NOTIFY vadMethodChanged)
    Q_PROPERTY(double vadThreshold READ vadThreshold WRITE setVadThreshold NOTIFY vadThresholdChanged)
    Q_PROPERTY(double autoCorrThreshold READ autoCorrThreshold WRITE setAutoCorrThreshold NOTIFY autoCorrThresholdChanged)
    Q_PROPERTY(double autoCorrThresholdK READ autoCorrThresholdK WRITE setAutoCorrThresholdK NOTIFY autoCorrThresholdKChanged)
    Q_PROPERTY(double autoCorrMinF0 READ autoCorrMinF0 WRITE setAutoCorrMinF0 NOTIFY autoCorrMinF0Changed)
    Q_PROPERTY(double autoCorrMaxF0 READ autoCorrMaxF0 WRITE setAutoCorrMaxF0 NOTIFY autoCorrMaxF0Changed)
    Q_PROPERTY(double autoCorrEnergyThreshold READ autoCorrEnergyThreshold WRITE setAutoCorrEnergyThreshold NOTIFY autoCorrEnergyThresholdChanged)
    Q_PROPERTY(bool advanced READ advanced WRITE setAdvanced NOTIFY advancedChanged)
    Q_PROPERTY(int analysisWindowSec READ analysisWindowSec WRITE setAnalysisWindowSec NOTIFY analysisWindowSecChanged)
    Q_PROPERTY(int updatesPerMinute READ updatesPerMinute WRITE setUpdatesPerMinute NOTIFY updatesPerMinuteChanged)
    Q_PROPERTY(int gaugeAverageCount READ gaugeAverageCount WRITE setGaugeAverageCount NOTIFY gaugeAverageCountChanged)
    Q_PROPERTY(int gaugeMode READ gaugeMode WRITE setGaugeMode NOTIFY gaugeModeChanged)
    Q_PROPERTY(int pauseSec READ pauseSec WRITE setPauseSec NOTIFY pauseSecChanged)
    Q_PROPERTY(int phrasalPauseMs READ phrasalPauseMs WRITE setPhrasalPauseMs NOTIFY phrasalPauseMsChanged)
    Q_PROPERTY(double slowWpm READ slowWpm WRITE setSlowWpm NOTIFY slowWpmChanged)
    Q_PROPERTY(double fastWpm READ fastWpm WRITE setFastWpm NOTIFY fastWpmChanged)
    Q_PROPERTY(int meanValueDegry READ meanValueDegry WRITE setMeanValueDegry NOTIFY meanValueDegryChanged)
    Q_PROPERTY(double k1 READ k1 WRITE setK1 NOTIFY k1Changed)
    Q_PROPERTY(double k2 READ k2 WRITE setK2 NOTIFY k2Changed)
    Q_PROPERTY(double k3 READ k3 WRITE setK3 NOTIFY k3Changed)
    Q_PROPERTY(double k4 READ k4 WRITE setK4 NOTIFY k4Changed)
    Q_PROPERTY(int intensityFrame READ intensityFrame WRITE setIntensityFrame NOTIFY intensityFrameChanged)
    Q_PROPERTY(int intensityShift READ intensityShift WRITE setIntensityShift NOTIFY intensityShiftChanged)
    Q_PROPERTY(int intensitySmooth READ intensitySmooth WRITE setIntensitySmooth NOTIFY intensitySmoothChanged)
    Q_PROPERTY(int segmentMinLengthMs READ segmentMinLengthMs WRITE setSegmentMinLengthMs NOTIFY segmentMinLengthMsChanged)
    Q_PROPERTY(double fillerMin READ fillerMin WRITE setFillerMin NOTIFY fillerMinChanged)
    Q_PROPERTY(double fillerMax READ fillerMax WRITE setFillerMax NOTIFY fillerMaxChanged)

public:
    explicit SettingsApi(QObject* parent = nullptr);

    QString language() const;
    void setLanguage(const QString& language);

    QString languageTitle() const;
    void setLanguageTitle(const QString& languageTitle);

    QString theme() const;
    void setTheme(const QString& theme);

    double fontSizeMultiplier() const;
    void setFontSizeMultiplier(double fontSizeMultiplier);

    QString primaryColor() const;
    void setPrimaryColor(const QString& primaryColor);

    bool showNavigationMenu() const;
    void setShowNavigationMenu(bool showNavigationMenu);

    bool autoStopRecording() const;
    void setAutoStopRecording(bool autoStopRecording);

    bool autoCalibrate() const;
    void setAutoCalibrate(bool autoCalibrate);

    int vadCalibrationDurationMs() const;
    void setVadCalibrationDurationMs(int vadCalibrationDurationMs);

    int autoStopSilenceDuration() const;
    void setAutoStopSilenceDuration(int autoStopSilenceDuration);

    int vadMethod() const;
    void setVadMethod(int method);

    double vadThreshold() const;
    void setVadThreshold(double vadThreshold);

    double autoCorrThreshold() const;
    void setAutoCorrThreshold(double threshold);

    double autoCorrThresholdK() const;
    void setAutoCorrThresholdK(double thresholdK);

    double autoCorrMinF0() const;
    void setAutoCorrMinF0(double minF0);

    double autoCorrMaxF0() const;
    void setAutoCorrMaxF0(double maxF0);

    double autoCorrEnergyThreshold() const;
    void setAutoCorrEnergyThreshold(double threshold);

    bool advanced() const;
    void setAdvanced(bool advanced);

    int analysisWindowSec() const;
    void setAnalysisWindowSec(int seconds);
    int updatesPerMinute() const;
    void setUpdatesPerMinute(int count);
    int gaugeAverageCount() const;
    void setGaugeAverageCount(int count);
    int gaugeMode() const;
    void setGaugeMode(int mode);
    int pauseSec() const;
    void setPauseSec(int seconds);
    int phrasalPauseMs() const;
    void setPhrasalPauseMs(int milliseconds);
    double slowWpm() const;
    void setSlowWpm(double wpm);
    double fastWpm() const;
    void setFastWpm(double wpm);
    int meanValueDegry() const;
    void setMeanValueDegry(int degree);
    double k1() const;
    void setK1(double value);
    double k2() const;
    void setK2(double value);
    double k3() const;
    void setK3(double value);
    double k4() const;
    void setK4(double value);
    int intensityFrame() const;
    void setIntensityFrame(int value);
    int intensityShift() const;
    void setIntensityShift(int value);
    int intensitySmooth() const;
    void setIntensitySmooth(int value);
    int segmentMinLengthMs() const;
    void setSegmentMinLengthMs(int value);
    double fillerMin() const;
    void setFillerMin(double value);
    double fillerMax() const;
    void setFillerMax(double value);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

signals:
    void settingsChanged();
    void languageChanged();
    void languageTitleChanged();
    void themeChanged();
    void fontSizeMultiplierChanged();
    void primaryColorChanged();
    void showNavigationMenuChanged();
    void autoStopRecordingChanged();
    void autoCalibrateChanged();
    void vadCalibrationDurationMsChanged();
    void autoStopSilenceDurationChanged();
    void vadMethodChanged();
    void vadThresholdChanged();
    void autoCorrThresholdChanged();
    void autoCorrThresholdKChanged();
    void autoCorrMinF0Changed();
    void autoCorrMaxF0Changed();
    void autoCorrEnergyThresholdChanged();
    void advancedChanged();
    void analysisWindowSecChanged();
    void updatesPerMinuteChanged();
    void gaugeAverageCountChanged();
    void gaugeModeChanged();
    void pauseSecChanged();
    void phrasalPauseMsChanged();
    void slowWpmChanged();
    void fastWpmChanged();
    void meanValueDegryChanged();
    void k1Changed();
    void k2Changed();
    void k3Changed();
    void k4Changed();
    void intensityFrameChanged();
    void intensityShiftChanged();
    void intensitySmoothChanged();
    void segmentMinLengthMsChanged();
    void fillerMinChanged();
    void fillerMaxChanged();
private:
    AppSettings m_settings;
    bool m_advanced = false;
    QTranslator m_translator;

    void updateTranslator();
    void applyFont();
};

#endif // SETTINGSAPI_H
