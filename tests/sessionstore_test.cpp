#include "src/api/helpers/settings.h"
#include "src/services/sessionstore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>
#include <cstdlib>
#include <iostream>

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
    testMalformedJsonIsQuarantined();

    Settings::clearAppDataDirForTests();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "session store tests passed\n";
    return EXIT_SUCCESS;
}
