#ifndef VADENERGRYSERVICE_H
#define VADENERGRYSERVICE_H

#include <vector>
#include <cstdint>
#include <QObject>

/**
 * VADEnergyService handles Voice Activity Detection using energy-based analysis.
 * It processes audio samples and calculates VAD metrics (A, U, H, V) incrementally.
 * 
 * The algorithm uses:
 * - A(n): Amplitude (energy of 128-sample frame with 64-sample hop)
 * - U(n): Mean amplitude (smoothed using K_FRAMES)
 * - H(n): Absolute difference from mean
 * - V(n): Energy/Variation (smoothed H values)
 */
class VADEnergyService : public QObject {
    Q_OBJECT

public:
    explicit VADEnergyService(QObject* parent = nullptr);

    /**
     * Process audio samples and calculate VAD metrics incrementally.
     * @param samples - raw audio samples (int16)
     * @param numSamples - number of samples to process
     * @return vector of newly calculated V values
     */
    std::vector<double> processAudioSamples(const qint16* samples, qint64 numSamples);

    /**
     * Add a V value to calibration frames and check if calibration is complete.
     * @param V_n - energy value to calibrate
     * @return true if calibration is now complete
     */
    bool addCalibrationFrame(double V_n);

    /**
     * Get the current VAD threshold (Pe).
     */
    double getThreshold() const { return m_Pe; }

    /**
     * Set the VAD threshold manually (e.g., from saved settings).
     */
    void setThreshold(double threshold) { 
        m_Pe = threshold; 
        m_voiceThresholdHigh = threshold;
        m_voiceThresholdLow = threshold * 0.9;  // 10% gap for hysteresis
    }

    /**
     * Set hysteresis thresholds manually.
     */
    void setVoiceThresholdHigh(double high) { m_voiceThresholdHigh = high; }
    void setVoiceThresholdLow(double low) { m_voiceThresholdLow = low; }

    /**
     * Check if a V value indicates speech based on current threshold.
     */
    bool isSpeech(double V_n) const { return V_n > m_Pe; }

    /**
     * Check if a V value indicates speech with hysteresis.
     */
    bool isSpeech(double V_n, bool currentState) const {
        if (currentState) {
            return V_n > m_voiceThresholdLow;
        } else {
            return V_n > m_voiceThresholdHigh;
        }
    }

    /**
     * Get current valid V index (for frame tracking).
     */
    int getValidVIndex() const { return m_valid_V; }

    /**
     * Reset all internal state (for new recording).
     */
    void reset();

    /**
     * Reset calibration state only (keep current threshold).
     */
    void resetCalibration();

    /**
     * Prepare for calibration (clear state, ready to record 2 seconds).
     */
    void prepareForCalibration();

    /**
     * Get all calibration frames collected so far.
     */
    const std::vector<double>& getCalibrationFrames() const { return m_calibrationFrames; }

    /**
     * Get saved A, U, V metrics from last recording.
     */
    const std::vector<double>& getSavedA() const { return m_savedA; }
    const std::vector<double>& getSavedU() const { return m_savedU; }
    const std::vector<double>& getSavedV() const { return m_savedV; }

    /**
     * Update saved metrics (called after recording stops).
     */
    void updateSavedMetrics();

    /**
     * Crop saved metrics to a range.
     * @param startFrame - starting frame index
     * @param endFrame - ending frame index (exclusive)
     */
    void cropSavedMetrics(int startFrame, int endFrame);

    /**
     * Calculate threshold from a set of V values (for calibration).
     * Uses 95th percentile × 3 approach.
     */
    static double calculateThresholdFromValues(const std::vector<double>& values);

    /**
     * Collect recently calculated V values (since last call).
     * Clears the internal state after returning.
     */
    std::vector<double> getAndClearRecentVValues();

signals:
    /**
     * Emitted when calibration completes, providing the computed threshold.
     */
    void calibrationFinished(double threshold);

private:
    // VAD algorithm constants
    static constexpr int FRAME_SIZE = 128;
    static constexpr int HOP_SIZE = 64;
    static constexpr int K_FRAMES = 16;
    static constexpr int CALIBRATION_FRAMES = 50;

    // Current VAD state (incremental calculation)
    std::vector<qint16> m_sampleBuf;      // Temporary buffer for frame assembly
    std::vector<double> m_A;               // Amplitude frames
    std::vector<double> m_U;               // Mean amplitude
    std::vector<double> m_H;               // Absolute difference from mean
    std::vector<double> m_V;               // Energy/variation

    // Saved metrics from last recording
    std::vector<double> m_savedA;
    std::vector<double> m_savedU;
    std::vector<double> m_savedV;

    // Incremental processing indices
    int m_valid_U = 0;
    int m_valid_V = 0;

    // Calibration state
    int m_calibrationCounter = 0;
    std::vector<double> m_calibrationFrames;
    double m_Pe = 0.0;  // Threshold (percentile * 3)
    double m_voiceThresholdHigh = 0.0;
    double m_voiceThresholdLow = 0.0;
};

#endif // VADENERGRYSERVICE_H
