#include "audioapi.h"
#include <QPointF>
#include "helpers/logger.h"
#include "helpers/settings.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QIODevice>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <memory>

#include <QAudioOutput>
#include <QEventLoop>
#include <QMediaPlayer>
#include <QTimer>

AudioApi::AudioApi(QObject* parent)
    : QObject(parent)
{
    m_audioDevice = QMediaDevices::defaultAudioInput();
    m_format.setSampleRate(8000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
    #ifdef Q_OS_ANDROID
    m_wavFileService = std::unique_ptr<WavFileService>(new WavFileService(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()));
#else
    m_wavFileService = std::unique_ptr<WavFileService>(new WavFileService(
        QCoreApplication::applicationDirPath().toStdString()));
#endif

    m_vadService = std::unique_ptr<VADEnergyService>(new VADEnergyService(this));
    m_vadAutocorrService = std::unique_ptr<VADAutocorrelationService>(new VADAutocorrelationService(this));

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
        [this](QMediaPlayer::PlaybackState state) {
            bool isPlaying = (state == QMediaPlayer::PlayingState);
            if (m_isPlaying != isPlaying) {
                m_isPlaying = isPlaying;
                emit isPlayingChanged();
            }
        });

    // Forward BeepPlayer finished signal
    connect(&m_beepPlayer, &BeepPlayer::finished, this, &AudioApi::beepFinished);
}

bool AudioApi::isRecording() const
{
    LOG_DEBUG() << "Start: isRecording";
    bool result = m_isRecording;
    LOG_DEBUG() << "Finish: isRecording - result=" << result;
    return result;
}

bool AudioApi::isPlaying() const
{
    LOG_DEBUG() << "Start: isPlaying";
    bool result = m_isPlaying;
    LOG_DEBUG() << "Finish: isPlaying - result=" << result;
    return result;
}

bool AudioApi::isVoiceDetected() const
{
    return m_voiceDetected;
}

qreal AudioApi::audioLevel() const
{
    qreal result = m_audioLevel;
    return result;
}

QVariantList AudioApi::getVadA() const
{
    QVariantList list;
    const auto& savedA = m_vadService->getSavedA();
    for (int i = 0; i < static_cast<int>(savedA.size()); ++i)
        list.append(QPointF(i, savedA[i]));
    return list;
}

QVariantList AudioApi::getVadU() const
{
    QVariantList list;
    const auto& savedU = m_vadService->getSavedU();
    for (int i = 0; i < static_cast<int>(savedU.size()); ++i)
        list.append(QPointF(i, savedU[i]));
    return list;
}

QVariantList AudioApi::getVadV() const
{
    QVariantList list;
    const auto& savedV = m_vadService->getSavedV();
    for (int i = 0; i < static_cast<int>(savedV.size()); ++i)
        list.append(QPointF(i, savedV[i]));
    return list;
}

QVariantList AudioApi::getVadCorr() const
{
    QVariantList list;
    const auto& savedCorr = m_vadAutocorrService->getSavedCorrelations();
    for (int i = 0; i < static_cast<int>(savedCorr.size()); ++i)
        list.append(QPointF(i, savedCorr[i]));
    return list;
}

QVariantList AudioApi::getVadCorrU() const
{
    QVariantList list;
    const auto& savedU = m_vadAutocorrService->getSavedU();
    for (int i = 0; i < static_cast<int>(savedU.size()); ++i)
        list.append(QPointF(i, savedU[i]));
    return list;
}

QVariantList AudioApi::getVadCorrV() const
{
    // V(n) is no longer calculated, return empty list
    return QVariantList();
}

void AudioApi::setVadMethod(int method)
{
    if (m_vadMethod == method)
        return;
    m_vadMethod = method;
    emit vadMethodChanged();
}

void AudioApi::play(const QString& filePath)
{
    LOG_DEBUG() << "Start: play - filePath=" << filePath;
    if (!m_player) {
        LOG_CRITICAL() << "QMediaPlayer is not initialized";
        return;
    }
    QString fullPath = QDir(Settings::getAppDataDir()).filePath(filePath);
    m_player->setSource(QUrl::fromLocalFile(fullPath));
    m_player->play();
    LOG_DEBUG() << "Finish: play";
}

void AudioApi::stopPlayback()
{
    LOG_DEBUG() << "Start: stopPlayback";
    if (m_player) {
        m_player->stop();
    }
    LOG_DEBUG() << "Finish: stopPlayback";
}

void AudioApi::playBeep(double frequencyHz, int durationMs, double amplitude)
{
    LOG_DEBUG() << "Start: playBeep - freq=" << frequencyHz
                << " dur=" << durationMs << " amp=" << amplitude;
    m_beepPlayer.playBeep(frequencyHz, durationMs, amplitude);
    LOG_DEBUG() << "Finish: playBeep";
}

void AudioApi::playDoubleBeep(double freq1, int dur1, double freq2, int dur2, double amp)
{
    LOG_DEBUG() << "Start: playDoubleBeep - freq1=" << freq1 << " dur1=" << dur1
                << " freq2=" << freq2 << " dur2=" << dur2 << " amp=" << amp;
    m_beepPlayer.playBeep(freq1, dur1, amp, freq2, dur2);
    LOG_DEBUG() << "Finish: playDoubleBeep";
}

void AudioApi::setAudioLevel(qreal level)
{
    if (qFuzzyCompare(m_audioLevel, level)) {
        return;
    }
    m_audioLevel = level;
    emit isAudioLevelChanged();
}

bool AudioApi::requestAudioPermission()
{
#ifdef Q_OS_ANDROID
    // Qt 6.5+ QPermission API for runtime permission requests
    QMicrophonePermission microphonePermission;
    auto *app = qApp;
    if (!app) return false;
    if (app->checkPermission(microphonePermission) == Qt::PermissionStatus::Granted) {
        return true;
    }

    // Request the permission asynchronously — result arrives via
    // permissionResultReceived signal.
    app->requestPermission(microphonePermission, this,
        [this](const QPermission &permission) {
            bool granted = (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted);
            if (!granted) {
                LOG_WARNING() << "RECORD_AUDIO permission denied by user";
            }
            emit permissionResultReceived(granted);
        });
    return false;
#else
    // Desktop — no runtime permission needed
    return true;
#endif
}

void AudioApi::startRecording(int durationSeconds, int minimumRecordLength)
{
    LOG_DEBUG() << "Start: startRecording - durationSeconds=" << durationSeconds;
    if (m_isRecording) {
        LOG_DEBUG() << "Finish: startRecording - Already recording";
        return;
    }

    AppSettings settings = Settings::loadSettings();
    m_autoStopEnabled = settings.autoStopRecording;
    m_silenceDurationMs = settings.autoStopSilenceDuration;
    m_vadMethod = settings.vadMethod;
    
    m_firstSpeechFrame = -1;
    m_lastSpeechFrame = -1;
    m_silenceFramesCount = 0;
    m_minimumRecordLength = minimumRecordLength;

    // Initialize VAD service
    m_vadService->reset();
    if (settings.vadThreshold > 0) {
        m_vadService->setThreshold(settings.vadThreshold);
    } else {
        m_vadService->setThreshold(0.0);
    }

    m_vadAutocorrService->reset();
    m_vadAutocorrService->setSampleRate(m_format.sampleRate());
    m_vadAutocorrService->setPitchRange(settings.autoCorrMinF0, settings.autoCorrMaxF0);

    if (settings.autoCorrThreshold > 0) {
        // Set hysteresis thresholds with smaller gap to include post-silence
        // Smaller gap allows staying in speech state longer, including silence at end
        double high = settings.autoCorrThreshold;         // Use setting as HIGH threshold
        double low = std::max(0.0, high - 0.1);          // LOW = HIGH - 0.1 for smaller gap
        m_vadAutocorrService->setVoiceThresholdHigh(high);
        m_vadAutocorrService->setVoiceThresholdLow(low);
    } else {
        // Use defaults: HIGH=0.45, LOW=0.35 (gap=0.1)
        m_vadAutocorrService->setVoiceThresholdHigh(0.45);
        m_vadAutocorrService->setVoiceThresholdLow(0.35);
    }
    // Use calibrated energy threshold from settings, fallback to default if not set
    double energyThreshold = settings.autoCorrEnergyThreshold > 0 ? settings.autoCorrEnergyThreshold : 0.02;
    m_vadAutocorrService->setEnergyThreshold(energyThreshold);

    if (m_voiceDetected) {
        m_voiceDetected = false;
        emit isVoiceDetectedChanged();
    }

    m_buffer.clear();
    m_bufferOffsetBytes = 0;
    setAudioLevel(0.0);

    m_audioSource = std::unique_ptr<QAudioSource>(new QAudioSource(m_audioDevice, m_format, this));
    QIODevice* io = m_audioSource->start();

    connect(io, &QIODevice::readyRead, this, [this, io, durationSeconds]() {
        const qint64 len = m_audioSource->bytesAvailable();
        QByteArray data = io->read(len);
        m_buffer.append(data);

        qint64 numSamples = data.size() / (m_format.bytesPerSample());
        qint64 sumValue = 0;
        if (numSamples > 0) {
            const auto samples = reinterpret_cast<const qint16*>(data.constData());
            for (qint64 i = 0; i < numSamples; ++i) {
                sumValue += std::abs(static_cast<int>(samples[i]));
            }
            // Calculate VAD metrics using services
            std::vector<double> newVValues = m_vadService->processAudioSamples(samples, numSamples);
            std::vector<double> newCorrValues = m_vadAutocorrService->processAudioSamples(samples, numSamples);
            
            // Process new VAD frames for auto-stop if enabled
            if (m_autoStopEnabled) {
                const int validEnergyV = m_vadService->getValidVIndex();
                const int validAutocorrU = m_vadAutocorrService->getValidUIndex();

                if (m_vadMethod == 1) {
                    // Autocorr only — U(n) is one stage earlier than energy V(n)
                    const int firstFrame = validAutocorrU - static_cast<int>(newCorrValues.size());
                    for (size_t i = 0; i < newCorrValues.size(); ++i) {
                        processVadFrame(firstFrame + static_cast<int>(i), 0.0, newCorrValues[i]);
                    }
                } else if (m_vadMethod == 0) {
                    const int firstFrame = validEnergyV - static_cast<int>(newVValues.size());
                    for (size_t i = 0; i < newVValues.size(); ++i) {
                        processVadFrame(firstFrame + static_cast<int>(i), newVValues[i], 0.0);
                    }
                } else {
                    // Hybrid AND/OR — use energy frame index; U at same index is always ready
                    const int firstFrame = validEnergyV - static_cast<int>(newVValues.size());
                    for (size_t i = 0; i < newVValues.size(); ++i) {
                        const int frameIdx = firstFrame + static_cast<int>(i);
                        processVadFrame(frameIdx, newVValues[i],
                                        m_vadAutocorrService->getU(frameIdx));
                    }
                }
            }
        }
        double avgValue = (numSamples > 0) ? (double)sumValue / numSamples : 0.0;
        double level = avgValue / 4000.0;
        setAudioLevel(level > 1.0 ? 1.0 : level);

        if (durationSeconds > 0) {
            const int maxBufferSize = m_format.sampleRate() * m_format.bytesPerSample() * durationSeconds;
            if (m_buffer.size() > maxBufferSize) {
                int removeBytes = m_buffer.size() - maxBufferSize;
                m_buffer = m_buffer.right(maxBufferSize);
                m_bufferOffsetBytes += removeBytes;
            }
        }
    });

    m_isRecording = true;
    emit isRecordingChanged();

    LOG_DEBUG() << "Finish: startRecording - Recording started";
}

void AudioApi::processVadFrame(int frameIndex, double V_n, double correlation)
{
    if (m_vadService->addCalibrationFrame(V_n)) {
        // Calibration just completed
        double threshold = m_vadService->getThreshold();
        LOG_DEBUG() << "AutoStop: Calibrated threshold Pe=" << threshold;
    }

    // Use hysteresis by passing current state to isSpeech
    bool energySpeech = m_vadService->isSpeech(V_n, m_voiceDetected);
    bool autocorrSpeech = m_vadAutocorrService->isSpeech(correlation, m_voiceDetected);

    bool isSpeech = false;
    if (m_vadMethod == 0) { // Energy
        isSpeech = energySpeech;
    } else if (m_vadMethod == 1) { // Autocorr (now with hysteresis!)
        isSpeech = autocorrSpeech;
    } else if (m_vadMethod == 2) { // Hybrid AND
        isSpeech = energySpeech && autocorrSpeech;
    } else if (m_vadMethod == 3) { // Hybrid OR
        isSpeech = energySpeech || autocorrSpeech;
    }

    if (isSpeech) {
        if (!m_voiceDetected) {
            m_voiceDetected = true;
            emit isVoiceDetectedChanged();
            m_firstSpeechFrame = frameIndex;
            LOG_DEBUG()
                << "AutoStop: Voice detected at frame " << frameIndex
                << " | V=" << QString::number(V_n, 'f', 2)
                << " corr=" << QString::number(correlation, 'f', 3)
                << " | autoStop=" << m_silenceDurationMs << "ms";
        }
        m_lastSpeechFrame = frameIndex;
        m_silenceFramesCount = 0;
    } else if (m_voiceDetected) {
        m_silenceFramesCount++;
        // 1 frame corresponds to a 64-sample advance (8ms at 8kHz).
        int silenceDuration = (m_silenceFramesCount * 64 * 1000) / m_format.sampleRate();
        
        // Log silence counter periodically (every 5 silence frames = 40ms)
        if (m_silenceFramesCount % 5 == 0) {
            LOG_DEBUG() << QString("AutoStop: Silence cumulated: %1ms / %2ms threshold")
                            .arg(silenceDuration)
                            .arg(m_silenceDurationMs);
        }
        
        if (silenceDuration >= m_silenceDurationMs) {
            LOG_DEBUG() << QString("AutoStop: Silence limit reached (%1ms). Stopping.")
                            .arg(silenceDuration);
            stopRecording(true);
        }
    }
}

void AudioApi::calibrateVadEnergy()
{
    LOG_DEBUG() << "Start: calibrateVad";
    if (m_isRecording) {
        LOG_DEBUG() << "Finish: calibrateVad - Already recording, skipping";
        return;
    }

    // Create a temporary VAD service for calibration
    auto calibrationService = std::make_unique<VADEnergyService>(nullptr);
    calibrationService->prepareForCalibration();

    QAudioFormat fmt = m_format;
    auto source = std::unique_ptr<QAudioSource>(new QAudioSource(m_audioDevice, fmt, this));
    QIODevice* io = source->start();
    if (!io) {
        LOG_DEBUG() << "Finish: calibrateVad - Failed to open audio device";
        emit calibrationFinishedEnergy(50000.0);
        return;
    }

    // Record for 2 seconds synchronously using a local event loop
    AppSettings settings = Settings::loadSettings();
    QTimer stopTimer;
    stopTimer.setSingleShot(true);
    stopTimer.setInterval(settings.vadCalibrationDurationMs);

    QEventLoop loop;
    // Vector to collect frame energies (R0) for energy threshold calculation
    std::vector<double> r0Values;
    connect(&stopTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    connect(io, &QIODevice::readyRead, this, [this, io, calibrationService = calibrationService.get(), fmt, &r0Values]() {
        QByteArray data = io->readAll();
        qint64 numSamples = data.size() / fmt.bytesPerSample();
        if (numSamples > 0) {
            const auto samples = reinterpret_cast<const qint16*>(data.constData());
            // Process audio samples through calibration service
            calibrationService->processAudioSamples(samples, numSamples);
            
            // Also collect frame energies (R0) for energy threshold calculation
            // Process the same samples to compute frame energies
            const int frameSize = 256; // FRAME_SIZE from vadautocorrelationservice.cpp
            const int hopSize = 64;    // HOP_SIZE from vadautocorrelationservice.cpp
            
            for (int i = 0; i + frameSize <= numSamples; i += hopSize) {
                double r0 = 0.0;
                for (int j = 0; j < frameSize; ++j) {
                    double sample = static_cast<double>(samples[i + j]) / 32768.0;
                    r0 += sample * sample;
                }
                r0Values.push_back(r0);
            }
        }
    });

    stopTimer.start();
    loop.exec();

    source->stop();

    // Get all V values and calculate threshold
    std::vector<double> vValues = calibrationService->getAndClearRecentVValues();
    double threshold = VADEnergyService::calculateThresholdFromValues(vValues);
    
    LOG_DEBUG() << "Calibration done: threshold=" << threshold
                << " (from" << vValues.size() << "frames)";
    emit calibrationFinishedEnergy(threshold);
    LOG_DEBUG() << "Finish: calibrateVad";
}

void AudioApi::calibrateVadAutocorrelation()
{
    LOG_DEBUG() << "Start: calibrateVadAutocorrelation";
    if (m_isRecording) {
        LOG_DEBUG() << "Finish: calibrateVadAutocorrelation - Already recording, skipping";
        return;
    }

    // Create a temporary VAD autocorrelation service for calibration
    auto calibrationService = std::make_unique<VADAutocorrelationService>(nullptr);

    QAudioFormat fmt = m_format;
    auto source = std::unique_ptr<QAudioSource>(new QAudioSource(m_audioDevice, fmt, this));
    QIODevice* io = source->start();
    if (!io) {
        LOG_DEBUG() << "Finish: calibrateVadAutocorrelation - Failed to open audio device";
        emit calibrationFinishedAutocorrelation(0.4);  // Default fallback threshold for autocorrelation
        return;
    }

    // Record for 2 seconds synchronously using a local event loop
    AppSettings settings = Settings::loadSettings();
    QTimer stopTimer;
    stopTimer.setSingleShot(true);
    stopTimer.setInterval(settings.vadCalibrationDurationMs);

    QEventLoop loop;
    // Vector to collect frame energies (R0) for energy threshold calculation
    std::vector<double> r0Values;
    connect(&stopTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Configure service with settings
    calibrationService->setSampleRate(fmt.sampleRate());
    calibrationService->setPitchRange(settings.autoCorrMinF0, settings.autoCorrMaxF0);

    connect(io, &QIODevice::readyRead, this, [this, io, calibrationService = calibrationService.get(), fmt, &r0Values]() {
        QByteArray data = io->readAll();
        qint64 numSamples = data.size() / fmt.bytesPerSample();
        if (numSamples > 0) {
            const auto samples = reinterpret_cast<const qint16*>(data.constData());
            // Process audio samples through calibration service
            calibrationService->processAudioSamples(samples, numSamples);
            
            // Also collect frame energies (R0) for energy threshold calculation
            // Process the same samples to compute frame energies
            const int frameSize = 256; // FRAME_SIZE from vadautocorrelationservice.cpp
            const int hopSize = 64;    // HOP_SIZE from vadautocorrelationservice.cpp
            
            for (int i = 0; i + frameSize <= numSamples; i += hopSize) {
                double r0 = 0.0;
                for (int j = 0; j < frameSize; ++j) {
                    double sample = static_cast<double>(samples[i + j]) / 32768.0;
                    r0 += sample * sample;
                }
                r0Values.push_back(r0);
            }
        }
    });

    stopTimer.start();
    loop.exec();

    source->stop();

    // Get U(n) values and calculate threshold
    calibrationService->updateSavedMetrics();
    const auto& uValues = calibrationService->getSavedU();
    
    double threshold = 0.4;  // Default threshold for autocorrelation [0..1]
    if (!uValues.empty()) {
        // Calculate mean of U(n) values (silence/background noise autocorrelation)
        double sum = 0.0;
        for (double val : uValues) {
            sum += val;
        }
        double mean = sum / static_cast<double>(uValues.size());
        
// Load settings for the threshold multiplier
    AppSettings settings = Settings::loadSettings();
    // Use mean as the threshold (conservative estimate)
    // Autocorrelation is [0, 1], so add a reasonable margin above background noise
    threshold = mean * settings.autoCorrThresholdK;
        
        // Clamp to reasonable range [0.2, 0.6]
        if (threshold < 0.2) threshold = 0.2;
        if (threshold > 0.6) threshold = 0.6;
        
        LOG_DEBUG() << "Autocorrelation calibration:"
                    << "mean U(n)=" << QString::number(mean, 'f', 4)
                    << " threshold=" << QString::number(threshold, 'f', 4)
                    << " (from" << uValues.size() << "frames)";
    } else {
        LOG_DEBUG() << "Autocorrelation calibration: no U(n) values collected, using default threshold";
    }

    // Calculate energy threshold from collected R0 values
    double energyThreshold = 0.0001;  // Default energy threshold
    if (!r0Values.empty()) {
        // Calculate mean of R0 values (frame energies)
        double sumR0 = 0.0;
        for (double val : r0Values) {
            sumR0 += val;
        }
        double meanR0 = sumR0 / static_cast<double>(r0Values.size());
        
        // Use mean + 3*stddev as threshold for energy (similar to energy VAD calibration)
        double sumSqDiff = 0.0;
        for (double val : r0Values) {
            double diff = val - meanR0;
            sumSqDiff += diff * diff;
        }
        double stddevR0 = std::sqrt(sumSqDiff / static_cast<double>(r0Values.size()));
        energyThreshold = meanR0 + 3.0 * stddevR0;
        
        // Clamp to reasonable range [0.0001, 0.1]
        if (energyThreshold < 0.0001) energyThreshold = 0.0001;
        if (energyThreshold > 0.1) energyThreshold = 0.1;
        
        LOG_DEBUG() << "Energy threshold from R0 values:"
                    << "mean R0=" << QString::number(meanR0, 'f', 6)
                    << " stddev R0=" << QString::number(stddevR0, 'f', 6)
                    << " energyThreshold=" << QString::number(energyThreshold, 'f', 6)
                    << " (from" << r0Values.size() << "frames)";
    } else {
        LOG_DEBUG() << "No R0 values collected for energy threshold, using default: " << energyThreshold;
    }

    // Save the energy threshold to settings for use in future recordings
    AppSettings currentSettings = Settings::loadSettings();
    currentSettings.autoCorrEnergyThreshold = energyThreshold;
    Settings::saveSettings(currentSettings);
    LOG_DEBUG() << "Saved autoCorrEnergyThreshold to settings: " << energyThreshold;

    emit calibrationFinishedAutocorrelation(threshold);
    LOG_DEBUG() << "Finish: calibrateVadAutocorrelation";
}

void AudioApi::stopRecording(bool isAutoStop)
{
    LOG_DEBUG() << "Start: stopRecording";
    if (!m_isRecording) {
        LOG_DEBUG() << "Finish: stopRecording - Not recording";
        return;
    }

    if (m_minimumRecordLength > 0) {
        int currentSamples = m_buffer.size() / m_format.bytesPerSample() + 2 * ((PRE_BUFFER_MS * m_format.sampleRate()) / 1000);
        int effectiveLength = currentSamples;

        // If auto-stop is enabled and voice was detected, use speech length instead of total buffer
        // to account for recorded silence at the end
        if (m_autoStopEnabled && m_voiceDetected && m_firstSpeechFrame >= 0 && m_lastSpeechFrame >= 0) {
            int speechSamples = (m_lastSpeechFrame - m_firstSpeechFrame + 1) * 64; // approximate speech length
            effectiveLength = speechSamples;
        }
        
        if (effectiveLength < m_minimumRecordLength) {
            if (isAutoStop) {
                // Reset VAD status to initial - it was high level noise, not voice
                m_voiceDetected = false;
                emit isVoiceDetectedChanged();
                m_firstSpeechFrame = -1;
                m_lastSpeechFrame = -1;
                m_silenceFramesCount = 0;
                LOG_DEBUG() << "Auto-stop: Recording too short (" << effectiveLength << " < " << m_minimumRecordLength << "), resetting VAD and continuing";
                return;
            } else {
                LOG_DEBUG() << "Manual stop: Recording too short (" << effectiveLength << " < " << m_minimumRecordLength << "), skipping stop";
                return;
            }
        }
    }

    if (m_audioSource) {
        m_audioSource->stop();
    }
    m_audioSource.reset();
    setAudioLevel(0.0);

    m_isRecording = false;

    // Crop m_buffer to keep only relevant speech + pre/post buffers
    if (m_autoStopEnabled && m_voiceDetected && m_firstSpeechFrame >= 0 && m_lastSpeechFrame >= 0) {
        int pre_samples = (PRE_BUFFER_MS * m_format.sampleRate()) / 1000;
        int post_samples = (PRE_BUFFER_MS * m_format.sampleRate()) / 1000;
        
        int start_sample = m_firstSpeechFrame * 64 - pre_samples;
        if (start_sample < 0) start_sample = 0;
        
        int end_sample = m_lastSpeechFrame * 64 + 128 + post_samples;
        
        int start_byte_absolute = start_sample * m_format.bytesPerSample();
        int end_byte_absolute = end_sample * m_format.bytesPerSample();
        
        int start_byte = start_byte_absolute - m_bufferOffsetBytes;
        if (start_byte < 0) start_byte = 0;
        
        int end_byte = end_byte_absolute - m_bufferOffsetBytes;
        if (end_byte > m_buffer.size()) end_byte = m_buffer.size();
        
        int length = end_byte - start_byte;
        if (length > 0 && start_byte >= 0 && start_byte + length <= m_buffer.size()) {
            m_buffer = m_buffer.mid(start_byte, length);
            LOG_DEBUG() << "AutoStop: Cropped buffer to length " << length << " bytes";
        }
    }

    // Save current VAD metrics
    m_vadService->updateSavedMetrics();
    m_vadAutocorrService->updateSavedMetrics();

    emit isRecordingChanged();

    // Crop saved metrics to synchronize with m_buffer
    if (m_autoStopEnabled && m_voiceDetected && m_firstSpeechFrame >= 0 && m_lastSpeechFrame >= 0) {
        int pre_samples = (PRE_BUFFER_MS * m_format.sampleRate()) / 1000;
        int post_samples = (PRE_BUFFER_MS * m_format.sampleRate()) / 1000;
        
        int start_frame = (m_firstSpeechFrame * 64 - pre_samples) / 64;
        if (start_frame < 0) start_frame = 0;
        
        int end_frame = (m_lastSpeechFrame * 64 + 128 + post_samples) / 64;
        
        m_vadService->cropSavedMetrics(start_frame, end_frame);
        m_vadAutocorrService->cropSavedMetrics(start_frame, end_frame);
    }

    if (m_voiceDetected) {
        m_voiceDetected = false;
        emit isVoiceDetectedChanged();
    }

    LOG_DEBUG() << "Finish: stopRecording - Recording stopped";
}

QString AudioApi::saveWavFile(QString fileName)
{
    LOG_DEBUG() << "Start: saveWavFile - fileName=" << fileName;
    if (fileName.isEmpty()) {
        fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    }

    std::vector<char> buffer(m_buffer.begin(), m_buffer.end());

    int bitsPerSample = 16;
    switch (m_format.sampleFormat()) {
    case QAudioFormat::UInt8:
        bitsPerSample = 8;
        break;
    case QAudioFormat::Int16:
        bitsPerSample = 16;
        break;
    case QAudioFormat::Int32:
        bitsPerSample = 32;
        break;
    case QAudioFormat::Float:
        bitsPerSample = 32;
        break;
    default:
        bitsPerSample = 16;
    }

    AudioFormat format;
    format.sampleRate = m_format.sampleRate();
    format.channelCount = m_format.channelCount();
    format.bitsPerSample = bitsPerSample;

    std::string result = m_wavFileService->writeWaveFile(fileName.toStdString(), buffer, format);
    QString qResult = QString::fromStdString(result);
    LOG_DEBUG() << "Finish: saveWavFile - result=" << qResult;
    return qResult;
}
