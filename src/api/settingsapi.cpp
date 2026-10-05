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
    emit settingsChanged();
}

void SettingsApi::save()
{
    Settings::saveSettings(m_settings);
    emit settingsChanged();
}

void SettingsApi::clearUserData()
{
    const QString recordsPath = Settings::getAppDataDir() + QStringLiteral("/data/records");
    QDir recordsDir(recordsPath);
    if (!recordsDir.exists())
        return;

    const QFileInfoList files = recordsDir.entryInfoList(QDir::Files);
    for (const QFileInfo& fileInfo : files) {
        if (!QFile::remove(fileInfo.absoluteFilePath()))
            LOG_WARNING() << "Failed to delete recording:" << fileInfo.absoluteFilePath();
    }

    const QFileInfoList dirs = recordsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& dirInfo : dirs) {
        QDir(dirInfo.absoluteFilePath()).removeRecursively();
    }
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
