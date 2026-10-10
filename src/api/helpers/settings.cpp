#include "settings.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>

#include "logger.h"

namespace {

QMutex& settingsMutex()
{
    static QMutex mutex;
    return mutex;
}

QMutex& appDataMutex()
{
    static QMutex mutex;
    return mutex;
}

QString& appDataOverride()
{
    static QString path;
    return path;
}

void seedBundledSettings()
{
    if (qgetenv("APPIMAGE").isEmpty())
        return;

    const QString dest = Settings::getAppDataDir() + QStringLiteral("/settings.ini");
    if (QFile::exists(dest))
        return;

    const QString bundled = QCoreApplication::applicationDirPath()
        + QStringLiteral("/settings.ini");
    if (!QFile::exists(bundled))
        return;
    if (QFileInfo(bundled).absoluteFilePath() == QFileInfo(dest).absoluteFilePath())
        return;

    QDir().mkpath(QFileInfo(dest).absolutePath());
    if (!QFile::copy(bundled, dest)) {
        LOG_WARNING() << "Failed to copy default settings to" << dest;
        return;
    }
    QFile::setPermissions(dest,
        QFile::permissions(dest) | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

} // namespace

AppSettings
Settings::getDefaultSettings()
{
    return AppSettings();
}

QString
Settings::getAppDataDir()
{
    {
        QMutexLocker lock(&appDataMutex());
        if (!appDataOverride().isEmpty())
            return appDataOverride();
    }
#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    const QByteArray appImage = qgetenv("APPIMAGE");
    if (!appImage.isEmpty()) {
        const QString portableDir = QFileInfo(QString::fromLocal8Bit(appImage)).absolutePath();
        if (QFileInfo(portableDir).isWritable())
            return portableDir;
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    return QCoreApplication::applicationDirPath();
#endif
}

void Settings::setAppDataDirForTests(const QString& path)
{
    QMutexLocker lock(&appDataMutex());
    appDataOverride() = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

void Settings::clearAppDataDirForTests()
{
    QMutexLocker lock(&appDataMutex());
    appDataOverride().clear();
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
    QMutexLocker lock(&settingsMutex());
    seedBundledSettings();
    AppSettings settings;
    QString absolutePath = QFileInfo(getSettingsFilePath()).absoluteFilePath();
    QSettings qsettings(absolutePath, QSettings::IniFormat);

    // The file is applied only after a save has written date_v3.
    if (!qsettings.contains(QStringLiteral("date_v3")))
        return settings;

    qsettings.beginGroup("General");
    settings.language = qsettings.value("language", QString("ru")).toString().toStdString();
    settings.languageTitle = qsettings.value("languageTitle", QString("Русский")).toString().toStdString();
    settings.theme = qsettings.value("theme", QString("light")).toString().toStdString();
    settings.fontSizeMultiplier = qsettings.value("fontSizeMultiplier", 1.0).toDouble();
    settings.primaryColor = qsettings.value("primaryColor", QString("blue")).toString().toStdString();
    settings.showNavigationMenu = qsettings.value("showNavigationMenu", false).toBool();
    settings.autoStopRecording = qsettings.value("autoStopRecording", true).toBool();
    settings.autoCalibrate = qsettings.value("autoCalibrate", false).toBool();
    settings.keepRecordingFiles = qsettings.value("keepRecordingFiles", false).toBool();
    settings.vadCalibrationDurationMs = qsettings.value("vadCalibrationDurationMs", 2000).toInt();
    settings.autoStopSilenceDuration = qsettings.value("autoStopSilenceDuration", 2000).toInt();
    settings.phrasalPauseMs = std::clamp(qsettings.value("phrasalPauseMs", 150).toInt(), 50, 2000);
    settings.analysisWindowSec = std::clamp(qsettings.value("analysisWindowSec", 10).toInt(), 3, 30);
    settings.updatesPerMinute = std::clamp(qsettings.value("updatesPerMinute", 60).toInt(), 6, 240);
    settings.gaugeAverageCount = std::clamp(qsettings.value("gaugeAverageCount", 3).toInt(), 1, 30);
    settings.gaugeMode = std::clamp(qsettings.value("gaugeMode", 0).toInt(), 0, 2);
    settings.vadMethod = qsettings.value("vadMethod", 0).toInt();
    settings.vadThreshold = qsettings.value("vadThreshold", 10000.0).toDouble();
    settings.autoCorrThreshold = qsettings.value("autoCorrThreshold", 0.3).toDouble();
    settings.autoCorrThresholdK = qsettings.value("autoCorrThresholdK", 1.0).toDouble();
    settings.autoCorrMinF0 = qsettings.value("autoCorrMinF0", 80.0).toDouble();
    settings.autoCorrMaxF0 = qsettings.value("autoCorrMaxF0", 300.0).toDouble();
    settings.autoCorrEnergyThreshold = qsettings.value("autoCorrEnergyThreshold", 0.02).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("speechRate");
    settings.meanValueDegry = qsettings.value("MeanValueDegry", 3).toInt();
    settings.speechRateK1 = qsettings.value("K1", 0.71).toDouble();
    settings.speechRateMin = qsettings.value("Min", 70).toDouble();
    settings.speechRateMax = qsettings.value("Max", 210).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("articulationRate");
    settings.articulationK2 = qsettings.value("K2", 1.2).toDouble();
    settings.articulationMin = qsettings.value("Min", settings.speechRateMin).toDouble();
    settings.articulationMax = qsettings.value("Max", settings.speechRateMax).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("meanPauses");
    settings.pausesK3 = qsettings.value("Max", 0.30).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("intensity");
    settings.intensityFrame = qsettings.value("frame", 240).toInt();
    settings.intensityShift = qsettings.value("shift", 120).toInt();
    settings.intensitySmooth = qsettings.value("smoothFrame", 120).toInt();
    qsettings.endGroup();

    qsettings.beginGroup("segmentsByIntensity");
    settings.segmentMinLengthMs = qsettings.value("minimumLength", 5).toInt();
    qsettings.endGroup();

    qsettings.beginGroup("fillerSounds");
    settings.fillerK4 = qsettings.value("K4", 100).toDouble();
    settings.fillerMin = qsettings.value("Min", 120).toDouble();
    settings.fillerMax = qsettings.value("Max", 240).toDouble();
    qsettings.endGroup();

    return settings;
}

void Settings::saveSettings(const AppSettings& settings)
{
    QMutexLocker lock(&settingsMutex());
    QString absolutePath = QFileInfo(getSettingsFilePath()).absoluteFilePath();
    QSettings qsettings(absolutePath, QSettings::IniFormat);
    LOG_INFO() << "Saving settings to:" << absolutePath;

    qsettings.setValue(QStringLiteral("date_v3"), QDate());

    qsettings.beginGroup("General");
    qsettings.setValue("language", QString::fromStdString(settings.language));
    qsettings.setValue("languageTitle", QString::fromStdString(settings.languageTitle));
    qsettings.setValue("theme", QString::fromStdString(settings.theme));
    qsettings.setValue("fontSizeMultiplier", settings.fontSizeMultiplier);
    qsettings.setValue("primaryColor", QString::fromStdString(settings.primaryColor));
    qsettings.setValue("showNavigationMenu", settings.showNavigationMenu);
    qsettings.remove("metricAverageCount");
    qsettings.remove("minRecordingTimeMs");
    qsettings.remove("maxRecordingTimeMs");
    qsettings.setValue("autoStopRecording", settings.autoStopRecording);
    qsettings.setValue("autoCalibrate", settings.autoCalibrate);
    qsettings.setValue("keepRecordingFiles", settings.keepRecordingFiles);
    qsettings.setValue("vadCalibrationDurationMs", settings.vadCalibrationDurationMs);
    qsettings.setValue("autoStopSilenceDuration", settings.autoStopSilenceDuration);
    qsettings.setValue("phrasalPauseMs", settings.phrasalPauseMs);
    qsettings.setValue("analysisWindowSec", settings.analysisWindowSec);
    qsettings.setValue("updatesPerMinute", settings.updatesPerMinute);
    qsettings.setValue("gaugeAverageCount", settings.gaugeAverageCount);
    qsettings.setValue("gaugeMode", settings.gaugeMode);
    qsettings.setValue("vadMethod", settings.vadMethod);
    qsettings.setValue("vadThreshold", settings.vadThreshold);
    qsettings.setValue("autoCorrThreshold", settings.autoCorrThreshold);
    qsettings.setValue("autoCorrThresholdK", settings.autoCorrThresholdK);
    qsettings.setValue("autoCorrMinF0", settings.autoCorrMinF0);
    qsettings.setValue("autoCorrMaxF0", settings.autoCorrMaxF0);
    qsettings.setValue("autoCorrEnergyThreshold", settings.autoCorrEnergyThreshold);
    qsettings.endGroup();

    qsettings.beginGroup("speechRate");
    qsettings.setValue("MeanValueDegry", settings.meanValueDegry);
    qsettings.setValue("K1", settings.speechRateK1);
    qsettings.setValue("Min", settings.speechRateMin);
    qsettings.setValue("Max", settings.speechRateMax);
    qsettings.endGroup();

    qsettings.beginGroup("articulationRate");
    qsettings.setValue("K2", settings.articulationK2);
    qsettings.setValue("Min", settings.articulationMin);
    qsettings.setValue("Max", settings.articulationMax);
    qsettings.endGroup();

    qsettings.beginGroup("meanPauses");
    qsettings.setValue("Max", settings.pausesK3);
    qsettings.endGroup();

    qsettings.beginGroup("intensity");
    qsettings.setValue("frame", settings.intensityFrame);
    qsettings.setValue("shift", settings.intensityShift);
    qsettings.setValue("smoothFrame", settings.intensitySmooth);
    qsettings.endGroup();

    qsettings.beginGroup("segmentsByIntensity");
    qsettings.setValue("minimumLength", settings.segmentMinLengthMs);
    qsettings.endGroup();

    qsettings.beginGroup("fillerSounds");
    qsettings.setValue("K4", settings.fillerK4);
    qsettings.setValue("Min", settings.fillerMin);
    qsettings.setValue("Max", settings.fillerMax);
    qsettings.endGroup();

    qsettings.sync();
    if (qsettings.status() != QSettings::NoError) {
        LOG_WARNING() << "Failed to save settings to:" << absolutePath;
    }
}
