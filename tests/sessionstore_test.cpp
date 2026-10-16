#include "src/api/helpers/settings.h"
#include "src/services/sessionstore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

QVariantMap measuredSegment()
{
    speechrate::Parts parts;
    parts.vowelLengths = { 4, 10 };
    parts.gapLengths = { 40 };
    parts.speechDuration = 2.0;
    parts.vowelMaxFrames = 10;
    parts.gapMaxFrames = 40;

    speechrate::Config config;
    const speechrate::Metrics metrics = speechrate::metricsFromLengths(
        parts.vowelLengths,
        parts.gapLengths,
        parts.speechDuration,
        config,
        parts.vowelMaxFrames,
        parts.gapMaxFrames);

    QVariantMap segment;
    segment.insert(QStringLiteral("startedAt"), QStringLiteral("2026-10-06T23:00:01.000"));
    segment.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-06T23:00:03.000"));
    segment.insert(QStringLiteral("speechRate"), metrics.speechRate);
    segment.insert(QStringLiteral("articulationRate"), metrics.articulationRate);
    segment.insert(QStringLiteral("phrasePauses"), metrics.phrasePauses);
    segment.insert(QStringLiteral("speechDuration"), metrics.speechDuration);
    segment.insert(QStringLiteral("fillerPercent"), 12);
    SessionStore::insertMeasurement(segment, parts, config, 120, 240);
    return segment;
}

void testRoundTripAndClear()
{
    const QString id = QStringLiteral("20261006-230000-000");
    const QString started = QStringLiteral("2026-10-06T23:00:00.000");
    expect(SessionStore::appendSegment(id, started, measuredSegment()), "append measured segment");

    QVariantMap shown;
    shown.insert(QStringLiteral("at"), QStringLiteral("2026-10-06T23:00:02.000"));
    shown.insert(QStringLiteral("speechRate"), 128);
    shown.insert(QStringLiteral("articulationRate"), 151);
    shown.insert(QStringLiteral("phrasePauses"), 0.18);
    shown.insert(QStringLiteral("speechDuration"), 2);
    shown.insert(QStringLiteral("fillerPercent"), 12);
    expect(SessionStore::appendShown(id, started, shown), "append shown metric");

    const QVariantList sessions = SessionStore::listSessions();
    expect(sessions.size() == 1, "one session listed");
    const QVariantMap loaded = SessionStore::loadSession(id);
    expect(!loaded.isEmpty(), "session loads");
    expect(loaded.value(QStringLiteral("segments")).toList().size() == 1, "shown point loads");
    expect(loaded.value(QStringLiteral("updateCount")).toInt() == 1, "update count retained");

    SessionStore::clearAll();
    expect(SessionStore::listSessions().isEmpty(), "clear removes sessions");
    expect(SessionStore::recordsAreEmpty(), "clear removes scratch records");
}

void testDurationWeightedSessionMean()
{
    AppSettings settings = Settings::getDefaultSettings();
    settings.summaryMethod = static_cast<int>(speechrate::SummaryMethod::WeightedMean);
    Settings::saveSettings(settings);

    const QString id = QStringLiteral("20261006-230100-000");
    const QString started = QStringLiteral("2026-10-06T23:01:00.000");
    QVariantMap first = measuredSegment();
    QVariantMap second = measuredSegment();
    second.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-06T23:01:08.000"));
    second.insert(QStringLiteral("speechDuration"), 4.0);
    second.insert(QStringLiteral("speechRate"), 180.0);
    second.insert(QStringLiteral("articulationRate"), 210.0);
    second.insert(QStringLiteral("phrasePauses"), 0.40);
    second.insert(QStringLiteral("fillerPercent"), 30);

    expect(SessionStore::appendSegment(id, started, first), "append first phrase");
    expect(SessionStore::appendSegment(id, started, second), "append second phrase");

    const QVariantMap loaded = SessionStore::loadSession(id);
    const double duration = 6.0;
    const auto near = [](double left, double right) {
        return std::fabs(left - right) < 1e-6;
    };
    expect(near(loaded.value(QStringLiteral("speechDuration")).toDouble(), duration),
        "speech is the sum");
    expect(near(loaded.value(QStringLiteral("speechRate")).toDouble(),
                (first.value(QStringLiteral("speechRate")).toDouble() * 2.0 + 180.0 * 4.0) / duration),
        "speech rate is the duration-weighted mean");
    expect(near(loaded.value(QStringLiteral("articulationRate")).toDouble(),
                (first.value(QStringLiteral("articulationRate")).toDouble() * 2.0 + 210.0 * 4.0) / duration),
        "articulation is the duration-weighted mean");
    expect(near(loaded.value(QStringLiteral("phrasePauses")).toDouble(),
                (first.value(QStringLiteral("phrasePauses")).toDouble() * 2.0 + 0.40 * 4.0) / duration),
        "pauses are the duration-weighted mean");
    expect(near(loaded.value(QStringLiteral("fillerPercent")).toDouble(), (12.0 * 2.0 + 30.0 * 4.0) / duration),
        "fillers are the duration-weighted mean");

    SessionStore::clearAll();
}

void setKeepRecordingFiles(bool keep)
{
    AppSettings settings = Settings::getDefaultSettings();
    settings.keepRecordingFiles = keep;
    Settings::saveSettings(settings);
}

int fileCount(const QString& filter)
{
    return QDir(SessionStore::recordsDir()).entryList({ filter }, QDir::Files).size();
}

void testCommitKeepsWavUntilClose()
{
    setKeepRecordingFiles(false);
    SessionStore::clearAll();
    const QString id = QStringLiteral("20261011-010000-000");
    const QString started = QStringLiteral("2026-10-11T01:00:00.000");
    const std::vector<float> samples { 0.f, 1000.f, -1000.f, 0.f };
    QString wavPath;
    expect(SessionStore::commitSegment(id, started, measuredSegment(), samples, &wavPath),
        "commit keeps wav for playback");
    expect(!wavPath.isEmpty(), "commit reports the wav path");
    expect(QFileInfo::exists(wavPath), "reported wav exists");
    expect(fileCount(QStringLiteral("*.wav")) == 1, "wav remains after analysis");
    expect(fileCount(QStringLiteral("*.pending.json")) == 0, "pending sidecar is removed");
    SessionStore::discardUnkeptRecordings();
    expect(fileCount(QStringLiteral("*.wav")) == 0, "close removes the wav when keep is off");
    expect(SessionStore::recordsAreEmpty(), "records dir is empty after close");
    SessionStore::clearAll();
}

void testCommitKeepsWavWhenEnabled()
{
    setKeepRecordingFiles(true);
    SessionStore::clearAll();
    const QString id = QStringLiteral("20261011-010100-000");
    const QString started = QStringLiteral("2026-10-11T01:01:00.000");
    const std::vector<float> samples { 0.f, 1000.f, -1000.f, 0.f };
    QVariantMap first = measuredSegment();
    first.insert(QStringLiteral("startedAt"), QStringLiteral("2026-10-11T01:01:01.000"));
    first.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-11T01:01:03.000"));
    expect(SessionStore::commitSegment(id, started, first, samples), "commit keeps wav");
    expect(fileCount(QStringLiteral("*.wav")) == 1, "kept wav remains");
    expect(fileCount(QStringLiteral("*.pending.json")) == 0, "pending sidecar is removed");

    QVariantMap second = first;
    second.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-11T01:01:08.000"));
    expect(SessionStore::commitSegment(id, started, second, samples),
        "second commit while wavs are kept");
    expect(fileCount(QStringLiteral("*.wav")) == 2, "second wav is kept too");
    expect(fileCount(QStringLiteral("*.pending.json")) == 0, "second pending sidecar is removed");
    SessionStore::discardUnkeptRecordings();
    expect(fileCount(QStringLiteral("*.wav")) == 2, "close leaves wavs while keep is on");

    setKeepRecordingFiles(false);
    SessionStore::discardUnkeptRecordings();
    expect(fileCount(QStringLiteral("*.wav")) == 0, "close removes wavs when keep is turned off");

    SessionStore::clearAll();
    expect(fileCount(QStringLiteral("*.wav")) == 0, "delete user data removes kept wavs");
    setKeepRecordingFiles(false);
}

void testDiscardSkipsUnsavedWav()
{
    setKeepRecordingFiles(false);
    SessionStore::clearAll();
    QDir().mkpath(SessionStore::recordsDir());
    const QString wav = SessionStore::recordsDir() + QStringLiteral("/pending-keep.wav");
    QFile audio(wav);
    expect(audio.open(QIODevice::WriteOnly), "create unsaved wav");
    audio.write("RIFF");
    audio.close();
    QFile side(wav + QStringLiteral(".pending.json"));
    expect(side.open(QIODevice::WriteOnly), "create pending sidecar");
    side.write("{}");
    side.close();

    SessionStore::discardUnkeptRecordings();
    expect(QFileInfo::exists(wav), "unsaved wav is kept");
    expect(QFile::remove(wav + QStringLiteral(".pending.json")), "remove sidecar");
    SessionStore::discardUnkeptRecordings();
    expect(!QFileInfo::exists(wav), "saved wav is removed on close");
    SessionStore::clearAll();
}

void testPhrasalPauseDefaultMigration()
{
    expect(Settings::getDefaultSettings().phrasalPauseMs == speechrate::kDefaultPhrasalPauseMs,
        "settings pause default matches the analysis default");
    expect(Settings::getDefaultSettings().phrasalPauseMs == 200, "in-code pause default is 200 ms");
    expect(Settings::getDefaultSettings().speechRateMethod == 0, "speech rate method defaults to direct");
    expect(Settings::getDefaultSettings().summaryMethod == static_cast<int>(speechrate::SummaryMethod::Median),
        "session summary defaults to the median");

    const QString path = Settings::getAppDataDir() + QStringLiteral("/settings.ini");
    auto rewritePause = [&path](int milliseconds, bool clearMigrationFlag) {
        QSettings stored(path, QSettings::IniFormat);
        stored.beginGroup(QStringLiteral("General"));
        stored.setValue(QStringLiteral("phrasalPauseMs"), milliseconds);
        if (clearMigrationFlag)
            stored.remove(QStringLiteral("phrasalPauseMsMigrated200"));
        stored.endGroup();
        stored.sync();
    };

    AppSettings custom = Settings::getDefaultSettings();
    custom.phrasalPauseMs = 180;
    Settings::saveSettings(custom);
    rewritePause(180, true);
    expect(Settings::loadSettings().phrasalPauseMs == 180, "a custom pause minimum is kept");

    rewritePause(150, true);
    expect(Settings::loadSettings().phrasalPauseMs == 200, "a stored 150 moves to 200");
    expect(Settings::loadSettings().phrasalPauseMs == 200, "the migrated 200 stays on the next load");

    AppSettings chosen = Settings::loadSettings();
    chosen.phrasalPauseMs = 150;
    Settings::saveSettings(chosen);
    expect(Settings::loadSettings().phrasalPauseMs == 150, "a pause minimum chosen after migration stays 150");

    rewritePause(250, true);
    expect(Settings::loadSettings().phrasalPauseMs == 250, "250 ms is left alone");

    {
        QSettings stored(path, QSettings::IniFormat);
        stored.beginGroup(QStringLiteral("General"));
        stored.remove(QStringLiteral("phrasalPauseMs"));
        stored.remove(QStringLiteral("phrasalPauseMsMigrated200"));
        stored.endGroup();
        stored.sync();
    }
    expect(Settings::loadSettings().phrasalPauseMs == 200, "a missing pause minimum uses 200");
}

void testSummaryMethodChangesHistory()
{
    AppSettings settings = Settings::getDefaultSettings();
    settings.summaryMethod = static_cast<int>(speechrate::SummaryMethod::Median);
    Settings::saveSettings(settings);

    const QString id = QStringLiteral("20261016-210000-000");
    const QString started = QStringLiteral("2026-10-16T21:00:00.000");
    QVariantMap first = measuredSegment();
    QVariantMap second = measuredSegment();
    second.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-16T21:00:08.000"));
    second.insert(QStringLiteral("speechDuration"), 4.0);
    second.insert(QStringLiteral("speechRate"), 180.0);
    second.insert(QStringLiteral("articulationRate"), 210.0);
    second.insert(QStringLiteral("phrasePauses"), 0.40);
    second.insert(QStringLiteral("fillerPercent"), 30);
    QVariantMap third = measuredSegment();
    third.insert(QStringLiteral("endedAt"), QStringLiteral("2026-10-16T21:00:12.000"));
    third.insert(QStringLiteral("speechDuration"), 2.0);
    third.insert(QStringLiteral("speechRate"), 90.0);
    third.insert(QStringLiteral("articulationRate"), 100.0);
    third.insert(QStringLiteral("phrasePauses"), 0.10);
    third.insert(QStringLiteral("fillerPercent"), 0);

    expect(SessionStore::appendSegment(id, started, first), "append first summary phrase");
    expect(SessionStore::appendSegment(id, started, second), "append second summary phrase");
    expect(SessionStore::appendSegment(id, started, third), "append third summary phrase");

    const QVariantMap loaded = SessionStore::loadSession(id);
    const auto near = [](double left, double right) {
        return std::fabs(left - right) < 1e-6;
    };
    const double firstRate = first.value(QStringLiteral("speechRate")).toDouble();
    expect(near(loaded.value(QStringLiteral("speechRate")).toDouble(), 90.0),
        "history speech rate follows the median setting");
    expect(firstRate < 90.0, "the first phrase sits below the median");
    std::vector<double> articulation {
        first.value(QStringLiteral("articulationRate")).toDouble(), 210.0, 100.0
    };
    std::sort(articulation.begin(), articulation.end());
    expect(near(loaded.value(QStringLiteral("articulationRate")).toDouble(), articulation[1]),
        "history articulation follows the median setting");
    expect(near(loaded.value(QStringLiteral("phrasePauses")).toDouble(), 0.10),
        "history pauses follow the median setting");
    expect(near(loaded.value(QStringLiteral("fillerPercent")).toDouble(), 12.0),
        "history fillers follow the median setting");
    expect(near(loaded.value(QStringLiteral("speechDuration")).toDouble(), 8.0),
        "speech duration stays the sum");

    const QString path = Settings::getAppDataDir() + QStringLiteral("/settings.ini");
    {
        QSettings stored(path, QSettings::IniFormat);
        stored.beginGroup(QStringLiteral("General"));
        stored.setValue(QStringLiteral("summaryMethod"), 99);
        stored.endGroup();
        stored.sync();
    }
    expect(Settings::loadSettings().summaryMethod == speechrate::kSummaryMethodCount - 1,
        "a stored summary past the last choice clamps to the last choice");

    {
        QSettings stored(path, QSettings::IniFormat);
        stored.beginGroup(QStringLiteral("General"));
        stored.setValue(QStringLiteral("summaryMethod"), static_cast<int>(speechrate::SummaryMethod::WeightedMean));
        stored.setValue(QStringLiteral("summaryMethodMedianDefault"), false);
        stored.endGroup();
        stored.sync();
    }
    expect(Settings::loadSettings().summaryMethod == static_cast<int>(speechrate::SummaryMethod::Median),
        "a stored weighted mean from before the median default moves to median");
    expect(Settings::loadSettings().summaryMethod == static_cast<int>(speechrate::SummaryMethod::Median),
        "the migrated median stays on the next load");

    settings = Settings::loadSettings();
    settings.summaryMethod = static_cast<int>(speechrate::SummaryMethod::WeightedMean);
    Settings::saveSettings(settings);
    expect(Settings::loadSettings().summaryMethod == static_cast<int>(speechrate::SummaryMethod::WeightedMean),
        "an explicit weighted mean stays after the default has moved");

    settings.summaryMethod = static_cast<int>(speechrate::SummaryMethod::Median);
    Settings::saveSettings(settings);
    SessionStore::clearAll();
}

void testMalformedJsonIsQuarantined()
{
    QDir().mkpath(SessionStore::sessionsDir());
    const QString malformed = SessionStore::sessionsDir() + QStringLiteral("/broken.json");
    QFile file(malformed);
    expect(file.open(QIODevice::WriteOnly), "open malformed fixture");
    file.write("{ definitely not json");
    file.close();

    SessionStore::listSessions();
    expect(!QFile::exists(malformed), "malformed source is moved");
    const QStringList quarantined = QDir(SessionStore::sessionsDir()).entryList(
        { QStringLiteral("broken.json.malformed-*") }, QDir::Files);
    expect(quarantined.size() == 1, "malformed session is quarantined");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir root;
    expect(root.isValid(), "temporary data root");
    Settings::setAppDataDirForTests(root.path());

    testRoundTripAndClear();
    testDurationWeightedSessionMean();
    testCommitKeepsWavUntilClose();
    testCommitKeepsWavWhenEnabled();
    testDiscardSkipsUnsavedWav();
    testMalformedJsonIsQuarantined();
    testPhrasalPauseDefaultMigration();
    testSummaryMethodChangesHistory();

    Settings::clearAppDataDirForTests();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "session store tests passed\n";
    return EXIT_SUCCESS;
}
