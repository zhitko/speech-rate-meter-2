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

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

    /**
     * Deletes saved recordings under data/records.
     */
    Q_INVOKABLE void clearUserData();

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

private:
    AppSettings m_settings;
    QTranslator m_translator;

    void updateTranslator();
    void applyFont();
};

#endif // SETTINGSAPI_H
