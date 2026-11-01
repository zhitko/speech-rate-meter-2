#include "settings.h"
#include "speechrateanalysis.h"

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

// Settings saved by the previous release stored this as the untouched default.
constexpr int kLegacyDefaultPhrasalPauseMs = 150;

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

void seedBundledFile(const QString& fileName)
{
    if (qgetenv("APPIMAGE").isEmpty())
        return;

    const QString dest = Settings::getAppDataDir() + QLatin1Char('/') + fileName;
    if (QFile::exists(dest))
        return;

    const QString bundled = QCoreApplication::applicationDirPath()
        + QLatin1Char('/') + fileName;
    if (!QFile::exists(bundled))
        return;
    if (QFileInfo(bundled).absoluteFilePath() == QFileInfo(dest).absoluteFilePath())
        return;

    QDir().mkpath(QFileInfo(dest).absolutePath());
    if (!QFile::copy(bundled, dest)) {
        LOG_WARNING() << "Failed to copy" << fileName << "to" << dest;
        return;
    }
    QFile::setPermissions(dest,
        QFile::permissions(dest) | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void seedBundledSettings()
{
    seedBundledFile(QStringLiteral("settings.ini"));
    seedBundledFile(QStringLiteral("settings_fast.ini"));
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

namespace {

// writeMigrations stores one-time INI fixes. Preset files are left untouched.
bool readSettingsFile(QSettings& qsettings, AppSettings* out, bool writeMigrations)
{
    if (!out || !qsettings.contains(QStringLiteral("date_v3")))
        return false;

    AppSettings settings;

    qsettings.beginGroup("General");
    settings.language = qsettings.value("language", QString("ru")).toString().toStdString();
    settings.languageTitle = qsettings.value("languageTitle", QString("Русский")).toString().toStdString();
    settings.theme = qsettings.value("theme", QString("light")).toString().toStdString();
    settings.fontSizeMultiplier = qsettings.value("fontSizeMultiplier", 1.0).toDouble();
    settings.primaryColor = qsettings.value("primaryColor", QString("blue")).toString().toStdString();
    settings.showNavigationMenu = qsettings.value("showNavigationMenu", false).toBool();
    settings.preset = qsettings.value(QStringLiteral("preset"), 0).toInt() == 1 ? 1 : 0;
    settings.autoStopRecording = qsettings.value("autoStopRecording", true).toBool();
    settings.autoCalibrate = qsettings.value("autoCalibrate", false).toBool();
    settings.keepRecordingFiles = qsettings.value("keepRecordingFiles", false).toBool();
    settings.vadCalibrationDurationMs = qsettings.value("vadCalibrationDurationMs", 2000).toInt();
    settings.autoStopSilenceDuration = qsettings.value("autoStopSilenceDuration", 2000).toInt();
    settings.phrasalPauseMs = std::clamp(
        qsettings.value(QStringLiteral("phrasalPauseMs"), speechrate::kDefaultPhrasalPauseMs).toInt(),
        50,
        2000);
    // A stored 150 is the old default, not a custom choice, until this flag is set.
    if (!qsettings.value(QStringLiteral("phrasalPauseMsMigrated200"), false).toBool()) {
        if (qsettings.contains(QStringLiteral("phrasalPauseMs"))
            && settings.phrasalPauseMs == kLegacyDefaultPhrasalPauseMs) {
            settings.phrasalPauseMs = speechrate::kDefaultPhrasalPauseMs;
        }
        if (writeMigrations) {
            qsettings.setValue(QStringLiteral("phrasalPauseMs"), settings.phrasalPauseMs);
            qsettings.setValue(QStringLiteral("phrasalPauseMsMigrated200"), true);
        }
    }
    settings.articulationMethod = std::clamp(qsettings.value("articulationMethod", 0).toInt(), 0, 2);
    settings.speechRateMethod = std::clamp(qsettings.value("speechRateMethod", 0).toInt(), 0, 1);
    settings.summaryMethod = std::clamp(
        qsettings.value(QStringLiteral("summaryMethod"), static_cast<int>(speechrate::SummaryMethod::Median)).toInt(),
        0,
        speechrate::kSummaryMethodCount - 1);
    // A stored 0 is the previous default, not a custom choice, until this flag is set.
    if (!qsettings.value(QStringLiteral("summaryMethodMedianDefault"), false).toBool()) {
        if (!qsettings.contains(QStringLiteral("summaryMethod"))
            || settings.summaryMethod == static_cast<int>(speechrate::SummaryMethod::WeightedMean)) {
            settings.summaryMethod = static_cast<int>(speechrate::SummaryMethod::Median);
        }
        if (writeMigrations) {
            qsettings.setValue(QStringLiteral("summaryMethod"), settings.summaryMethod);
            qsettings.setValue(QStringLiteral("summaryMethodMedianDefault"), true);
        }
    }
    settings.analysisWindowSec = clampAnalysisWindowSec(qsettings.value("analysisWindowSec", 10).toInt());
    settings.updatesPerMinute = clampUpdatesPerMinute(qsettings.value("updatesPerMinute", 60).toInt());
    settings.tempoChartWindowSec = std::clamp(qsettings.value("tempoChartWindowSec", 300).toInt(), 30, 1800);
    // General/recordingView (0 chart, 1 arcs) used to pick the live Home view.
    // Home now always shows both. Read the key so an older file still loads,
    // then drop it on the next save.
    (void)qsettings.value(QStringLiteral("recordingView"));
    settings.chartUseAllReadings = qsettings.value("chartUseAllReadings", true).toBool();
    settings.gaugeAverageCount = std::clamp(qsettings.value("gaugeAverageCount", 3).toInt(), 1, 30);
    settings.gaugeMode = std::clamp(qsettings.value("gaugeMode", 0).toInt(), 0, 2);
    settings.showSpeechRateTile = qsettings.value("showSpeechRateTile", true).toBool();
    settings.showArticulationRateTile = qsettings.value("showArticulationRateTile", true).toBool();
    settings.showFillersTile = qsettings.value("showFillersTile", false).toBool();
    settings.showPausesTile = qsettings.value("showPausesTile", true).toBool();
    settings.showSpeechTile = qsettings.value("showSpeechTile", true).toBool();
    settings.showWholeRecordingTile = qsettings.value("showWholeRecordingTile", true).toBool();
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
    settings.speechRateK1 = qsettings.value("K1", 0.41).toDouble();
    settings.speechRateMin = qsettings.value("Min", 70).toDouble();
    settings.speechRateMax = qsettings.value("Max", 210).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("articulationRate");
    settings.articulationK2 = qsettings.value("K2", 1.2).toDouble();
    settings.windowK5 = qsettings.value("K5", 0.71).toDouble();
    settings.articulationMin = qsettings.value("Min", settings.speechRateMin).toDouble();
    settings.articulationMax = qsettings.value("Max", settings.speechRateMax).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("meanPauses");
    settings.pausesK3 = qsettings.value("Max", 0.30).toDouble();
    qsettings.endGroup();

    qsettings.beginGroup("intensity");
    settings.intensityFrame = qsettings.value("frame", 64).toInt();
    settings.intensityShift = qsettings.value("shift", 32).toInt();
    settings.intensitySmooth = qsettings.value("smoothFrame", 64).toInt();
    qsettings.endGroup();

    qsettings.beginGroup("segmentsByIntensity");
    settings.segmentMinLengthMs = std::clamp(qsettings.value("minimumLength", 8).toInt(), 0, 2000);
    qsettings.endGroup();

    qsettings.beginGroup("fillerSounds");
    settings.fillerK4 = qsettings.value("K4", 100).toDouble();
    settings.fillerMin = qsettings.value("Min", 120).toDouble();
    settings.fillerMax = qsettings.value("Max", 240).toDouble();
    qsettings.endGroup();

    *out = settings;
    return true;
}

QString fastPresetPath()
{
    const QString beside = Settings::getAppDataDir() + QStringLiteral("/settings_fast.ini");
    if (QFile::exists(beside))
        return beside;
    return QStringLiteral(":/presets/settings_fast.ini");
}

} // namespace

AppSettings
Settings::loadSettings()
{
    QMutexLocker lock(&settingsMutex());
    seedBundledSettings();
    AppSettings settings;
    const QString absolutePath = QFileInfo(getSettingsFilePath()).absoluteFilePath();
    QSettings qsettings(absolutePath, QSettings::IniFormat);
    if (!readSettingsFile(qsettings, &settings, true))
        return AppSettings();
    return settings;
}

bool Settings::readPreset(int preset, AppSettings* settings)
{
    QMutexLocker lock(&settingsMutex());
    // The live settings.ini is rewritten whenever a control changes, so the
    // standard preset is the copy compiled into the program.
    const QString path = preset == 1
        ? fastPresetPath()
        : QStringLiteral(":/presets/settings.ini");
    QSettings qsettings(path, QSettings::IniFormat);
    return readSettingsFile(qsettings, settings, false);
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
    qsettings.setValue(QStringLiteral("preset"), settings.preset == 1 ? 1 : 0);
    qsettings.remove("metricAverageCount");
    qsettings.remove("minRecordingTimeMs");
    qsettings.remove("maxRecordingTimeMs");
    qsettings.setValue("autoStopRecording", settings.autoStopRecording);
    qsettings.setValue("autoCalibrate", settings.autoCalibrate);
    qsettings.setValue("keepRecordingFiles", settings.keepRecordingFiles);
    qsettings.setValue("vadCalibrationDurationMs", settings.vadCalibrationDurationMs);
    qsettings.setValue("autoStopSilenceDuration", settings.autoStopSilenceDuration);
    qsettings.setValue("phrasalPauseMs", settings.phrasalPauseMs);
    qsettings.setValue(QStringLiteral("phrasalPauseMsMigrated200"), true);
    qsettings.setValue("articulationMethod", settings.articulationMethod);
    qsettings.setValue("speechRateMethod", settings.speechRateMethod);
    qsettings.setValue(QStringLiteral("summaryMethod"),
        std::clamp(settings.summaryMethod, 0, speechrate::kSummaryMethodCount - 1));
    qsettings.setValue(QStringLiteral("summaryMethodMedianDefault"), true);
    qsettings.setValue("analysisWindowSec", settings.analysisWindowSec);
    qsettings.setValue("updatesPerMinute", settings.updatesPerMinute);
    qsettings.setValue("tempoChartWindowSec", settings.tempoChartWindowSec);
    qsettings.remove(QStringLiteral("recordingView"));
    qsettings.setValue("chartUseAllReadings", settings.chartUseAllReadings);
    qsettings.setValue("gaugeAverageCount", settings.gaugeAverageCount);
    qsettings.setValue("gaugeMode", settings.gaugeMode);
    qsettings.setValue("showSpeechRateTile", settings.showSpeechRateTile);
    qsettings.setValue("showArticulationRateTile", settings.showArticulationRateTile);
    qsettings.setValue("showFillersTile", settings.showFillersTile);
    qsettings.setValue("showPausesTile", settings.showPausesTile);
    qsettings.setValue("showSpeechTile", settings.showSpeechTile);
    qsettings.setValue("showWholeRecordingTile", settings.showWholeRecordingTile);
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
    qsettings.setValue("K5", settings.windowK5);
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
