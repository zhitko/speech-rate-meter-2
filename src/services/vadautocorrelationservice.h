#ifndef VADAUTOCORRELATIONSERVICE_H
#define VADAUTOCORRELATIONSERVICE_H

#include <vector>
#include <cstdint>
#include <QObject>

/**
 * VADAutocorrelationService — Voice Activity Detection via NACF
 * with a single-stage smoothing pipeline:
 *
 *   C(n) — raw max Normalized Autocorrelation Function per frame  [0..1]
 *   U(n) — smoothed mean of C  over window [n-K+1 .. n+K-1]
 *
 * VAD decisions are taken directly on U(n) with hysteresis.
 *
 * Uses F0 bounds in Hz from settings and converts them to lag indices:
 *   lag = sampleRate / frequency
 *   80–200 Hz at  8 kHz : LAG_MIN=40,  LAG_MAX=100
 *   80–200 Hz at 16 kHz : LAG_MIN=80,  LAG_MAX=200
 *
 * F0 bounds are now configured in AppSettings as minF0/maxF0 in Hz, and the
 * conversion to lag is done inside VADAutocorrelationService based on the audio
 * sample rate.
 *
 * ENERGY GATE NOTE:
 *   DEFAULT_ENERGY_THRESHOLD is intentionally low (0.0001) so that even quiet
 *   microphones (Windows AGC, low-gain drivers) pass the gate.
 *   Raise it via setEnergyThreshold() if stricter silence rejection is needed.
 */
class VADAutocorrelationService : public QObject {
    Q_OBJECT

public:
    explicit VADAutocorrelationService(QObject* parent = nullptr);

    // ------------------------------------------------------------------ //
    //  Main processing
    // ------------------------------------------------------------------ //

    /**
     * Process audio samples incrementally.
     * @param samples    raw int16 audio
     * @param numSamples number of samples
     * @return newly computed U(n) values (empty if not enough data yet)
     */
    std::vector<double> processAudioSamples(const qint16* samples, qint64 numSamples);

    // ------------------------------------------------------------------ //
    //  Speech detection — based on U(n)
    // ------------------------------------------------------------------ //

    /**
     * VAD decision on smoothed U(n) with hysteresis.
     * ON  when U > voiceThresholdHigh
     * OFF when U < voiceThresholdLow
     * @param U_n          smoothed mean autocorrelation value
     * @param currentState current VAD state (true = speech)
     */
    bool isSpeech(double U_n, bool currentState = false) const;

    // ------------------------------------------------------------------ //
    //  Threshold setters
    // ------------------------------------------------------------------ //

    /** Hysteresis ON  threshold for U(n). Default 0.45. */
    void setVoiceThresholdHigh(double t) { m_voiceThresholdUHigh = t; }

    /** Hysteresis OFF threshold for U(n). Default 0.35. */
    void setVoiceThresholdLow(double t)  { m_voiceThresholdULow  = t; }

    /**
     * Minimum frame energy (R0 after Hamming) below which C(n) is forced to 0.
     * Default 0.0001 — deliberately low to work with quiet Windows mic drivers.
     * Raise to 0.005 for stricter noise rejection on high-gain setups.
     */
    void   setEnergyThreshold(double t) { m_energyThreshold = t; }
    double getEnergyThreshold() const   { return m_energyThreshold; }
    /** Set the audio sample rate used to convert F0 bounds to lag values. */
    void setSampleRate(double sampleRate) { m_sampleRate = sampleRate; }

    /** Set the F0 search range in Hz used to compute lag bounds for autocorrelation. */
    void setPitchRange(double minF0Hz, double maxF0Hz) {
        m_minF0 = minF0Hz;
        m_maxF0 = maxF0Hz;
    }
    // ------------------------------------------------------------------ //
    //  Lifecycle
    // ------------------------------------------------------------------ //

    /** Reset all internal state (call before a new recording). */
    void reset();

    // ------------------------------------------------------------------ //
    //  Saved metrics  (call updateSavedMetrics() after recording stops)
    // ------------------------------------------------------------------ //

    void updateSavedMetrics();
    void cropSavedMetrics(int startFrame, int endFrame);

    /** Raw NACF frames C(n) from the last recording. */
    const std::vector<double>& getSavedC() const { return m_savedC; }

    /** First-pass smoothed mean U(n) from the last recording. */
    const std::vector<double>& getSavedU() const { return m_savedU; }

    /** Legacy alias for getSavedC(). */
    const std::vector<double>& getSavedCorrelations() const { return m_savedC; }

    /** Index of the next frame to be folded into U (mirrors VADEnergyService::getValidVIndex). */
    int getValidUIndex() const { return m_valid_U; }

    /** Smoothed U(n) at frame index, or 0 if not yet computed. */
    double getU(int frameIndex) const {
        if (frameIndex < 0 || frameIndex >= m_valid_U) return 0.0;
        return m_U[static_cast<std::size_t>(frameIndex)];
    }

private:
    void   applyHammingWindow(std::vector<double>& frame);
    double calculateAutocorrelation(const std::vector<double>& frame, int lag);
    double findMaxAutocorrelation(const std::vector<double>& frame, int lagMin, int lagMax);

    // --- Constants ---
    static constexpr int FRAME_SIZE = 128;    // ~16 ms at 8 kHz
    static constexpr int HOP_SIZE   = 64;     // ~8 ms  at 8 kHz (50% overlap)
    static constexpr int K_FRAMES   = 16;     // smoothing half-window (matches VADEnergyService)

    static constexpr double DEFAULT_VOICE_THRESHOLD_U_HIGH = 0.45;
    static constexpr double DEFAULT_VOICE_THRESHOLD_U_LOW  = 0.35;

    // Intentionally low: 0.005 (old default) gates out quiet Windows mic input entirely.
    static constexpr double DEFAULT_ENERGY_THRESHOLD = 0.0001;

    // --- Sample buffer ---
    std::vector<qint16> m_sampleBuf;

    // --- Smoothing pipeline (never drained — updateSavedMetrics needs full history) ---
    std::vector<double> m_C;
    std::vector<double> m_U;
    std::vector<double> m_H;

    // --- Saved snapshots ---
    std::vector<double> m_savedC;
    std::vector<double> m_savedU;

    // --- Incremental cursors (same pattern as VADEnergyService) ---
    int m_valid_U = 0;  // next C index to be folded into U/H

    // --- Thresholds ---
    double m_voiceThresholdUHigh = DEFAULT_VOICE_THRESHOLD_U_HIGH;
    double m_voiceThresholdULow  = DEFAULT_VOICE_THRESHOLD_U_LOW;
    double m_energyThreshold     = DEFAULT_ENERGY_THRESHOLD;

    // --- VAD state ---
    std::vector<bool> m_vadDecisions;
    bool              m_currentVadState = false;

    // --- Audio and pitch settings ---
    double m_sampleRate = 8000.0;
    double m_minF0 = 80.0;
    double m_maxF0 = 200.0;
};

#endif // VADAUTOCORRELATIONSERVICE_H