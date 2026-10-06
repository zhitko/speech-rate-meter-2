#include "settingsapi.h"

#include "helpers/logger.h"
#include "helpers/settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QQmlEngine>
#include <algorithm>

namespace {

template <typename T>
bool assignIfChanged(T& field, const T& value)
{
    if (field == value)
        return false;
    field = value;
    return true;
}

bool assignIfChanged(double& field, double value)
{
    if (qAbs(field - value) < 0.0001)
        return false;
    field = value;
    return true;
}

} // namespace

SettingsApi::SettingsApi(QObject* parent)
    : QObject(parent)
{
    load();
}

QString SettingsApi::language() const
{
    return QString::fromStdString(m_settings.language);
}

void SettingsApi::setLanguage(const QString& language)
{
    if (!assignIfChanged(m_settings.language, language.toStdString()))
        return;
    if (language == QLatin1String("ru"))
        m_settings.languageTitle = "Русский";
    else if (language == QLatin1String("en"))
        m_settings.languageTitle = "English";
    save();
    updateTranslator();
    emit languageChanged();
    emit languageTitleChanged();
}

QString SettingsApi::languageTitle() const
{
    return QString::fromStdString(m_settings.languageTitle);
}

void SettingsApi::setLanguageTitle(const QString& languageTitle)
{
    if (!assignIfChanged(m_settings.languageTitle, languageTitle.toStdString()))
        return;
    save();
    emit languageTitleChanged();
}

QString SettingsApi::theme() const
{
    return QString::fromStdString(m_settings.theme);
}

void SettingsApi::setTheme(const QString& theme)
{
    if (!assignIfChanged(m_settings.theme, theme.toStdString()))
        return;
    save();
    emit themeChanged();
}

double SettingsApi::fontSizeMultiplier() const
{
    return m_settings.fontSizeMultiplier;
}

void SettingsApi::setFontSizeMultiplier(double fontSizeMultiplier)
{
    const double clamped = std::clamp(fontSizeMultiplier, 1.0, 2.0);
    if (!assignIfChanged(m_settings.fontSizeMultiplier, clamped))
        return;
    applyFont();
    save();
    emit fontSizeMultiplierChanged();
}

QString SettingsApi::primaryColor() const
{
    return QString::fromStdString(m_settings.primaryColor);
}

void SettingsApi::setPrimaryColor(const QString& primaryColor)
{
    if (!assignIfChanged(m_settings.primaryColor, primaryColor.toStdString()))
        return;
    save();
    emit primaryColorChanged();
}

bool SettingsApi::showNavigationMenu() const
{
    return m_settings.showNavigationMenu;
}

void SettingsApi::setShowNavigationMenu(bool showNavigationMenu)
{
    if (!assignIfChanged(m_settings.showNavigationMenu, showNavigationMenu))
        return;
    save();
    emit showNavigationMenuChanged();
}

bool SettingsApi::autoStopRecording() const
{
    return m_settings.autoStopRecording;
}

void SettingsApi::setAutoStopRecording(bool autoStopRecording)
{
    if (!assignIfChanged(m_settings.autoStopRecording, autoStopRecording))
        return;
    save();
    emit autoStopRecordingChanged();
}

bool SettingsApi::autoCalibrate() const
{
    return m_settings.autoCalibrate;
}

void SettingsApi::setAutoCalibrate(bool autoCalibrate)
{
    if (!assignIfChanged(m_settings.autoCalibrate, autoCalibrate))
        return;
    save();
    emit autoCalibrateChanged();
}

int SettingsApi::vadCalibrationDurationMs() const
{
    return m_settings.vadCalibrationDurationMs;
}

void SettingsApi::setVadCalibrationDurationMs(int vadCalibrationDurationMs)
{
    if (!assignIfChanged(m_settings.vadCalibrationDurationMs, vadCalibrationDurationMs))
        return;
    save();
    emit vadCalibrationDurationMsChanged();
}

int SettingsApi::autoStopSilenceDuration() const
{
    return m_settings.autoStopSilenceDuration;
}

void SettingsApi::setAutoStopSilenceDuration(int autoStopSilenceDuration)
{
    if (!assignIfChanged(m_settings.autoStopSilenceDuration, autoStopSilenceDuration))
        return;
    save();
    emit autoStopSilenceDurationChanged();
}

int SettingsApi::vadMethod() const
{
    return m_settings.vadMethod;
}

void SettingsApi::setVadMethod(int method)
{
    if (!assignIfChanged(m_settings.vadMethod, method))
        return;
    save();
    emit vadMethodChanged();
}

double SettingsApi::vadThreshold() const
{
    return m_settings.vadThreshold;
}

void SettingsApi::setVadThreshold(double vadThreshold)
{
    if (!assignIfChanged(m_settings.vadThreshold, vadThreshold))
        return;
    save();
    emit vadThresholdChanged();
}

double SettingsApi::autoCorrThreshold() const
{
    return m_settings.autoCorrThreshold;
}

void SettingsApi::setAutoCorrThreshold(double threshold)
{
    if (!assignIfChanged(m_settings.autoCorrThreshold, threshold))
        return;
    save();
    emit autoCorrThresholdChanged();
}

double SettingsApi::autoCorrThresholdK() const
{
    return m_settings.autoCorrThresholdK;
}

void SettingsApi::setAutoCorrThresholdK(double thresholdK)
{
    if (!assignIfChanged(m_settings.autoCorrThresholdK, thresholdK))
        return;
    save();
    emit autoCorrThresholdKChanged();
}

double SettingsApi::autoCorrMinF0() const
{
    return m_settings.autoCorrMinF0;
}

void SettingsApi::setAutoCorrMinF0(double minF0)
{
    if (!assignIfChanged(m_settings.autoCorrMinF0, minF0))
        return;
    save();
    emit autoCorrMinF0Changed();
}

double SettingsApi::autoCorrMaxF0() const
{
    return m_settings.autoCorrMaxF0;
}

void SettingsApi::setAutoCorrMaxF0(double maxF0)
{
    if (!assignIfChanged(m_settings.autoCorrMaxF0, maxF0))
        return;
    save();
    emit autoCorrMaxF0Changed();
}

bool SettingsApi::advanced() const
{
    return m_advanced;
}

void SettingsApi::setAdvanced(bool advanced)
{
    if (m_advanced == advanced)
        return;
    m_advanced = advanced;
    emit advancedChanged();
}

int SettingsApi::shortestPhraseSec() const
{
    return m_settings.minRecordingTimeMs / 1000;
}

void SettingsApi::setShortestPhraseSec(int seconds)
{
    if (seconds < 0)
        seconds = 0;
    const int ms = seconds * 1000;
    const bool minChanged = assignIfChanged(m_settings.minRecordingTimeMs, ms);
    bool maxChanged = false;
    if (m_settings.maxRecordingTimeMs <= m_settings.minRecordingTimeMs) {
        maxChanged = assignIfChanged(m_settings.maxRecordingTimeMs, m_settings.minRecordingTimeMs + 1000);
    }
    if (!minChanged && !maxChanged)
        return;
    save();
    if (minChanged)
        emit shortestPhraseSecChanged();
    if (maxChanged)
        emit longestPhraseSecChanged();
}

int SettingsApi::longestPhraseSec() const
{
    return m_settings.maxRecordingTimeMs / 1000;
}

void SettingsApi::setLongestPhraseSec(int seconds)
{
    int ms = std::max(0, seconds) * 1000;
    if (ms <= m_settings.minRecordingTimeMs)
        ms = m_settings.minRecordingTimeMs + 1000;
    if (!assignIfChanged(m_settings.maxRecordingTimeMs, ms))
        return;
    save();
    emit longestPhraseSecChanged();
}

int SettingsApi::pauseSec() const
{
    return m_settings.autoStopSilenceDuration / 1000;
}

void SettingsApi::setPauseSec(int seconds)
{
    if (seconds < 0)
        seconds = 0;
    if (!assignIfChanged(m_settings.autoStopSilenceDuration, seconds * 1000))
        return;
    save();
    emit pauseSecChanged();
}

double SettingsApi::slowWpm() const
{
    return m_settings.speechRateMin;
}

void SettingsApi::setSlowWpm(double wpm)
{
    const bool rateChanged = assignIfChanged(m_settings.speechRateMin, wpm);
    const bool articulationChanged = assignIfChanged(m_settings.articulationMin, wpm);
    if (!rateChanged && !articulationChanged)
        return;
    save();
    emit slowWpmChanged();
}

double SettingsApi::fastWpm() const
{
    return m_settings.speechRateMax;
}

void SettingsApi::setFastWpm(double wpm)
{
    const bool rateChanged = assignIfChanged(m_settings.speechRateMax, wpm);
    const bool articulationChanged = assignIfChanged(m_settings.articulationMax, wpm);
    if (!rateChanged && !articulationChanged)
        return;
    save();
    emit fastWpmChanged();
}

int SettingsApi::meanValueDegry() const
{
    return m_settings.meanValueDegry;
}

void SettingsApi::setMeanValueDegry(int degree)
{
    if (!assignIfChanged(m_settings.meanValueDegry, degree))
        return;
    save();
    emit meanValueDegryChanged();
}

double SettingsApi::k1() const { return m_settings.speechRateK1; }
void SettingsApi::setK1(double value)
{
    if (!assignIfChanged(m_settings.speechRateK1, value))
        return;
    save();
    emit k1Changed();
}

double SettingsApi::k2() const { return m_settings.articulationK2; }
void SettingsApi::setK2(double value)
{
    if (!assignIfChanged(m_settings.articulationK2, value))
        return;
    save();
    emit k2Changed();
}

double SettingsApi::k3() const { return m_settings.pausesK3; }
void SettingsApi::setK3(double value)
{
    if (!assignIfChanged(m_settings.pausesK3, value))
        return;
    save();
    emit k3Changed();
}

double SettingsApi::k4() const { return m_settings.fillerK4; }
void SettingsApi::setK4(double value)
{
    if (!assignIfChanged(m_settings.fillerK4, value))
        return;
    save();
    emit k4Changed();
}

int SettingsApi::intensityFrame() const { return m_settings.intensityFrame; }
void SettingsApi::setIntensityFrame(int value)
{
    if (!assignIfChanged(m_settings.intensityFrame, std::clamp(value, 0, 1024)))
        return;
    save();
    emit intensityFrameChanged();
}

int SettingsApi::intensityShift() const { return m_settings.intensityShift; }
void SettingsApi::setIntensityShift(int value)
{
    if (!assignIfChanged(m_settings.intensityShift, std::clamp(value, 0, 512)))
        return;
    save();
    emit intensityShiftChanged();
}

int SettingsApi::intensitySmooth() const { return m_settings.intensitySmooth; }
void SettingsApi::setIntensitySmooth(int value)
{
    if (!assignIfChanged(m_settings.intensitySmooth, std::clamp(value, 0, 1024)))
        return;
    save();
    emit intensitySmoothChanged();
}

int SettingsApi::segmentMinLengthMs() const { return m_settings.segmentMinLengthMs; }
void SettingsApi::setSegmentMinLengthMs(int value)
{
    if (!assignIfChanged(m_settings.segmentMinLengthMs, std::clamp(value, 0, 2000)))
        return;
    save();
    emit segmentMinLengthMsChanged();
}

double SettingsApi::fillerMin() const { return m_settings.fillerMin; }
void SettingsApi::setFillerMin(double value)
{
    if (!assignIfChanged(m_settings.fillerMin, value))
        return;
    save();
    emit fillerMinChanged();
}

double SettingsApi::fillerMax() const { return m_settings.fillerMax; }
void SettingsApi::setFillerMax(double value)
{
    if (!assignIfChanged(m_settings.fillerMax, value))
        return;
    save();
    emit fillerMaxChanged();
}

int SettingsApi::metricAverageCount() const
{
    return m_settings.metricAverageCount;
}

void SettingsApi::setMetricAverageCount(int count)
{
    if (!assignIfChanged(m_settings.metricAverageCount, std::clamp(count, 1, 30)))
        return;
    save();
    emit metricAverageCountChanged();
}

void SettingsApi::load()
{
    m_settings = Settings::loadSettings();
    updateTranslator();
    applyFont();
    emit languageChanged();
    emit languageTitleChanged();
    emit themeChanged();
    emit fontSizeMultiplierChanged();
    emit primaryColorChanged();
    emit showNavigationMenuChanged();
    emit autoStopRecordingChanged();
    emit autoCalibrateChanged();
    emit vadCalibrationDurationMsChanged();
    emit autoStopSilenceDurationChanged();
    emit vadMethodChanged();
    emit vadThresholdChanged();
    emit autoCorrThresholdChanged();
    emit autoCorrThresholdKChanged();
    emit autoCorrMinF0Changed();
    emit autoCorrMaxF0Changed();
    emit shortestPhraseSecChanged();
    emit longestPhraseSecChanged();
    emit pauseSecChanged();
    emit slowWpmChanged();
    emit fastWpmChanged();
    emit meanValueDegryChanged();
    emit k1Changed();
    emit k2Changed();
    emit k3Changed();
    emit k4Changed();
    emit intensityFrameChanged();
    emit intensityShiftChanged();
    emit intensitySmoothChanged();
    emit segmentMinLengthMsChanged();
    emit fillerMinChanged();
    emit fillerMaxChanged();
    emit metricAverageCountChanged();
    emit settingsChanged();
}

void SettingsApi::save()
{
    Settings::saveSettings(m_settings);
    emit settingsChanged();
}

void SettingsApi::applyFont()
{
    QFont appFont = QGuiApplication::font();
    appFont.setPixelSize(qRound(14 * m_settings.fontSizeMultiplier));
    QGuiApplication::setFont(appFont);
}

void SettingsApi::updateTranslator()
{
    QCoreApplication::removeTranslator(&m_translator);
    const QString lang = QString::fromStdString(m_settings.language);
    if (m_translator.load(QStringLiteral("speech-rate-meter-2_") + lang,
            QCoreApplication::applicationDirPath())) {
        QCoreApplication::installTranslator(&m_translator);
        if (QQmlEngine* engine = qmlEngine(this))
            engine->retranslate();
    } else {
        LOG_WARNING() << "Failed to load translation for" << lang;
    }
}
