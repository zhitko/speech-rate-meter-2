#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include "logger.h"

AppSettings
Settings::getDefaultSettings()
{
    return AppSettings();
}

QString
Settings::getAppDataDir()
{
#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    return QCoreApplication::applicationDirPath();
#endif
}

QString
Settings::getSettingsFilePath()
{
#ifdef Q_OS_ANDROID
    QString path = getAppDataDir() + "/settings.ini";
    QDir().mkpath(QFileInfo(path).absolutePath());
    return path;
#else
    return getAppDataDir() + "/settings.ini";
#endif
}

AppSettings
Settings::loadSettings()
{
    AppSettings settings;
    QString absolutePath = QFileInfo(getSettingsFilePath()).absoluteFilePath();
    QSettings qsettings(absolutePath, QSettings::IniFormat);
    LOG_INFO() << "Loading settings from:" << absolutePath;

    qsettings.beginGroup("General");
    settings.language = qsettings.value("language", QString("ru")).toString().toStdString();
    settings.languageTitle = qsettings.value("languageTitle", QString("Русский")).toString().toStdString();
    settings.theme = qsettings.value("theme", QString("light")).toString().toStdString();
    settings.fontSizeMultiplier = qsettings.value("fontSizeMultiplier", 1.0).toDouble();
    settings.primaryColor = qsettings.value("primaryColor", QString("blue")).toString().toStdString();
    settings.showNavigationMenu = qsettings.value("showNavigationMenu", false).toBool();
    settings.autoStopRecording = qsettings.value("autoStopRecording", true).toBool();
    settings.autoCalibrate = qsettings.value("autoCalibrate", false).toBool();
    settings.vadCalibrationDurationMs = qsettings.value("vadCalibrationDurationMs", 2000).toInt();
    settings.autoStopSilenceDuration = qsettings.value("autoStopSilenceDuration", 2000).toInt();
    settings.vadMethod = qsettings.value("vadMethod", 0).toInt();
    settings.vadThreshold = qsettings.value("vadThreshold", 10000.0).toDouble();
    settings.autoCorrThreshold = qsettings.value("autoCorrThreshold", 0.3).toDouble();
    settings.autoCorrThresholdK = qsettings.value("autoCorrThresholdK", 1.0).toDouble();
    settings.autoCorrMinF0 = qsettings.value("autoCorrMinF0", 80.0).toDouble();
    settings.autoCorrMaxF0 = qsettings.value("autoCorrMaxF0", 300.0).toDouble();
    settings.autoCorrEnergyThreshold = qsettings.value("autoCorrEnergyThreshold", 0.02).toDouble();
    qsettings.endGroup();

    return settings;
}

void Settings::saveSettings(const AppSettings& settings)
{
    QString absolutePath = QFileInfo(getSettingsFilePath()).absoluteFilePath();
    QSettings qsettings(absolutePath, QSettings::IniFormat);
    LOG_INFO() << "Saving settings to:" << absolutePath;

    qsettings.beginGroup("General");
    qsettings.setValue("language", QString::fromStdString(settings.language));
    qsettings.setValue("languageTitle", QString::fromStdString(settings.languageTitle));
    qsettings.setValue("theme", QString::fromStdString(settings.theme));
    qsettings.setValue("fontSizeMultiplier", settings.fontSizeMultiplier);
    qsettings.setValue("primaryColor", QString::fromStdString(settings.primaryColor));
    qsettings.setValue("showNavigationMenu", settings.showNavigationMenu);
    qsettings.setValue("autoStopRecording", settings.autoStopRecording);
    qsettings.setValue("autoCalibrate", settings.autoCalibrate);
    qsettings.setValue("vadCalibrationDurationMs", settings.vadCalibrationDurationMs);
    qsettings.setValue("autoStopSilenceDuration", settings.autoStopSilenceDuration);
    qsettings.setValue("vadMethod", settings.vadMethod);
    qsettings.setValue("vadThreshold", settings.vadThreshold);
    qsettings.setValue("autoCorrThreshold", settings.autoCorrThreshold);
    qsettings.setValue("autoCorrThresholdK", settings.autoCorrThresholdK);
    qsettings.setValue("autoCorrMinF0", settings.autoCorrMinF0);
    qsettings.setValue("autoCorrMaxF0", settings.autoCorrMaxF0);
    qsettings.setValue("autoCorrEnergyThreshold", settings.autoCorrEnergyThreshold);
    qsettings.endGroup();

    qsettings.sync();
    if (qsettings.status() != QSettings::NoError) {
        LOG_WARNING() << "Failed to save settings to:" << absolutePath;
    }
}
