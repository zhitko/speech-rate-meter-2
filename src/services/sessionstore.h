#ifndef SESSIONSTORE_H
#define SESSIONSTORE_H

#include "speechrateanalysis.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <vector>

/**
 * One JSON file per recording session under data/sessions/.
 * Scratch WAV files live under data/records/. After the segment's metrics
 * are stored, the WAV is removed unless Keep recording files is on.
 */
class SessionStore {
public:
    static QString sessionsDir();
    static QString recordsDir();

    static bool appendSegment(const QString& sessionId,
        const QString& sessionStartedAt,
        const QVariantMap& segment);

    /**
     * Stores one segment. Earlier scratch files are retried first.
     * This segment's WAV is deleted only after its metrics are in the session
     * file, unless Keep recording files is on. A failed write keeps that WAV.
     */
    static bool commitSegment(const QString& sessionId,
        const QString& sessionStartedAt,
        const QVariantMap& segment,
        const std::vector<float>& samples);

    /** Retries leftover scratches. True when none are still waiting to be stored. */
    static bool recoverPending();

    static bool setEndedAt(const QString& sessionId, const QString& endedAt);

    /** Appends one metric snapshot that was actually shown on Home. */
    static bool appendShown(const QString& sessionId,
        const QString& sessionStartedAt,
        const QVariantMap& sample);

    static QVariantList listSessions();
    static QVariantMap loadSession(const QString& sessionId);

    static void clearAll();

    /** Stores the lengths and coefficients needed to rebuild a session result. */
    static void insertMeasurement(QVariantMap& target,
        const speechrate::Parts& parts,
        const speechrate::Config& config,
        double fillerMin,
        double fillerMax);

    /**
     * Reads a stored phrase measurement.
     * False when this segment was written before length lists were kept.
     */
    static bool takeMeasurement(const QVariantMap& source,
        speechrate::Parts& parts,
        speechrate::Config& config,
        double& fillerMin,
        double& fillerMax);

    /** Writes one scratch WAV. Returns the absolute path, or empty on failure. */
    static QString writeScratch(const std::vector<float>& samples);
    static void removeFile(const QString& absolutePath);
    static bool recordsAreEmpty();

private:
    enum class JsonState { Missing, Valid, Malformed };

    static QString sessionPath(const QString& sessionId);
    static QString pendingPath(const QString& scratchPath);
    static bool writeJson(const QString& path, const QVariantMap& root);
    static JsonState readJson(const QString& path, QVariantMap& root);
    static bool quarantineMalformed(const QString& path);
    static QVariantMap summarize(const QVariantMap& root);
    static bool segmentAlreadyStored(const QString& sessionId, const QVariantMap& segment);
    static bool appendWithRetry(const QString& sessionId,
        const QString& sessionStartedAt,
        const QVariantMap& segment);
    static QString openScratch(const QString& sessionId,
        const QString& sessionStartedAt,
        const QVariantMap& segment,
        const std::vector<float>& samples);
    static void releaseScratch(const QString& scratchPath);
    static bool hasPendingScratch();
};

#endif
