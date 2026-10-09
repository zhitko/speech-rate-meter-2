#include "sessionapi.h"

#include "helpers/logger.h"
#include "helpers/settings.h"
#include "src/services/helpers/wavFile.h"
#include "src/services/sessionstore.h"
#include "src/services/speechrateanalysis.h"
#include "src/services/vadautocorrelationservice.h"
#include "src/services/vadenergryservice.h"

#include <QAudioFormat>
#include <QAudioSource>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QMediaDevices>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QUrl>
#include <QWaitCondition>

#ifdef Q_OS_ANDROID
#include <QPermission>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <utility>
#include <vector>

namespace {

constexpr int kSampleRate = 8000;
constexpr int kHop = 64;
constexpr int kVadFrame = 128;
constexpr qint64 kPadSamples = kSampleRate * 300 / 1000;
// A phrase shorter than this is not stored.
constexpr qint64 kMinPhraseSamples = kSampleRate;
// Live numbers need at least this much audio in the window.
constexpr qint64 kMinWindowSamples = kSampleRate * 3 / 2;
// Longer speech without a pause is stored as several segments.
constexpr qint64 kMaxSegmentSamples = kSampleRate * 15;
constexpr qint64 kMaxWindowSamples = kSampleRate * 30;

QString stamp(const QDateTime& time)
{
    return time.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzz"));
}

int roundHalfAway(double value)
{
    if (value >= 0)
        return static_cast<int>(std::floor(value + 0.5));
    return static_cast<int>(std::ceil(value - 0.5));
}

speechrate::Config configFrom(const AppSettings& settings)
{
    speechrate::Config config;
    config.frame = settings.intensityFrame;
    config.shift = settings.intensityShift;
    config.smooth = settings.intensitySmooth;
    config.minLengthMs = settings.segmentMinLengthMs;
    config.degree = settings.meanValueDegry;
    config.k1 = settings.speechRateK1;
    config.k2 = settings.articulationK2;
    config.k3 = settings.pausesK3;
    config.k4 = settings.fillerK4;
    return config;
}

QVariantMap metricsToMap(const speechrate::Metrics& metrics,
    double fillerMin,
    double fillerMax,
    bool live,
    int phraseEpoch,
    bool stored)
{
    QVariantMap map;
    map.insert(QStringLiteral("valid"), metrics.valid);
    map.insert(QStringLiteral("live"), live);
    map.insert(QStringLiteral("stored"), stored);
    map.insert(QStringLiteral("phraseEpoch"), phraseEpoch);
    map.insert(QStringLiteral("speechRate"), metrics.speechRate);
    map.insert(QStringLiteral("articulationRate"), metrics.articulationRate);
    map.insert(QStringLiteral("phrasePauses"), metrics.phrasePauses);
    map.insert(QStringLiteral("speechDuration"), metrics.speechDuration);
    map.insert(QStringLiteral("fillerScore"), metrics.fillerScore);
    map.insert(QStringLiteral("fillerPercent"),
        roundHalfAway(speechrate::fillerPercent(metrics.fillerScore, fillerMin, fillerMax)));
    map.insert(QStringLiteral("gapLength"), metrics.gapLength);
    map.insert(QStringLiteral("gapCount"), metrics.gapCount);
    map.insert(QStringLiteral("gapMax"), metrics.gapMax);
    map.insert(QStringLiteral("gapMean"), metrics.gapMean);
    map.insert(QStringLiteral("gapMedian"), metrics.gapMedian);
    map.insert(QStringLiteral("vowelLength"), metrics.vowelLength);
    map.insert(QStringLiteral("vowelCount"), metrics.vowelCount);
    map.insert(QStringLiteral("vowelMax"), metrics.vowelMax);
    map.insert(QStringLiteral("vowelMean"), metrics.vowelMean);
    map.insert(QStringLiteral("vowelMedian"), metrics.vowelMedian);
    map.insert(QStringLiteral("vowelsPerSecond"), metrics.vowelsPerSecond);
    return map;
}

std::vector<float> readWavSamples(const QString& path)
{
    WaveFile* wave = waveOpenFile(path.toStdString());
    if (!wave || !wave->dataChunk || !wave->formatChunk) {
        if (wave)
            waveCloseFile(wave);
        return {};
    }

    const std::uint32_t dataSize = littleEndianBytesToUInt32(wave->dataChunk->chunkDataSize);
    const std::uint16_t bitDepth = littleEndianBytesToUInt16(wave->formatChunk->significantBitsPerSample);
    const int bytesPerSample = bitDepth / 8;
    std::vector<float> samples;
    if (bytesPerSample > 0 && wave->dataChunk->waveformData && dataSize > 0) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(wave->dataChunk->waveformData);
        const int count = static_cast<int>(dataSize / static_cast<std::uint32_t>(bytesPerSample));
        samples.reserve(static_cast<std::size_t>(count));
        for (int index = 0; index < count; ++index) {
            const int offset = index * bytesPerSample;
            if (offset + 1 >= static_cast<int>(dataSize))
                break;
            const auto value = static_cast<std::int16_t>(bytes[offset] | (bytes[offset + 1] << 8));
            samples.push_back(static_cast<float>(value));
        }
    }
    waveCloseFile(wave);
    return samples;
}

QString validateWavFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable())
        return QCoreApplication::translate("SessionApi", "The selected file cannot be opened.");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QCoreApplication::translate("SessionApi", "The selected file cannot be opened.");
    const QByteArray bytes = file.readAll();
    if (bytes.size() < 12 || bytes.first(4) != "RIFF" || bytes.sliced(8, 4) != "WAVE") {
        return QCoreApplication::translate("SessionApi", "The selected file is not a valid WAV file.");
    }

    const auto u16 = [&bytes](qsizetype at) {
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.constData() + at);
        return std::uint16_t(p[0] | (p[1] << 8));
    };
    const auto u32 = [&bytes](qsizetype at) {
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.constData() + at);
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8)
            | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    };
    const std::uint32_t riffSize = u32(4);
    if (riffSize < 4 || static_cast<quint64>(riffSize) + 8 > static_cast<quint64>(bytes.size()))
        return QCoreApplication::translate("SessionApi", "The selected file is not a valid WAV file.");

    bool exactPcm = false;
    bool validData = false;
    qsizetype offset = 12;
    const qsizetype riffEnd = static_cast<qsizetype>(riffSize) + 8;
    while (offset + 8 <= riffEnd) {
        const QByteArray id = bytes.sliced(offset, 4);
        const std::uint32_t size = u32(offset + 4);
        const quint64 end = static_cast<quint64>(offset) + 8 + size;
        const quint64 paddedEnd = end + (size & 1U);
        if (end > static_cast<quint64>(riffEnd) || paddedEnd > static_cast<quint64>(riffEnd))
            return QCoreApplication::translate("SessionApi", "The selected file is not a valid WAV file.");
        if (id == "fmt " && size >= 16) {
            const qsizetype body = offset + 8;
            exactPcm = u16(body) == 1 && u16(body + 2) == 1
                && u32(body + 4) == kSampleRate && u32(body + 8) == kSampleRate * 2
                && u16(body + 12) == 2 && u16(body + 14) == 16;
        } else if (id == "data") {
            validData = size >= 2 && (size % 2) == 0;
        }
        offset = static_cast<qsizetype>(paddedEnd);
    }
    if (!exactPcm || !validData) {
        return QCoreApplication::translate(
            "SessionApi", "Use a PCM WAV file: 8000 Hz, mono, signed 16-bit little-endian.");
    }
    return {};
}

enum class JobKind { Live, Final, OpenFile, EndSession, ClearAll };

struct Job {
    JobKind kind = JobKind::Live;
    std::vector<float> samples;
    speechrate::Config config;
    double fillerMin = 120;
    double fillerMax = 240;
    int phraseEpoch = 0;
    double openSeconds = 0;
    quint64 liveGen = 0;
    quint64 generation = 0;
    QString path;
    QString sessionId;
    QString sessionStartedAt;
    QString segmentStartedAt;
    QString segmentEndedAt;
    QString sessionEndedAt;
};

} // namespace

class JobQueue {
public:
    void pushLive(Job job)
    {
        QMutexLocker lock(&m_mutex);
        if (m_stopping)
            return;
        job.liveGen = ++m_liveGen;
        m_live = std::move(job);
        m_hasLive = true;
        m_cond.wakeOne();
    }

    void pushOrdered(Job job)
    {
        QMutexLocker lock(&m_mutex);
        if (m_stopping)
            return;
        m_ordered.push_back(std::move(job));
        m_cond.wakeOne();
    }

    bool pop(Job& job)
    {
        QMutexLocker lock(&m_mutex);
        for (;;) {
            if (!m_ordered.empty()) {
                job = std::move(m_ordered.front());
                m_ordered.pop_front();
                return true;
            }
            if (m_hasLive && !m_stopping) {
                job = std::move(m_live);
                m_hasLive = false;
                return true;
            }
            if (m_stopping)
                return false;
            m_cond.wait(&m_mutex);
        }
    }

    void stop()
    {
        QMutexLocker lock(&m_mutex);
        m_stopping = true;
        m_hasLive = false;
        m_cond.wakeAll();
    }

    quint64 liveGeneration() const
    {
        QMutexLocker lock(&m_mutex);
        return m_liveGen;
    }

    void setPhraseEpoch(int epoch) { m_phraseEpoch.store(epoch); }
    int phraseEpoch() const { return m_phraseEpoch.load(); }

private:
    mutable QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_stopping = false;
    bool m_hasLive = false;
    quint64 m_liveGen = 0;
    Job m_live;
    std::deque<Job> m_ordered;
    std::atomic<int> m_phraseEpoch { 0 };
};

class AnalysisThread : public QThread {
public:
    AnalysisThread(SessionApi* api, JobQueue* queue)
        : m_api(api)
        , m_queue(queue)
    {
    }

protected:
    void run() override
    {
        Job job;
        while (m_queue->pop(job)) {
            if (job.kind == JobKind::ClearAll) {
                SessionStore::clearAll();
                m_cache.clear();
                QMetaObject::invokeMethod(m_api, "applyUserDataCleared", Qt::QueuedConnection);
                continue;
            }
            if (job.kind == JobKind::EndSession) {
                SessionStore::setEndedAt(job.sessionId, job.sessionEndedAt);
                SessionStore::recoverPending();
                QMetaObject::invokeMethod(m_api, "notifySessions", Qt::QueuedConnection);
                QMetaObject::invokeMethod(m_api, "applySessionEnded", Qt::QueuedConnection,
                    Q_ARG(quint64, job.generation));
                continue;
            }

            QVariantMap map;
            const auto stampJob = [&](QVariantMap& target) {
                target.insert(QStringLiteral("scope"),
                    job.kind == JobKind::OpenFile ? QStringLiteral("file") : QStringLiteral("phrase"));
                target.insert(QStringLiteral("generation"), job.generation);
                target.insert(QStringLiteral("sessionId"), job.sessionId);
                target.insert(QStringLiteral("sessionStartedAt"), job.sessionStartedAt);
            };
            if (job.kind == JobKind::OpenFile) {
                const QString key = QFileInfo(job.path).canonicalFilePath();
                if (key.isEmpty()) {
                    QMetaObject::invokeMethod(m_api, "applyOpenFileFinished", Qt::QueuedConnection,
                        Q_ARG(bool, false),
                        Q_ARG(QString, QCoreApplication::translate("SessionApi", "The selected file cannot be opened.")));
                    continue;
                }
                const auto cached = m_cache.constFind(key);
                if (cached != m_cache.cend()) {
                    map = cached.value();
                } else {
                    const std::vector<float> samples = readWavSamples(job.path);
                    const speechrate::Measurement measured = speechrate::measure(samples, job.config);
                    if (!measured.metrics.valid) {
                        QMetaObject::invokeMethod(m_api, "applyOpenFileFinished", Qt::QueuedConnection,
                            Q_ARG(bool, false),
                            Q_ARG(QString, QCoreApplication::translate("SessionApi", "No measurable speech was found in the file.")));
                        continue;
                    }
                    map = metricsToMap(measured.metrics, job.fillerMin, job.fillerMax, false, 0, false);
                    SessionStore::insertMeasurement(map, measured.parts, job.config, job.fillerMin, job.fillerMax);
                    m_cache.insert(key, map);
                }
                stampJob(map);
            } else {
                const speechrate::Measurement measured = speechrate::measure(job.samples, job.config);
                if (job.kind == JobKind::Live
                    && (m_queue->liveGeneration() != job.liveGen
                        || m_queue->phraseEpoch() != job.phraseEpoch)) {
                    continue;
                }
                if (!measured.metrics.valid) {
                    if (job.kind == JobKind::Live) {
                        map.insert(QStringLiteral("valid"), false);
                        map.insert(QStringLiteral("live"), true);
                        map.insert(QStringLiteral("phraseEpoch"), job.phraseEpoch);
                        stampJob(map);
                        QMetaObject::invokeMethod(m_api, "applyMetrics", Qt::QueuedConnection, Q_ARG(QVariantMap, map));
                    }
                    continue;
                }

                bool stored = false;
                if (job.kind == JobKind::Final) {
                    map = metricsToMap(measured.metrics, job.fillerMin, job.fillerMax, false, job.phraseEpoch, true);
                    QVariantMap segment;
                    segment.insert(QStringLiteral("startedAt"), job.segmentStartedAt);
                    segment.insert(QStringLiteral("endedAt"), job.segmentEndedAt);
                    segment.insert(QStringLiteral("speechRate"), measured.metrics.speechRate);
                    segment.insert(QStringLiteral("articulationRate"), measured.metrics.articulationRate);
                    segment.insert(QStringLiteral("phrasePauses"), measured.metrics.phrasePauses);
                    segment.insert(QStringLiteral("speechDuration"), measured.metrics.speechDuration);
                    segment.insert(QStringLiteral("fillerPercent"), map.value(QStringLiteral("fillerPercent")).toInt());
                    SessionStore::insertMeasurement(segment, measured.parts, job.config, job.fillerMin, job.fillerMax);
                    stored = SessionStore::commitSegment(job.sessionId,
                        job.sessionStartedAt,
                        segment,
                        job.samples);
                    if (stored)
                        LOG_INFO() << "Stored phrase" << job.segmentStartedAt << "rate" << measured.metrics.speechRate;
                } else {
                    map = metricsToMap(measured.metrics, job.fillerMin, job.fillerMax, true, job.phraseEpoch, false);
                    map.insert(QStringLiteral("openSeconds"), job.openSeconds);
                }
                map.insert(QStringLiteral("stored"), stored);
                if (job.kind == JobKind::Final)
                    SessionStore::insertMeasurement(map, measured.parts, job.config, job.fillerMin, job.fillerMax);
                stampJob(map);
            }

            if (map.isEmpty())
                continue;
            QMetaObject::invokeMethod(m_api, "applyMetrics", Qt::QueuedConnection, Q_ARG(QVariantMap, map));
            if (job.kind == JobKind::OpenFile) {
                QMetaObject::invokeMethod(m_api, "applyOpenFileFinished", Qt::QueuedConnection,
                    Q_ARG(bool, true), Q_ARG(QString, QString()));
            }
            if (map.value(QStringLiteral("stored")).toBool())
                QMetaObject::invokeMethod(m_api, "notifySessions", Qt::QueuedConnection);
        }
    }

private:
    SessionApi* m_api = nullptr;
    JobQueue* m_queue = nullptr;
    QHash<QString, QVariantMap> m_cache;
};

class StreamResampler {
public:
    void reset(const QAudioFormat& format)
    {
        m_format = format;
        m_pending.clear();
        m_position = 0;
        m_passthrough = format.sampleRate() == kSampleRate && format.channelCount() == 1
            && format.sampleFormat() == QAudioFormat::Int16;
    }

    std::vector<qint16> process(const QByteArray& data)
    {
        if (m_passthrough)
            return decodeInt16(data);
        appendMono(data);
        return resample();
    }

private:
    static std::vector<qint16> decodeInt16(const QByteArray& data)
    {
        const int count = data.size() / 2;
        std::vector<qint16> samples(static_cast<std::size_t>(count));
        const auto* bytes = reinterpret_cast<const unsigned char*>(data.constData());
        for (int index = 0; index < count; ++index) {
            samples[static_cast<std::size_t>(index)] = static_cast<qint16>(
                bytes[index * 2] | (bytes[index * 2 + 1] << 8));
        }
        return samples;
    }

    void appendMono(const QByteArray& data)
    {
        const int channels = std::max(1, m_format.channelCount());
        const int frameBytes = std::max(1, m_format.bytesPerFrame());
        const int frames = data.size() / frameBytes;
        const auto* bytes = reinterpret_cast<const unsigned char*>(data.constData());
        for (int frame = 0; frame < frames; ++frame) {
            double sum = 0;
            const unsigned char* frameBytesPtr = bytes + frame * frameBytes;
            for (int channel = 0; channel < channels; ++channel) {
                sum += sampleToInt16(frameBytesPtr + channel * m_format.bytesPerSample());
            }
            m_pending.push_back(sum / channels);
        }
    }

    double sampleToInt16(const unsigned char* sample) const
    {
        switch (m_format.sampleFormat()) {
        case QAudioFormat::UInt8:
            return (static_cast<int>(sample[0]) - 128) * 256.0;
        case QAudioFormat::Int16:
            return static_cast<double>(static_cast<qint16>(sample[0] | (sample[1] << 8)));
        case QAudioFormat::Int32: {
            const qint32 value = qint32(sample[0] | (sample[1] << 8) | (sample[2] << 16) | (sample[3] << 24));
            return static_cast<double>(value) * (32767.0 / 2147483647.0);
        }
        case QAudioFormat::Float: {
            float value = 0;
            std::memcpy(&value, sample, sizeof(float));
            return static_cast<double>(value) * 32767.0;
        }
        default:
            return 0;
        }
    }

    std::vector<qint16> resample()
    {
        std::vector<qint16> output;
        if (m_format.sampleRate() <= 0)
            return output;
        const double step = static_cast<double>(m_format.sampleRate()) / static_cast<double>(kSampleRate);
        while (m_position + 1.0 < static_cast<double>(m_pending.size())) {
            const int index = static_cast<int>(m_position);
            const double fraction = m_position - index;
            const double mixed = m_pending[static_cast<std::size_t>(index)] * (1.0 - fraction)
                + m_pending[static_cast<std::size_t>(index + 1)] * fraction;
            int sample = static_cast<int>(std::lround(mixed));
            sample = std::clamp(sample, -32768, 32767);
            output.push_back(static_cast<qint16>(sample));
            m_position += step;
        }
        const int drop = static_cast<int>(m_position);
        if (drop > 0) {
            m_pending.erase(m_pending.begin(), m_pending.begin() + drop);
            m_position -= drop;
        }
        return output;
    }

    QAudioFormat m_format;
    std::vector<double> m_pending;
    double m_position = 0;
    bool m_passthrough = true;
};

class CaptureWorker : public QObject {
    Q_OBJECT
public:
    CaptureWorker(JobQueue* queue, QObject* parent = nullptr)
        : QObject(parent)
        , m_queue(queue)
    {
    }

public slots:
    void startCapture(quint64 generation)
    {
        if (m_running)
            return;

        m_generation = generation;
        m_settings = Settings::loadSettings();
        m_useSpeechDetection = m_settings.autoCalibrate;
        m_settingsTimer.start();
        m_pcm.clear();
        m_recent.clear();
        m_origin = 0;
        m_fed = 0;
        m_phraseSpeaking = false;
        m_padLeading = true;
        m_dropped = false;
        m_speechStart = 0;
        m_speechEnd = 0;
        m_silenceFrames = 0;
        m_epoch = 0;
        m_queue->setPhraseEpoch(m_epoch);
        m_liveTimer.invalidate();

        const QDateTime started = QDateTime::currentDateTime();
        m_sessionId = started.toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
        m_sessionStartedAt = stamp(started);

        m_device = QMediaDevices::defaultAudioInput();
        if (m_device.isNull()) {
            emit deviceFailed();
            return;
        }

        QAudioFormat format;
        format.setSampleRate(kSampleRate);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        if (!m_device.isFormatSupported(format)) {
            QAudioFormat preferred = m_device.preferredFormat();
            preferred.setChannelCount(1);
            preferred.setSampleFormat(QAudioFormat::Int16);
            format = m_device.isFormatSupported(preferred) ? preferred : m_device.preferredFormat();
        }
        if (!format.isValid()) {
            emit deviceFailed();
            return;
        }
        m_resampler.reset(format);

        m_energy = std::make_unique<VADEnergyService>(this);
        m_energy->reset();
        m_energy->setThreshold(m_settings.vadThreshold > 0 ? m_settings.vadThreshold : 0);
        m_corr = std::make_unique<VADAutocorrelationService>(this);
        m_corr->reset();
        m_corr->setSampleRate(kSampleRate);
        m_corr->setPitchRange(m_settings.autoCorrMinF0, m_settings.autoCorrMaxF0);
        const double high = m_settings.autoCorrThreshold > 0 ? m_settings.autoCorrThreshold : 0.45;
        const double low = m_settings.autoCorrThreshold > 0 ? std::max(0.0, high - 0.1) : 0.35;
        m_corr->setVoiceThresholdHigh(high);
        m_corr->setVoiceThresholdLow(low);
        m_corr->setEnergyThreshold(m_settings.autoCorrEnergyThreshold > 0 ? m_settings.autoCorrEnergyThreshold : 0.02);

        m_source = std::make_unique<QAudioSource>(m_device, format, this);
        m_io = m_source->start();
        if (!m_io) {
            m_source.reset();
            m_energy.reset();
            m_corr.reset();
            emit deviceFailed();
            return;
        }
        connect(m_io, &QIODevice::readyRead, this, &CaptureWorker::onReadyRead);
        m_running = true;
        emit phaseUpdated(SessionApi::Listening, 0, m_epoch);
        LOG_INFO() << "Session started" << m_sessionId;
    }

    void stopCapture()
    {
        if (!m_running)
            return;
        if (m_phraseSpeaking)
            closePhrase(false, true);
        finishAudio();
        Job end;
        end.kind = JobKind::EndSession;
        end.generation = m_generation;
        end.sessionId = m_sessionId;
        end.sessionEndedAt = stamp(QDateTime::currentDateTime());
        m_queue->pushOrdered(end);
        emit runningChanged(false);
        LOG_INFO() << "Session stopped" << m_sessionId;
    }

    void shutdown()
    {
        stopCapture();
    }

signals:
    void phaseUpdated(int phase, int seconds, int epoch);
    void levelUpdated(qreal level);
    void runningChanged(bool running);
    void deviceFailed();

private slots:
    void onReadyRead()
    {
        if (!m_running || !m_io)
            return;
        const QByteArray data = m_io->readAll();
        if (data.isEmpty())
            return;
        if (!m_settingsTimer.isValid() || m_settingsTimer.elapsed() >= 250) {
            m_settings = Settings::loadSettings();
            m_settingsTimer.restart();
        }

        const std::vector<qint16> samples = m_resampler.process(data);
        if (samples.empty())
            return;

        qint64 absSum = 0;
        for (qint16 sample : samples)
            absSum += std::abs(static_cast<int>(sample));
        const qreal level = std::min(1.0, (static_cast<double>(absSum) / samples.size()) / 4000.0);
        emit levelUpdated(level);

        m_pcm.insert(m_pcm.end(), samples.begin(), samples.end());
        m_fed += static_cast<qint64>(samples.size());

        if (!m_useSpeechDetection) {
            // The whole take is one phrase. Numbers update from the first sample
            // and a pause does not close it.
            if (!m_phraseSpeaking)
                beginPhrase(0, m_fed);
            else
                m_speechEnd = m_fed;
            cutIfTooLong();
            notePhase();
            publishLive();
            trim();
            return;
        }

        const std::vector<double> energy = m_energy->processAudioSamples(samples.data(), static_cast<qint64>(samples.size()));
        const std::vector<double> correlation = m_corr->processAudioSamples(samples.data(), static_cast<qint64>(samples.size()));
        if (m_settings.vadMethod == 1) {
            const int first = m_corr->getValidUIndex() - static_cast<int>(correlation.size());
            for (std::size_t index = 0; index < correlation.size(); ++index)
                onFrame(first + static_cast<int>(index), 0, correlation[index]);
        } else if (m_settings.vadMethod == 0) {
            const int first = m_energy->getValidVIndex() - static_cast<int>(energy.size());
            for (std::size_t index = 0; index < energy.size(); ++index)
                onFrame(first + static_cast<int>(index), energy[index], 0);
        } else {
            const int first = m_energy->getValidVIndex() - static_cast<int>(energy.size());
            for (std::size_t index = 0; index < energy.size(); ++index) {
                const int frame = first + static_cast<int>(index);
                onFrame(frame, energy[index], m_corr->getU(frame));
            }
        }
        trim();
    }

private:
    enum class CloseKind { Pause, Manual, MaxTime };

    void finishAudio()
    {
        m_running = false;
        if (m_io)
            disconnect(m_io, nullptr, this, nullptr);
        if (m_source)
            m_source->stop();
        m_source.reset();
        m_io = nullptr;
        m_energy.reset();
        m_corr.reset();
        emit levelUpdated(0);
    }

    qint64 span() const
    {
        return std::max<qint64>(0, m_speechEnd - m_speechStart);
    }

    bool longEnough() const
    {
        return span() >= kMinPhraseSamples;
    }

    qint64 openSamples() const
    {
        return std::max<qint64>(0, m_speechEnd - leadingStart());
    }

    qint64 windowSamples() const
    {
        return static_cast<qint64>(std::clamp(m_settings.analysisWindowSec, 3, 30)) * kSampleRate;
    }

    bool windowReady() const
    {
        return static_cast<qint64>(m_recent.size()) + openSamples() >= kMinWindowSamples;
    }

    int liveIntervalMs() const
    {
        return 60000 / std::clamp(m_settings.updatesPerMinute, 6, 240);
    }

    void bumpEpoch()
    {
        ++m_epoch;
        m_queue->setPhraseEpoch(m_epoch);
    }

    void onFrame(int frameIndex, double energy, double correlation)
    {
        if (frameIndex < 0 || !m_running)
            return;

        const bool inPhrase = m_phraseSpeaking;
        const bool energySpeech = m_energy->isSpeech(energy, inPhrase);
        const bool correlationSpeech = m_corr->isSpeech(correlation, inPhrase);
        bool speech = energySpeech;
        if (m_settings.vadMethod == 1)
            speech = correlationSpeech;
        else if (m_settings.vadMethod == 2)
            speech = energySpeech && correlationSpeech;
        else if (m_settings.vadMethod == 3)
            speech = energySpeech || correlationSpeech;

        const qint64 frameStart = static_cast<qint64>(frameIndex) * kHop;
        const qint64 frameEnd = frameStart + kVadFrame;
        if (speech) {
            m_silenceFrames = 0;
            if (!m_phraseSpeaking)
                beginPhrase(frameStart, frameEnd);
            else {
                m_speechEnd = std::max(m_speechEnd, frameEnd);
                cutIfTooLong();
                notePhase();
                publishLive();
            }
        } else if (m_phraseSpeaking) {
            ++m_silenceFrames;
            const int silenceMs = (m_silenceFrames * kHop * 1000) / kSampleRate;
            if (silenceMs >= m_settings.autoStopSilenceDuration)
                closePhrase(true, false);
        }
    }

    void beginPhrase(qint64 start, qint64 end)
    {
        m_phraseSpeaking = true;
        m_padLeading = true;
        m_dropped = false;
        m_speechStart = start;
        m_speechEnd = end;
        m_phraseStartedAt = QDateTime::currentDateTime();
        m_silenceFrames = 0;
        bumpEpoch();
        m_liveTimer.invalidate();
        notePhase();
    }

    void cutIfTooLong()
    {
        while (m_phraseSpeaking && span() >= kMaxSegmentSamples) {
            const qint64 cut = m_speechStart + kMaxSegmentSamples;
            const std::vector<float> buffer = extract(leadingStart(), cut);
            const QDateTime ended = QDateTime::currentDateTime();
            postFinal(buffer, m_phraseStartedAt, ended);
            m_speechStart = cut;
            m_padLeading = false;
            m_phraseStartedAt = ended;
            if (m_speechEnd < m_speechStart)
                m_speechEnd = m_speechStart;
            bumpEpoch();
            m_liveTimer.invalidate();
        }
    }

    void closePhrase(bool pause, bool sessionEnding)
    {
        const bool keep = longEnough();
        if (keep) {
            const qint64 end = m_speechEnd + kPadSamples;
            postFinal(extract(leadingStart(), end), m_phraseStartedAt, QDateTime::currentDateTime());
        } else if (pause) {
            m_dropped = true;
        }
        m_phraseSpeaking = false;
        m_speechStart = 0;
        m_speechEnd = 0;
        m_silenceFrames = 0;
        bumpEpoch();
        if (!sessionEnding)
            notePhase();
    }

    void notePhase()
    {
        if (!m_running)
            return;
        int phase = SessionApi::Listening;
        int seconds = 0;
        if (m_phraseSpeaking) {
            seconds = static_cast<int>(span() / kSampleRate);
            phase = windowReady() ? SessionApi::Measuring : SessionApi::TooShort;
        } else if (m_dropped) {
            phase = SessionApi::Dropped;
        }
        if (phase == m_sentPhase && seconds == m_sentSeconds && m_epoch == m_sentEpoch)
            return;
        m_sentPhase = phase;
        m_sentSeconds = seconds;
        m_sentEpoch = m_epoch;
        emit phaseUpdated(phase, seconds, m_epoch);
    }

    void publishLive()
    {
        if (!m_phraseSpeaking || !windowReady())
            return;
        if (m_liveTimer.isValid() && m_liveTimer.elapsed() < liveIntervalMs())
            return;
        m_liveTimer.start();
        const std::vector<float> open = extract(leadingStart(), m_speechEnd);
        const std::size_t window = static_cast<std::size_t>(windowSamples());
        Job job;
        job.kind = JobKind::Live;
        if (open.size() >= window) {
            job.samples.assign(open.end() - static_cast<std::ptrdiff_t>(window), open.end());
        } else {
            const std::size_t fromRecent = std::min(m_recent.size(), window - open.size());
            job.samples.reserve(fromRecent + open.size());
            job.samples.insert(job.samples.end(),
                m_recent.end() - static_cast<std::ptrdiff_t>(fromRecent), m_recent.end());
            job.samples.insert(job.samples.end(), open.begin(), open.end());
        }
        job.openSeconds = static_cast<double>(open.size()) / kSampleRate;
        job.config = configFrom(m_settings);
        job.fillerMin = m_settings.fillerMin;
        job.fillerMax = m_settings.fillerMax;
        job.phraseEpoch = m_epoch;
        job.generation = m_generation;
        job.sessionId = m_sessionId;
        job.sessionStartedAt = m_sessionStartedAt;
        m_queue->pushLive(std::move(job));
    }

    void postFinal(const std::vector<float>& samples, const QDateTime& started, const QDateTime& ended)
    {
        if (samples.empty())
            return;
        m_recent.insert(m_recent.end(), samples.begin(), samples.end());
        if (m_recent.size() > static_cast<std::size_t>(kMaxWindowSamples))
            m_recent.erase(m_recent.begin(), m_recent.end() - static_cast<std::ptrdiff_t>(kMaxWindowSamples));
        Job job;
        job.kind = JobKind::Final;
        job.samples = samples;
        job.config = configFrom(m_settings);
        job.fillerMin = m_settings.fillerMin;
        job.fillerMax = m_settings.fillerMax;
        job.phraseEpoch = m_epoch;
        job.generation = m_generation;
        job.sessionId = m_sessionId;
        job.sessionStartedAt = m_sessionStartedAt;
        job.segmentStartedAt = stamp(started);
        job.segmentEndedAt = stamp(ended);
        m_queue->pushOrdered(std::move(job));
    }

    qint64 leadingStart() const
    {
        if (!m_padLeading)
            return m_speechStart;
        return m_speechStart - kPadSamples;
    }

    std::vector<float> extract(qint64 start, qint64 end) const
    {
        if (start < m_origin)
            start = m_origin;
        const qint64 available = m_origin + static_cast<qint64>(m_pcm.size());
        if (end > available)
            end = available;
        if (end < start)
            end = start;
        std::vector<float> samples;
        samples.reserve(static_cast<std::size_t>(end - start));
        for (qint64 index = start; index < end; ++index)
            samples.push_back(static_cast<float>(m_pcm[static_cast<std::size_t>(index - m_origin)]));
        return samples;
    }

    void trim()
    {
        // Keep a second of audio before the 300 ms pad so a late VAD decision
        // can still include the leading pad.
        qint64 keep = m_fed - (kPadSamples + kSampleRate);
        if (m_phraseSpeaking)
            keep = leadingStart();
        if (keep < m_origin)
            keep = m_origin;
        const qint64 drop = keep - m_origin;
        if (drop <= 0 || drop > static_cast<qint64>(m_pcm.size()))
            return;
        m_pcm.erase(m_pcm.begin(), m_pcm.begin() + static_cast<std::ptrdiff_t>(drop));
        m_origin = keep;
    }

    JobQueue* m_queue = nullptr;
    bool m_running = false;
    AppSettings m_settings;
    QElapsedTimer m_settingsTimer;
    QElapsedTimer m_liveTimer;
    QAudioDevice m_device;
    std::unique_ptr<QAudioSource> m_source;
    QIODevice* m_io = nullptr;
    StreamResampler m_resampler;
    std::unique_ptr<VADEnergyService> m_energy;
    std::unique_ptr<VADAutocorrelationService> m_corr;

    std::vector<qint16> m_pcm;
    // Audio of the segments already closed in this session, newest last.
    // Live windows continue from it across pauses.
    std::deque<float> m_recent;
    qint64 m_origin = 0;
    qint64 m_fed = 0;
    bool m_useSpeechDetection = false;
    bool m_phraseSpeaking = false;
    bool m_padLeading = true;
    bool m_dropped = false;
    qint64 m_speechStart = 0;
    qint64 m_speechEnd = 0;
    int m_silenceFrames = 0;
    int m_epoch = 0;
    int m_sentPhase = -1;
    int m_sentSeconds = -1;
    int m_sentEpoch = -1;
    QDateTime m_phraseStartedAt;
    QString m_sessionId;
    QString m_sessionStartedAt;
    quint64 m_generation = 0;
};

struct SessionApi::SessionAccumulator {
    speechrate::Parts parts;
    speechrate::Config config;
    speechrate::Metrics metrics;
    double fillerMin = 120;
    double fillerMax = 240;
    bool ready = false;
    bool pooled = true;
    double weight = 0;
    double speechRateSum = 0;
    double articulationSum = 0;
    double pausesSum = 0;
    double fillerSum = 0;

    void addPhrase(const speechrate::Metrics& phrase)
    {
        const double used = phrase.speechDuration > 0 ? phrase.speechDuration : 1.0;
        weight += used;
        speechRateSum += phrase.speechRate * used;
        articulationSum += phrase.articulationRate * used;
        pausesSum += phrase.phrasePauses * used;
        fillerSum += phrase.fillerScore * used;
    }

    speechrate::Metrics meanHeadlines() const
    {
        speechrate::Metrics out = metrics;
        if (!(weight > 0))
            return out;
        out.speechRate = speechRateSum / weight;
        out.articulationRate = articulationSum / weight;
        out.phrasePauses = pausesSum / weight;
        out.fillerScore = fillerSum / weight;
        return out;
    }
};

SessionApi::SessionApi(QObject* parent)
    : QObject(parent)
    , m_accumulator(std::make_unique<SessionAccumulator>())
    , m_queue(new JobQueue)
    , m_captureThread(new QThread(this))
    , m_worker(new CaptureWorker(m_queue))
    , m_analysisThread(new AnalysisThread(this, m_queue))
{
    m_worker->moveToThread(m_captureThread);
    connect(m_captureThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &CaptureWorker::phaseUpdated, this, &SessionApi::applyPhase, Qt::QueuedConnection);
    connect(m_worker, &CaptureWorker::levelUpdated, this, &SessionApi::applyLevel, Qt::QueuedConnection);
    connect(m_worker, &CaptureWorker::runningChanged, this, &SessionApi::applyRunning, Qt::QueuedConnection);
    connect(m_worker, &CaptureWorker::deviceFailed, this, &SessionApi::applyDeviceFailed, Qt::QueuedConnection);
    SessionStore::recoverPending();
    m_captureThread->start();
    m_analysisThread->start();
}

SessionApi::~SessionApi()
{
    m_alive.store(false);
    disconnect(m_worker, nullptr, this, nullptr);
    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::BlockingQueuedConnection);
    m_queue->stop();
    m_analysisThread->wait();
    m_captureThread->quit();
    m_captureThread->wait();
    delete m_analysisThread;
    delete m_queue;
    m_worker = nullptr;
}

bool SessionApi::openFileAvailable() const
{
#ifdef Q_OS_ANDROID
    return false;
#else
    return true;
#endif
}

void SessionApi::startSession()
{
    if (m_sessionActive || m_startPending || m_stopPending || m_openFileBusy || m_clearPending || !m_alive.load())
        return;

#ifdef Q_OS_ANDROID
    QMicrophonePermission microphonePermission;
    if (qApp->checkPermission(microphonePermission) != Qt::PermissionStatus::Granted) {
        m_startPending = true;
        qApp->requestPermission(microphonePermission, this, [this](const QPermission& permission) {
            m_startPending = false;
            if (m_clearPending) {
                enqueueClearUserData();
            } else if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted) {
                beginCapture();
            } else {
                applyDeviceFailed();
            }
        });
        return;
    }
#endif
    beginCapture();
}

void SessionApi::beginCapture()
{
    ++m_captureGeneration;
    *m_accumulator = {};
    resetResultState();
    m_shownSessionId.clear();
    m_shownSessionStartedAt.clear();
    m_lastShownKey.clear();
    m_noSpeech = false;
    m_sessionActive = true;
    m_phase = Listening;
    m_phraseSeconds = 0;
    m_audioLevel = 0;
    emit sessionActiveChanged();
    emit phaseChanged();
    emit phraseSecondsChanged();
    emit audioLevelChanged();
    QMetaObject::invokeMethod(m_worker, "startCapture", Qt::QueuedConnection,
        Q_ARG(quint64, m_captureGeneration));
}

void SessionApi::resetResultState()
{
    const bool hadResult = m_hasResult;
    m_hasResult = false;
    setShowingMean(false);
    m_speechRateWindow.clear();
    m_articulationWindow.clear();
    m_speechRate = 0;
    m_articulationRate = 0;
    m_gaugeArticulationRate = 0;
    m_phrasePauses = 0;
    m_speechDuration = 0;
    m_fillerScore = 0;
    m_details.clear();
    if (hadResult)
        emit hasResultChanged();
    emit metricsChanged();
}

void SessionApi::stopSession()
{
    if (!m_sessionActive || m_stopPending)
        return;
    m_stopPending = true;
    QMetaObject::invokeMethod(m_worker, "stopCapture", Qt::QueuedConnection);
}

void SessionApi::openWavFile(const QUrl& url)
{
    if (m_sessionActive || m_startPending || m_stopPending || m_openFileBusy || m_clearPending || !openFileAvailable())
        return;
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    const QString error = path.isEmpty()
        ? QCoreApplication::translate("SessionApi", "The selected file cannot be opened.")
        : validateWavFile(path);
    if (!error.isEmpty()) {
        setOpenFileError(error);
        emit openFileFinished(false, error);
        return;
    }
    ++m_captureGeneration;
    setOpenFileError({});
    m_openFileBusy = true;
    emit openFileBusyChanged();
    emit busyChanged();
    const AppSettings settings = Settings::loadSettings();
    Job job;
    job.kind = JobKind::OpenFile;
    job.path = path;
    job.config = configFrom(settings);
    job.fillerMin = settings.fillerMin;
    job.fillerMax = settings.fillerMax;
    job.generation = m_captureGeneration;
    m_queue->pushOrdered(std::move(job));
}

void SessionApi::reportMicrophoneDenied()
{
    if (m_sessionActive || m_stopPending)
        return;
    applyDeviceFailed();
}

void SessionApi::clearUserData()
{
    if (m_clearPending)
        return;
    m_clearPending = true;
    emit clearingUserDataChanged();
    emit busyChanged();
    if (m_startPending)
        return;
    if (m_sessionActive || m_stopPending) {
        if (!m_stopPending)
            stopSession();
        return;
    }
    if (m_openFileBusy)
        return;
    enqueueClearUserData();
}

void SessionApi::enqueueClearUserData()
{
    if (m_clearEnqueued)
        return;
    m_clearEnqueued = true;
    Job job;
    job.kind = JobKind::ClearAll;
    m_queue->pushOrdered(std::move(job));
}

void SessionApi::setOpenFileError(const QString& error)
{
    if (m_openFileError == error)
        return;
    m_openFileError = error;
    emit openFileErrorChanged();
}

QUrl SessionApi::testsFolderUrl() const
{
    const QString path = Settings::getAppDataDir() + QStringLiteral("/data/tests");
    QDir().mkpath(path);
    return QUrl::fromLocalFile(path);
}

QVariantList SessionApi::sessions() const
{
    return SessionStore::listSessions();
}

QVariantMap SessionApi::session(const QString& sessionId) const
{
    return SessionStore::loadSession(sessionId);
}

void SessionApi::applyPhase(int phase, int seconds, int epoch)
{
    if (!m_alive.load())
        return;
    m_epoch = epoch;
    if (m_phraseSeconds != seconds) {
        m_phraseSeconds = seconds;
        emit phraseSecondsChanged();
    }
    if (m_phase != phase) {
        m_phase = phase;
        emit phaseChanged();
    }
}

void SessionApi::applyLevel(qreal level)
{
    if (!m_alive.load() || qFuzzyCompare(m_audioLevel, level))
        return;
    m_audioLevel = level;
    emit audioLevelChanged();
}

void SessionApi::applyMetrics(const QVariantMap& metrics)
{
    if (!m_alive.load())
        return;
    if (metrics.value(QStringLiteral("generation")).toULongLong() != m_captureGeneration)
        return;
    const bool valid = metrics.value(QStringLiteral("valid")).toBool();
    const bool live = metrics.value(QStringLiteral("live")).toBool();
    if (live && metrics.value(QStringLiteral("phraseEpoch")).toInt() != m_epoch)
        return;
    const bool sessionPhrase = metrics.value(QStringLiteral("scope")).toString() == QLatin1String("phrase");
    if (!sessionPhrase) {
        if (!valid)
            return;
        m_shownSessionId.clear();
        m_shownSessionStartedAt.clear();
        setShowingMean(false);
        showMetrics(metrics, metrics.value(QStringLiteral("speechDuration")).toDouble());
        return;
    }
    if (live && (!m_sessionActive || m_stopPending))
        return;
    m_shownSessionId = metrics.value(QStringLiteral("sessionId")).toString();
    m_shownSessionStartedAt = metrics.value(QStringLiteral("sessionStartedAt")).toString();

    SessionAccumulator& acc = *m_accumulator;
    const double storedSpeech = acc.ready ? acc.metrics.speechDuration : 0.0;
    if (live) {
        setNoSpeech(!valid);
        if (valid)
            showMetrics(metrics, storedSpeech + metrics.value(QStringLiteral("openSeconds")).toDouble());
        return;
    }
    if (!valid)
        return;

    speechrate::Parts parts;
    speechrate::Config config;
    double fillerMin = 0;
    double fillerMax = 0;
    if (!SessionStore::takeMeasurement(metrics, parts, config, fillerMin, fillerMax))
        return;

    const speechrate::Metrics phrase = speechrate::metricsFromLengths(parts.vowelLengths,
        parts.gapLengths,
        parts.speechDuration,
        config,
        parts.vowelMaxFrames,
        parts.gapMaxFrames);

    if (!acc.ready || (acc.pooled && speechrate::sameMeasurement(acc.config, config))) {
        if (!acc.ready)
            acc.parts = parts;
        else
            speechrate::appendParts(acc.parts, parts);
        acc.config = acc.ready ? acc.config : config;
        acc.pooled = true;
        acc.fillerMin = fillerMin;
        acc.fillerMax = fillerMax;
        acc.metrics = speechrate::metricsFromLengths(acc.parts.vowelLengths,
            acc.parts.gapLengths,
            acc.parts.speechDuration,
            acc.config,
            acc.parts.vowelMaxFrames,
            acc.parts.gapMaxFrames);
        acc.ready = acc.metrics.valid;
    } else {
        acc.pooled = false;
        acc.fillerMin = fillerMin;
        acc.fillerMax = fillerMax;
        acc.metrics = speechrate::blendMetrics(acc.metrics, phrase);
        acc.ready = acc.metrics.valid;
    }
    if (!acc.ready)
        return;
    if (phrase.valid)
        acc.addPhrase(phrase);
    if (!m_hasResult) {
        showMetrics(metricsToMap(acc.metrics, acc.fillerMin, acc.fillerMax, false, 0, true),
            acc.metrics.speechDuration);
        return;
    }
    if (m_speechDuration != acc.metrics.speechDuration) {
        m_speechDuration = acc.metrics.speechDuration;
        emit metricsChanged();
        rememberShown(m_details, m_speechDuration);
    }
}

void SessionApi::applySessionEnded(quint64 generation)
{
    if (!m_alive.load() || generation != m_captureGeneration || (m_sessionActive && !m_stopPending))
        return;
    if (!m_accumulator->ready)
        return;
    showSessionMean();
}

void SessionApi::showSessionMean()
{
    const SessionAccumulator& acc = *m_accumulator;
    if (!acc.ready)
        return;
    // Headlines are the duration-weighted mean of kept phrases. Speech duration
    // stays the sum. Vowel and gap statistics stay those of the joined collection.
    const speechrate::Metrics mean = acc.meanHeadlines();
    QVariantMap shown = metricsToMap(mean, acc.fillerMin, acc.fillerMax, false, 0, true);
    shown.insert(QStringLiteral("speechDuration"), acc.metrics.speechDuration);
    m_shownSessionId.clear();
    showMetrics(shown, acc.metrics.speechDuration);
    setShowingMean(acc.weight > 0);
}

void SessionApi::setShowingMean(bool showing)
{
    if (m_showingMean == showing)
        return;
    m_showingMean = showing;
    emit showingMeanChanged();
}

void SessionApi::setNoSpeech(bool noSpeech)
{
    if (m_noSpeech == noSpeech)
        return;
    const int before = phase();
    m_noSpeech = noSpeech;
    if (phase() != before)
        emit phaseChanged();
}

double SessionApi::medianOf(std::deque<double>& window, double rate, bool live)
{
    if (!live) {
        window.clear();
        return rate;
    }

    const int count = std::clamp(Settings::loadSettings().gaugeAverageCount, 1, 30);
    window.push_back(rate);
    while (static_cast<int>(window.size()) > count)
        window.pop_front();

    std::vector<double> samples(window.begin(), window.end());
    std::sort(samples.begin(), samples.end());
    const std::size_t mid = samples.size() / 2;
    if (samples.size() % 2 == 1)
        return samples[mid];
    return (samples[mid - 1] + samples[mid]) / 2.0;
}

void SessionApi::showMetrics(const QVariantMap& shown, double speechSeconds)
{
    const bool live = shown.value(QStringLiteral("live")).toBool();
    m_speechRate = medianOf(m_speechRateWindow, shown.value(QStringLiteral("speechRate")).toDouble(), live);
    m_articulationRate = shown.value(QStringLiteral("articulationRate")).toDouble();
    m_gaugeArticulationRate = medianOf(m_articulationWindow, m_articulationRate, live);
    m_phrasePauses = shown.value(QStringLiteral("phrasePauses")).toDouble();
    m_speechDuration = speechSeconds;
    m_fillerScore = shown.value(QStringLiteral("fillerScore")).toDouble();
    m_details = shown;
    if (!m_hasResult) {
        m_hasResult = true;
        emit hasResultChanged();
    }
    emit metricsChanged();
    rememberShown(shown, speechSeconds);
    if (!m_sessionActive && m_phase == IdleEmpty) {
        m_phase = IdleReady;
        emit phaseChanged();
    }
}

void SessionApi::rememberShown(const QVariantMap& shown, double speechSeconds)
{
    if (m_shownSessionId.isEmpty() || !shown.value(QStringLiteral("valid"), true).toBool())
        return;

    const AppSettings settings = Settings::loadSettings();
    const int speechRate = roundHalfAway(m_speechRate);
    // The chart stores the articulation number Home was showing: the gauge median
    // when the gauge draws it, otherwise the raw tile value.
    const int articulation = roundHalfAway(settings.gaugeMode == 0
        ? shown.value(QStringLiteral("articulationRate")).toDouble()
        : m_gaugeArticulationRate);
    const int pauses = roundHalfAway(shown.value(QStringLiteral("phrasePauses")).toDouble() * 100.0);
    const int duration = roundHalfAway(speechSeconds);
    const int fillers = roundHalfAway(speechrate::fillerPercent(
        shown.value(QStringLiteral("fillerScore")).toDouble(), settings.fillerMin, settings.fillerMax));
    const QString key = QString::number(speechRate) + QLatin1Char('|')
        + QString::number(articulation) + QLatin1Char('|')
        + QString::number(pauses) + QLatin1Char('|')
        + QString::number(duration) + QLatin1Char('|')
        + QString::number(fillers);
    if (key == m_lastShownKey)
        return;

    QVariantMap sample;
    sample.insert(QStringLiteral("at"), stamp(QDateTime::currentDateTime()));
    sample.insert(QStringLiteral("speechRate"), speechRate);
    sample.insert(QStringLiteral("articulationRate"), articulation);
    sample.insert(QStringLiteral("phrasePauses"), pauses / 100.0);
    sample.insert(QStringLiteral("speechDuration"), duration);
    sample.insert(QStringLiteral("fillerPercent"), fillers);
    if (!SessionStore::appendShown(m_shownSessionId, m_shownSessionStartedAt, sample))
        return;
    m_lastShownKey = key;
    notifySessions();
}

void SessionApi::applyRunning(bool running)
{
    if (!m_alive.load())
        return;
    m_stopPending = false;
    if (!running) {
        m_noSpeech = false;
        m_audioLevel = 0;
        emit audioLevelChanged();
        m_phraseSeconds = 0;
        emit phraseSecondsChanged();
        const int phase = m_hasResult ? IdleReady : IdleEmpty;
        if (m_phase != phase) {
            m_phase = phase;
            emit phaseChanged();
        }
    }
    if (m_sessionActive != running) {
        m_sessionActive = running;
        emit sessionActiveChanged();
    }
    if (!running && m_clearPending && !m_openFileBusy)
        enqueueClearUserData();
}

void SessionApi::applyDeviceFailed()
{
    if (!m_alive.load())
        return;
    m_stopPending = false;
    m_sessionActive = false;
    m_audioLevel = 0;
    m_phase = MicDenied;
    emit sessionActiveChanged();
    emit audioLevelChanged();
    emit phaseChanged();
    if (m_clearPending && !m_openFileBusy)
        enqueueClearUserData();
}

void SessionApi::applyOpenFileFinished(bool success, const QString& error)
{
    if (!m_alive.load() || !m_openFileBusy)
        return;
    m_openFileBusy = false;
    emit openFileBusyChanged();
    emit busyChanged();
    setOpenFileError(error);
    emit openFileFinished(success, error);
    if (m_clearPending)
        enqueueClearUserData();
}

void SessionApi::applyUserDataCleared()
{
    if (!m_alive.load() || !m_clearPending)
        return;
    m_clearEnqueued = false;
    ++m_captureGeneration;
    *m_accumulator = {};
    resetResultState();
    m_shownSessionId.clear();
    m_shownSessionStartedAt.clear();
    m_lastShownKey.clear();
    if (m_phase != IdleEmpty) {
        m_phase = IdleEmpty;
        emit phaseChanged();
    }
    m_clearPending = false;
    emit clearingUserDataChanged();
    emit busyChanged();
    emit sessionsChanged();
    emit userDataCleared();
}

void SessionApi::notifySessions()
{
    if (m_alive.load())
        emit sessionsChanged();
}

#include "sessionapi.moc"
