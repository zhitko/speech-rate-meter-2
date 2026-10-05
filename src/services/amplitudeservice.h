#ifndef AMPLITUDESERVICE_H
#define AMPLITUDESERVICE_H

#include <vector>

/**
 * AmplitudeService provides simple envelope/level calculations over a waveform.
 * It is intentionally lightweight and does not depend on external libraries.
 */
class AmplitudeService {
public:
    AmplitudeService();
    ~AmplitudeService();

    /**
     * Compute an amplitude vector from the input waveform using a sliding window.
     *
     * The amplitude for each frame is calculated as the root-mean-square (RMS)
     * value of the samples within the window.  Frames are generated starting at
     * sample 0, moving forward by \p shift samples each time.  Any leftover
     * samples at the end that do not fill a full window are ignored.
     *
     * @param inputWaveData waveform samples (single channel)
     * @param window      length of the analysis window in samples
     * @param shift       hop size between successive windows in samples
     * @return vector containing the RMS amplitude for each frame
     */
    std::vector<double> getAmplitude(const std::vector<double>& inputWaveData,
        int window, int shift);

    /**
     * Compute the first-order derivative of the amplitude envelope returned by
     * getAmplitude().  The returned vector has the same length as the amplitude
     * vector; the first element is set to 0.0.
     *
     * @param inputWaveData waveform samples (single channel)
     * @param window      length of the analysis window in samples
     * @param shift       hop size between successive windows in samples
     * @return derivative of the amplitude envelope
     */
    std::vector<double>
    getAmplitudeDerivative(const std::vector<double>& inputWaveData, int window,
        int shift);
};

#endif // AMPLITUDESERVICE_H
