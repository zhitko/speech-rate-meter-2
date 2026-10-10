#include "sessionstore.h"

#include "src/api/helpers/logger.h"
#include "src/api/helpers/settings.h"
#include "src/services/wavfileservice.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMutex>
#include <QRecursiveMutex>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

QRecursiveMutex& fileMutex()
{
    static QRecursiveMutex mutex;
    return mutex;
}

QDateTime parseStamp(const QString& text)
{
    return QDateTime::fromString(text, QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzz"));
}

double sumOf(const QVariantList& segments, const QString& key)
{
    double sum = 0;
    for (const QVariant& item : segments)
        sum += item.toMap().value(key).toDouble();
    return sum;
}

double weightedMean(const QVariantList& segments, const QString& key)
{
    double weight = 0;
    double acc = 0;
    for (const QVariant& item : segments) {
        const QVariantMap row = item.toMap();
        const double duration = row.value(QStringLiteral("speechDuration")).toDouble();
        const double used = duration > 0 ? duration : 1;
        acc += row.value(key).toDouble() * used;
        weight += used;
    }
    return weight > 0 ? acc / weight : 0;
}

std::vector<int> intList(const QVariant& value)
{
    std::vector<int> lengths;
    const QVariantList list = value.toList();
    lengths.reserve(static_cast<std::size_t>(list.size()));
    for (const QVariant& item : list)
        lengths.push_back(item.toInt());
    return lengths;
}

QVariantList variantList(const std::vector<int>& lengths)
{
    QVariantList list;
    list.reserve(static_cast<int>(lengths.size()));
    for (int length : lengths)
        list.append(length);
    return list;
}

QString sessionTitle(const QString& startedAt, const QString& endedAt)
{
    const QDateTime started = parseStamp(startedAt);
    const QDateTime ended = parseStamp(endedAt);
    if (!started.isValid())
        return startedAt;
    const QLocale locale;
    const QString date = locale.toString(started.date(), QStringLiteral("d MMM yyyy"));
    const QString from = started.toString(QStringLiteral("HH:mm"));
    const QString to = ended.isValid() ? ended.toString(QStringLiteral("HH:mm")) : from;
    return date + QStringLiteral(", ") + from + QStringLiteral("–") + to;
}

} // namespace

QString SessionStore::sessionsDir()
{
    return Settings::getAppDataDir() + QStringLiteral("/data/sessions");
}

QString SessionStore::recordsDir()
{
    return Settings::getAppDataDir() + QStringLiteral("/data/records");
}

QString SessionStore::sessionPath(const QString& sessionId)
{
    return sessionsDir() + QLatin1Char('/') + sessionId + QStringLiteral(".json");
}

SessionStore::JsonState SessionStore::readJson(const QString& path, QVariantMap& root)
{
    root.clear();
    QFile file(path);
    if (!file.exists())
        return JsonState::Missing;
    if (!file.open(QIODevice::ReadOnly))
        return JsonState::Malformed;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return JsonState::Malformed;
    root = document.object().toVariantMap();
    if (root.isEmpty())
        return JsonState::Malformed;
    return JsonState::Valid;
}

bool SessionStore::quarantineMalformed(const QString& path)
{
    if (!QFileInfo::exists(path))
        return true;
    const QString suffix = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    QString quarantined = path + QStringLiteral(".malformed-") + suffix;
    for (int counter = 1; QFileInfo::exists(quarantined); ++counter)
        quarantined = path + QStringLiteral(".malformed-") + suffix + QLatin1Char('-') + QString::number(counter);
    if (QFile::rename(path, quarantined)) {
        LOG_WARNING() << "Quarantined malformed JSON:" << quarantined;
        return true;
    }
    LOG_WARNING() << "Could not quarantine malformed JSON:" << path;
    return false;
}

bool SessionStore::writeJson(const QString& path, const QVariantMap& root)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QJsonDocument document(QJsonObject::fromVariantMap(root));
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0)
        return false;
    return file.commit();
}

QString SessionStore::pendingPath(const QString& scratchPath)
{
    return scratchPath + QStringLiteral(".pending.json");
}

bool SessionStore::segmentAlreadyStored(const QString& sessionId, const QVariantMap& segment)
{
    const QString started = segment.value(QStringLiteral("startedAt")).toString();
    const QString ended = segment.value(QStringLiteral("endedAt")).toString();
    if (started.isEmpty() || ended.isEmpty())
        return false;
    QVariantMap root;
    if (readJson(sessionPath(sessionId), root) != JsonState::Valid)
        return false;
    const QVariantList segments = root.value(QStringLiteral("segments")).toList();
    for (const QVariant& item : segments) {
        const QVariantMap row = item.toMap();
        if (row.value(QStringLiteral("startedAt")).toString() == started
            && row.value(QStringLiteral("endedAt")).toString() == ended)
            return true;
    }
    return false;
}

bool SessionStore::appendWithRetry(const QString& sessionId,
    const QString& sessionStartedAt,
    const QVariantMap& segment)
{
    if (appendSegment(sessionId, sessionStartedAt, segment))
        return true;
    return appendSegment(sessionId, sessionStartedAt, segment);
}

void SessionStore::releaseScratch(const QString& scratchPath)
{
    if (scratchPath.isEmpty())
        return;
    removeFile(pendingPath(scratchPath));
    if (!Settings::loadSettings().keepRecordingFiles)
        removeFile(scratchPath);
}

bool SessionStore::hasPendingScratch()
{
    QDir records(recordsDir());
    if (!records.exists())
        return false;
    const QFileInfoList wavs = records.entryInfoList({ QStringLiteral("*.wav") }, QDir::Files);
    for (const QFileInfo& info : wavs) {
        if (QFileInfo::exists(pendingPath(info.absoluteFilePath())))
            return true;
    }
    return false;
}

QString SessionStore::openScratch(const QString& sessionId,
    const QString& sessionStartedAt,
    const QVariantMap& segment,
    const std::vector<float>& samples)
{
    const QString scratch = writeScratch(samples);
    if (scratch.isEmpty())
        return {};

    QVariantMap pending;
    pending.insert(QStringLiteral("sessionId"), sessionId);
    pending.insert(QStringLiteral("sessionStartedAt"), sessionStartedAt);
    pending.insert(QStringLiteral("segment"), segment);
    if (!writeJson(pendingPath(scratch), pending)) {
        removeFile(scratch);
        return {};
    }
    return scratch;
}

bool SessionStore::recoverPending()
{
    const QMutexLocker locker(&fileMutex());
    QDir records(recordsDir());
    if (!records.exists())
        return true;

    const QFileInfoList wavs = records.entryInfoList({ QStringLiteral("*.wav") }, QDir::Files, QDir::Name);
    for (const QFileInfo& info : wavs) {
        const QString scratch = info.absoluteFilePath();
        const QString pending = pendingPath(scratch);
        if (!QFileInfo::exists(pending))
            continue;

        QVariantMap meta;
        const JsonState state = readJson(pending, meta);
        if (state == JsonState::Malformed) {
            quarantineMalformed(pending);
            continue;
        }
        if (state != JsonState::Valid)
            continue;
        const QString sessionId = meta.value(QStringLiteral("sessionId")).toString();
        const QVariantMap segment = meta.value(QStringLiteral("segment")).toMap();
        if (sessionId.isEmpty() || segment.isEmpty())
            continue;

        bool stored = segmentAlreadyStored(sessionId, segment);
        if (!stored) {
            stored = appendWithRetry(sessionId,
                meta.value(QStringLiteral("sessionStartedAt")).toString(),
                segment);
        }
        if (!stored) {
            LOG_WARNING() << "Scratch still waiting to be stored:" << scratch;
            continue;
        }
        releaseScratch(scratch);
    }
    return !hasPendingScratch();
}

bool SessionStore::commitSegment(const QString& sessionId,
    const QString& sessionStartedAt,
    const QVariantMap& segment,
    const std::vector<float>& samples)
{
    const QMutexLocker locker(&fileMutex());
    recoverPending();

    QString scratch;
    if (!hasPendingScratch())
        scratch = openScratch(sessionId, sessionStartedAt, segment, samples);

    if (segmentAlreadyStored(sessionId, segment)) {
        releaseScratch(scratch);
        return true;
    }

    if (appendWithRetry(sessionId, sessionStartedAt, segment)) {
        releaseScratch(scratch);
        return true;
    }

    if (scratch.isEmpty())
        scratch = openScratch(sessionId, sessionStartedAt, segment, samples);
    LOG_WARNING() << "Session file was not written; scratch kept at" << scratch;
    return false;
}

bool SessionStore::appendSegment(const QString& sessionId,
    const QString& sessionStartedAt,
    const QVariantMap& segment)
{
    const QMutexLocker locker(&fileMutex());
    const QString path = sessionPath(sessionId);
    QVariantMap root;
    const JsonState state = readJson(path, root);
    if (state == JsonState::Malformed && !quarantineMalformed(path))
        return false;
    if (state != JsonState::Valid) {
        root.insert(QStringLiteral("id"), sessionId);
        root.insert(QStringLiteral("startedAt"), sessionStartedAt);
        root.insert(QStringLiteral("segments"), QVariantList {});
    }
    QVariantList segments = root.value(QStringLiteral("segments")).toList();
    segments.append(segment);
    root.insert(QStringLiteral("segments"), segments);
    root.insert(QStringLiteral("endedAt"), segment.value(QStringLiteral("endedAt")).toString());
    return writeJson(path, root);
}

bool SessionStore::setEndedAt(const QString& sessionId, const QString& endedAt)
{
    const QMutexLocker locker(&fileMutex());
    const QString path = sessionPath(sessionId);
    if (!QFileInfo::exists(path))
        return true;
    QVariantMap root;
    const JsonState state = readJson(path, root);
    if (state == JsonState::Malformed)
        quarantineMalformed(path);
    if (state != JsonState::Valid)
        return false;
    root.insert(QStringLiteral("endedAt"), endedAt);
    return writeJson(path, root);
}

bool SessionStore::appendShown(const QString& sessionId,
    const QString& sessionStartedAt,
    const QVariantMap& sample)
{
    if (sessionId.isEmpty() || sample.isEmpty())
        return false;
    const QMutexLocker locker(&fileMutex());
    const QString path = sessionPath(sessionId);
    QVariantMap root;
    const JsonState state = readJson(path, root);
    if (state == JsonState::Malformed && !quarantineMalformed(path))
        return false;
    if (state != JsonState::Valid) {
        root.insert(QStringLiteral("id"), sessionId);
        root.insert(QStringLiteral("startedAt"), sessionStartedAt);
        root.insert(QStringLiteral("segments"), QVariantList {});
    }
    QVariantList shown = root.value(QStringLiteral("shown")).toList();
    shown.append(sample);
    root.insert(QStringLiteral("shown"), shown);
    root.insert(QStringLiteral("endedAt"), sample.value(QStringLiteral("at")).toString());
    return writeJson(path, root);
}

QVariantMap SessionStore::summarize(const QVariantMap& root)
{
    const QVariantList segments = root.value(QStringLiteral("segments")).toList();
    const QVariantList shown = root.value(QStringLiteral("shown")).toList();
    if (segments.isEmpty() && shown.isEmpty())
        return {};

    QVariantMap summary;
    summary.insert(QStringLiteral("id"), root.value(QStringLiteral("id")).toString());
    summary.insert(QStringLiteral("startedAt"), root.value(QStringLiteral("startedAt")).toString());
    summary.insert(QStringLiteral("endedAt"), root.value(QStringLiteral("endedAt")).toString());
    summary.insert(QStringLiteral("title"),
        sessionTitle(summary.value(QStringLiteral("startedAt")).toString(),
            summary.value(QStringLiteral("endedAt")).toString()));
    summary.insert(QStringLiteral("phraseCount"), segments.size());
    summary.insert(QStringLiteral("updateCount"), shown.isEmpty() ? segments.size() : shown.size());

    if (segments.isEmpty()) {
        const QVariantMap last = shown.last().toMap();
        summary.insert(QStringLiteral("speechRate"), last.value(QStringLiteral("speechRate")));
        summary.insert(QStringLiteral("articulationRate"), last.value(QStringLiteral("articulationRate")));
        summary.insert(QStringLiteral("phrasePauses"), last.value(QStringLiteral("phrasePauses")));
        summary.insert(QStringLiteral("speechDuration"), last.value(QStringLiteral("speechDuration")));
        summary.insert(QStringLiteral("fillerPercent"), last.value(QStringLiteral("fillerPercent")));
        return summary;
    }

    summary.insert(QStringLiteral("speechRate"), weightedMean(segments, QStringLiteral("speechRate")));
    summary.insert(QStringLiteral("articulationRate"), weightedMean(segments, QStringLiteral("articulationRate")));
    summary.insert(QStringLiteral("phrasePauses"), weightedMean(segments, QStringLiteral("phrasePauses")));
    summary.insert(QStringLiteral("speechDuration"), sumOf(segments, QStringLiteral("speechDuration")));
    summary.insert(QStringLiteral("fillerPercent"), weightedMean(segments, QStringLiteral("fillerPercent")));
    return summary;
}

void SessionStore::insertMeasurement(QVariantMap& target,
    const speechrate::Parts& parts,
    const speechrate::Config& config,
    double fillerMin,
    double fillerMax)
{
    target.insert(QStringLiteral("vowelLengths"), variantList(parts.vowelLengths));
    target.insert(QStringLiteral("gapLengths"), variantList(parts.gapLengths));
    target.insert(QStringLiteral("vowelMaxFrames"), parts.vowelMaxFrames);
    target.insert(QStringLiteral("gapMaxFrames"), parts.gapMaxFrames);
    target.insert(QStringLiteral("frame"), config.frame);
    target.insert(QStringLiteral("shift"), config.shift);
    target.insert(QStringLiteral("smooth"), config.smooth);
    target.insert(QStringLiteral("minLengthMs"), config.minLengthMs);
    target.insert(QStringLiteral("degree"), config.degree);
    target.insert(QStringLiteral("k1"), config.k1);
    target.insert(QStringLiteral("k2"), config.k2);
    target.insert(QStringLiteral("k3"), config.k3);
    target.insert(QStringLiteral("k4"), config.k4);
    target.insert(QStringLiteral("fillerMin"), fillerMin);
    target.insert(QStringLiteral("fillerMax"), fillerMax);
}

bool SessionStore::takeMeasurement(const QVariantMap& source,
    speechrate::Parts& parts,
    speechrate::Config& config,
    double& fillerMin,
    double& fillerMax)
{
    if (!source.contains(QStringLiteral("vowelLengths")) || !source.contains(QStringLiteral("gapLengths")))
        return false;
    parts = {};
    parts.vowelLengths = intList(source.value(QStringLiteral("vowelLengths")));
    parts.gapLengths = intList(source.value(QStringLiteral("gapLengths")));
    parts.speechDuration = source.value(QStringLiteral("speechDuration")).toDouble();
    parts.vowelMaxFrames = source.value(QStringLiteral("vowelMaxFrames")).toDouble();
    parts.gapMaxFrames = source.value(QStringLiteral("gapMaxFrames")).toDouble();
    config = {};
    config.frame = source.value(QStringLiteral("frame")).toInt();
    config.shift = source.value(QStringLiteral("shift")).toInt();
    config.smooth = source.value(QStringLiteral("smooth")).toInt();
    config.minLengthMs = source.value(QStringLiteral("minLengthMs")).toInt();
    config.degree = source.value(QStringLiteral("degree")).toInt();
    config.k1 = source.value(QStringLiteral("k1")).toDouble();
    config.k2 = source.value(QStringLiteral("k2")).toDouble();
    config.k3 = source.value(QStringLiteral("k3")).toDouble();
    config.k4 = source.value(QStringLiteral("k4")).toDouble();
    fillerMin = source.value(QStringLiteral("fillerMin")).toDouble();
    fillerMax = source.value(QStringLiteral("fillerMax")).toDouble();
    return config.shift > 0 && parts.speechDuration > 0;
}

QVariantList SessionStore::listSessions()
{
    const QMutexLocker locker(&fileMutex());
    QVariantList sessions;
    QDir dir(sessionsDir());
    if (!dir.exists())
        return sessions;

    const QFileInfoList files = dir.entryInfoList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name);
    for (const QFileInfo& info : files) {
        QVariantMap root;
        const JsonState state = readJson(info.absoluteFilePath(), root);
        if (state == JsonState::Malformed) {
            quarantineMalformed(info.absoluteFilePath());
            continue;
        }
        const QVariantMap summary = summarize(root);
        if (!summary.isEmpty())
            sessions.append(summary);
    }
    std::reverse(sessions.begin(), sessions.end());
    return sessions;
}

QVariantMap SessionStore::loadSession(const QString& sessionId)
{
    const QMutexLocker locker(&fileMutex());
    const QString path = sessionPath(sessionId);
    QVariantMap root;
    const JsonState state = readJson(path, root);
    if (state == JsonState::Malformed)
        quarantineMalformed(path);
    if (state != JsonState::Valid)
        return {};
    QVariantMap session = summarize(root);
    if (session.isEmpty())
        return {};

    QVariantList points;
    const QVariantList shown = root.value(QStringLiteral("shown")).toList();
    const QVariantList segments = root.value(QStringLiteral("segments")).toList();
    const QVariantList series = shown.isEmpty() ? segments : shown;
    for (const QVariant& item : series) {
        const QVariantMap row = item.toMap();
        const QString when = shown.isEmpty()
            ? row.value(QStringLiteral("endedAt")).toString()
            : row.value(QStringLiteral("at")).toString();
        const QDateTime ended = parseStamp(when);
        QVariantMap point;
        point.insert(QStringLiteral("speechRate"), row.value(QStringLiteral("speechRate")));
        point.insert(QStringLiteral("articulationRate"), row.value(QStringLiteral("articulationRate")));
        point.insert(QStringLiteral("phrasePauses"), row.value(QStringLiteral("phrasePauses")));
        point.insert(QStringLiteral("speechDuration"), row.value(QStringLiteral("speechDuration")));
        point.insert(QStringLiteral("fillerPercent"), row.value(QStringLiteral("fillerPercent")));
        point.insert(QStringLiteral("clock"),
            ended.isValid() ? ended.toString(QStringLiteral("HH:mm:ss"))
                            : when);
        points.append(point);
    }
    session.insert(QStringLiteral("segments"), points);
    return session;
}

void SessionStore::clearAll()
{
    const QMutexLocker locker(&fileMutex());
    QDir sessions(sessionsDir());
    if (sessions.exists()) {
        const QFileInfoList files = sessions.entryInfoList(QDir::Files);
        for (const QFileInfo& info : files)
            QFile::remove(info.absoluteFilePath());
    }

    QDir records(recordsDir());
    if (!records.exists())
        return;
    const QFileInfoList wavs = records.entryInfoList(QDir::Files);
    for (const QFileInfo& info : wavs)
        QFile::remove(info.absoluteFilePath());
}

QString SessionStore::writeScratch(const std::vector<float>& samples)
{
    std::vector<char> buffer(samples.size() * 2);
    for (std::size_t index = 0; index < samples.size(); ++index) {
        int rounded = static_cast<int>(std::lround(samples[index]));
        rounded = std::clamp(rounded, -32768, 32767);
        const auto bits = static_cast<std::uint16_t>(static_cast<std::int16_t>(rounded));
        buffer[index * 2] = static_cast<char>(bits & 0xff);
        buffer[index * 2 + 1] = static_cast<char>((bits >> 8) & 0xff);
    }

    AudioFormat format;
    format.sampleRate = 8000;
    format.channelCount = 1;
    format.bitsPerSample = 16;

    const QString stem = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy.HH.mm.ss.zzz"));
    QString name = stem;
    const QDir records(recordsDir());
    for (int extra = 1; QFileInfo::exists(records.filePath(name + QStringLiteral(".wav")))
         || QFileInfo::exists(records.filePath(name + QStringLiteral(".wav.pending.json")));
         ++extra) {
        name = stem + QLatin1Char('-') + QString::number(extra);
    }
    WavFileService service(Settings::getAppDataDir().toStdString());
    const std::string relative = service.writeWaveFile(name.toStdString(), buffer, format);
    if (relative.empty())
        return {};
    return Settings::getAppDataDir() + QLatin1Char('/') + QString::fromStdString(relative);
}

void SessionStore::removeFile(const QString& absolutePath)
{
    if (!absolutePath.isEmpty())
        QFile::remove(absolutePath);
}

bool SessionStore::recordsAreEmpty()
{
    QDir records(recordsDir());
    if (!records.exists())
        return true;
    return records.entryList(QDir::Files).isEmpty();
}
