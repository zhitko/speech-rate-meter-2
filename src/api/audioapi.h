#ifndef AUDIOAPI_H
#define AUDIOAPI_H

#include "helpers/beepplayer.h"
#include "src/services/helpers/wavFile.h"
#include "src/services/vadautocorrelationservice.h"
#include "src/services/vadenergryservice.h"
#include "src/services/wavfileservice.h"
#include <QAudioFormat>
#include <QAudioSource>
#include <QMediaDevices>
#include <QObject>
#include <memory>

#include <QMediaPlayer>
#include <QPermission>
#include <vector>

class WavFileService;
class VADEnergyService;
struct WaveFile;

/*
 * AudioApi class provides methods for recording audio from the microphone,
 * saving it as a WAV file, and playing back audio files. It uses Qt's
 * multimedia framework to handle audio input and output. The class maintains
 * the state of recording and playback, as well as the current audio level for
 * visual feedback in the UI.
 *
 * The startRecording method begins recording audio, optionally for a specified
 * duration. The stopRecording method stops the recording process. The
 * saveWavFile method saves the recorded audio to a WAV file with an optional
 * file name. The play method plays an audio file from a given file path, and
 * the stopPlayback method stops any ongoing playback.
 *
 * Signals are emitted when the recording state, audio level, or playback state
 * changes, allowing the UI to update accordingly.
 */
class AudioApi : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(qreal audioLevel READ audioLevel NOTIFY isAudioLevelChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isVoiceDetected READ isVoiceDetected NOTIFY isVoiceDetectedChanged)
    Q_PROPERTY(int vadMethod READ vadMethod WRITE setVadMethod NOTIFY vadMethodChanged)

public:
    explicit AudioApi(QObject* parent = nullptr);

    /**
     * Returns whether the application is currently recording audio.
     *
     * @return true if recording is in progress, false otherwise.
     */
    bool isRecording() const;

    /**
     * Returns the current audio level as a value between 0.0 and 1.0, where
     * 0.0 represents silence and 1.0 represents the maximum audio level.
     *
     * @return The current audio level for visual feedback in the UI.
     */
    qreal audioLevel() const;

    /**
     * Returns whether the application is currently playing audio.
     *
     * @return true if audio playback is in progress, false otherwise.
     */
    bool isPlaying() const;

    /**
     * Returns whether the Voice Activity Detection (VAD) has detected speech.
     *
     * @return true if speech is currently detected in the recording.
     */
    bool isVoiceDetected() const;

    /**
     * VAD method selection (0: energy, 1: autocorrelation, 2: hybrid)
     */
    int vadMethod() const { return m_vadMethod; }
    void setVadMethod(int method);

public slots:
    /**
     * Requests the RECORD_AUDIO permission on Android (runtime permission).
     * On desktop this is a no-op and returns true immediately.
     * Must be called from a UI context (QML signal handler or main thread).
     *
     * @return true if the permission is already granted (or not needed),
     *         false if the permission request is pending (result delivered
     *         via permissionResultReceived signal).
     */
    Q_INVOKABLE bool requestAudioPermission();

    /**
     * Starts recording audio from the microphone. If durationSeconds is greater
     * than 0, the recording will automatically stop after the specified number of
     * seconds. If durationSeconds is -1 (the default), recording will continue
     * until stopRecording is called.
     *
     * @param durationSeconds - The duration in seconds for automatic stop, or -1
     *                         for manual stop.
     * @param minimumRecordLength - Minimum number of samples required for recording.
     *                             If > 0 and recording is shorter, auto-stop resets VAD,
     *                             manual stop is ignored. Default -1 (no minimum).
     */
    Q_INVOKABLE void startRecording(int durationSeconds = -1, int minimumRecordLength = -1);
    /**
     * Stops the ongoing audio recording process. If recording is not in progress,
     * this method has no effect.
     *
     * @param isAutoStop - True if called from auto-stop logic, false for manual stop.
     */
    Q_INVOKABLE void stopRecording(bool isAutoStop = false);

    /**
     * Internal VAD curves for visualization.
     * Returns data from the last recording.
     */
    Q_INVOKABLE QVariantList getVadA() const;
    Q_INVOKABLE QVariantList getVadU() const;
    Q_INVOKABLE QVariantList getVadV() const;
    Q_INVOKABLE QVariantList getVadCorr() const;
    Q_INVOKABLE QVariantList getVadCorrU() const;
    Q_INVOKABLE QVariantList getVadCorrV() const;
    /**
     * Saves the recorded audio to a WAV file. If fileName is provided, it will be
     * used as the name of the saved file. If fileName is empty, a default name
     * based on the current timestamp will be generated. The method returns the
     * file path of the saved WAV file.
     *
     * @param fileName - Optional name for the saved WAV file.
     * @return The file path of the saved WAV file.
     */
    Q_INVOKABLE QString saveWavFile(QString fileName = "");
    /**
     * Plays an audio file from the specified file path. If a playback is already
     * in progress, it will be stopped before starting the new playback.
     *
     * @param filePath - The file path of the audio file to be played.
     */
    Q_INVOKABLE void play(const QString& filePath);
    /**
     * Plays a beep tone generated in memory (no temporary file).
     * The beep is a sine wave at the given frequency for the given duration.
     *
     * @param frequencyHz - Tone frequency in Hz (default 440).
     * @param durationMs  - Tone length in milliseconds (default 100).
     * @param amplitude   - Amplitude 0.0–1.0 (default 0.5).
     */
    Q_INVOKABLE void playBeep(double frequencyHz = 440.0, int durationMs = 100, double amplitude = 0.5);
    /**
     * Plays a two-tone "ding-dong" beep generated in memory.
     * Two tones play back-to-back with a short silence gap between them.
     *
     * @param freq1 - First tone frequency in Hz (default 880).
     * @param dur1  - First tone length in ms (default 60).
     * @param freq2 - Second tone frequency in Hz (default 440).
     * @param dur2  - Second tone length in ms (default 120).
     * @param amp   - Amplitude 0.0–1.0 (default 0.5).
     */
    Q_INVOKABLE void playDoubleBeep(double freq1 = 880.0, int dur1 = 60,
                                     double freq2 = 440.0, int dur2 = 120,
                                     double amp = 0.5);
    /**
     * Stops any ongoing audio playback. If no playback is in progress, this
     * method has no effect.
     */
    Q_INVOKABLE void stopPlayback();

    /**
     * Records 2 seconds of silence and calculates the VAD threshold (Pe)
     * based on the background noise level. Emits calibrationFinishedEnergy(threshold)
     * when done.
     */
    // Calibrate VAD using energy-based method.
    Q_INVOKABLE void calibrateVadEnergy();

    /**
     * Records 2 seconds of silence and calculates the autocorrelation VAD threshold
     * based on the background noise level. Emits calibrationFinishedAutocorrelation(threshold)
     * when done. The threshold represents the autocorrelation value (0-1 range)
     * above which speech is detected.
     */
    Q_INVOKABLE void calibrateVadAutocorrelation();

signals:
    /**
     * Signal emitted when the recording state changes (started or stopped).
     */
    void isRecordingChanged();
    /**
     * Signal emitted when the audio level changes, providing visual feedback for
     * the UI.
     */
    void isAudioLevelChanged();
    /**
     * Signal emitted when the playback state changes (started or stopped).
     */
    void isPlayingChanged();
    /**
     * Signal emitted when voice activity detection state changes.
     */
    void isVoiceDetectedChanged();
    void vadMethodChanged();
    /**
     * Emitted when a runtime permission request completes.
     * Only used on Android (RECORD_AUDIO); on desktop this is never emitted.
     *
     * @param permissionGranted - true if the user granted the permission.
     */
    void permissionResultReceived(bool permissionGranted);
    /**
     * Emitted when VAD energy calibration completes. The parameter is the computed
     * threshold value that should be saved in settings.
     */
    void calibrationFinishedEnergy(double threshold);
    /**
     * Emitted when VAD autocorrelation calibration completes. The parameter is the computed
     * threshold value that should be saved in settings.
     */
    void calibrationFinishedAutocorrelation(double threshold);
    /**
     * Emitted when the beep signal finishes playing.
     */
    void beepFinished();

private:
    /**
     * Updates the current audio level based on the recorded audio data. This
     * method is called internally during recording to provide real-time feedback
     * on the audio level for visualization in the UI.
     *
     * @param level - The new audio level value to be set, typically between 0.0
     *                and 1.0.
     */
    void setAudioLevel(qreal level = 0.0);

    // Base buffer elements
    QAudioDevice m_audioDevice;
    std::unique_ptr<QAudioSource> m_audioSource;
    QAudioFormat m_format;
    QByteArray m_buffer;
    qint64 m_bufferOffsetBytes = 0; // Tracks bytes removed from m_buffer
    bool m_isRecording = false;
    qreal m_audioLevel = 0.0;
    std::unique_ptr<WavFileService> m_wavFileService;
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    bool m_isPlaying = false;
    BeepPlayer m_beepPlayer;

    bool m_autoStopEnabled = false;
    int m_silenceDurationMs = 2000;
    bool m_voiceDetected = false;
    static constexpr int PRE_BUFFER_MS = 300; // Pre/post buffer duration in ms

    // VAD services
    std::unique_ptr<VADEnergyService> m_vadService;
    std::unique_ptr<VADAutocorrelationService> m_vadAutocorrService;
    int m_vadMethod = 0; // 0: energy, 1: autocorr, 2: hybrid (default AND)

    int m_firstSpeechFrame = -1;
    int m_lastSpeechFrame = -1;
    int m_silenceFramesCount = 0;
    int m_minimumRecordLength = -1;

    void processVadFrame(int frameIndex, double V_n, double correlation);
};

#endif // AUDIOAPI_H
