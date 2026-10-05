#ifndef DPSERVICE_H
#define DPSERVICE_H

#include <vector>

/**
 * DPService implements a dynamic programming algorithm for aligning a signal to a
 * template. It computes the optimal alignment path and allows for applying this
 * path to transform vectors (e.g., pitch contours) and cue points. The service is
 * designed to handle multi-dimensional feature streams and provides methods for
 * normalizing and scaling data as needed.
 */
class DPService {
public:
    /**
     * Constructs a DPService object with the given template and signal data. The
     * templateData and signalData parameters are expected to be 3D vectors where
     * the dimensions represent [feature][frame][value]. The constructor
     * initializes the internal state of the DPService object and prepares it for
     * computing the dynamic programming distance and optimal path.
     *
     * @param templateData - A 3D vector containing the feature data for the
     * template. The dimensions should be organized as [feature][frame][value].
     * @param signalData - A 3D vector containing the feature data for the signal.
     * The dimensions should be organized as [feature][frame][value].
     */
    DPService(std::vector<std::vector<std::vector<double>>> templateData,
        std::vector<std::vector<std::vector<double>>> signalData);
    ~DPService();

    /**
     * Computes the dynamic programming distance between the template and signal
     * data, and finds the optimal alignment path. The method updates the internal
     * state of the DPService object with the best start and end indices of the
     * alignment, the minimum final cost, and the optimal path. This method should
     * be called after initializing the DPService with the template and signal data.
     */
    void compute();

    /**
     * Applies the computed dynamic programming path to a vector of cue points,
     * transforming their positions according to the optimal alignment between the
     * template and signal. This method allows for adjusting the timing of cue
     * points based on the alignment, which can be useful for synchronizing events
     * in the signal with corresponding events in the template.
     *
     * @param cuePoints - A vector of CuePointData representing the original cue
     * points to be transformed. Each CuePointData should contain information about
     * the position and other relevant attributes of the cue point.
     * @return A vector of CuePointData containing the transformed cue points after
     * applying the computed DP path.
     */
    std::vector<double> applyPathToVector(const std::vector<double>& input,
        const int targetLength);

private:
    /**
     * Normalizes the template and signal data to ensure that the values are on a
     * comparable scale. This method should be called before computing the dynamic
     * programming distance to improve the accuracy of the alignment. The
     * normalization process may involve scaling the values to a specific range or
     * applying other normalization techniques as needed.
     */
    void normalizeData();

    /**
     * Scales the input stream to the target length using linear interpolation.
     * This method is used to adjust the length of feature streams (e.g., pitch
     * contours) to match the length of the optimal path found by the dynamic
     * programming algorithm. The scaling process ensures that the features are
     * properly aligned with the template and signal data for accurate analysis
     * and transformation.
     *
     * @param stream - A 2D vector representing the feature stream to be scaled,
     * where the dimensions are organized as [frame][value].
     * @param targetLength - The target length to which the stream should be
     * scaled, typically corresponding to the number of frames in the optimal
     * alignment path.
     * @return A 2D vector containing the scaled feature stream, adjusted to the
     * target length using linear interpolation.
     */
    std::vector<std::vector<double>>
    scaleStream(const std::vector<std::vector<double>>& stream,
        size_t targetLength);

    // Template and signal data for dynamic programming
    std::vector<std::vector<std::vector<double>>> templateData;
    std::vector<std::vector<std::vector<double>>> signalData;

    // Distance matrix for dynamic programming
    std::vector<std::vector<double>> distanceMatrix;

    // Integral time distance matrix for dynamic programming
    std::vector<std::vector<double>> integralTimeDistanceMatrix;

    // Optimal path found by dynamic programming
    std::vector<int> optimalPath;

protected:
    /**
     * Best start index of the alignment in the signal data.
     */
    void computeDistanceMatrix();

    /**
     * Calculates the distance between a template value and a signal value. This
     * method is used as part of the dynamic programming algorithm to populate the
     * distance matrix, which is then used to find the optimal alignment path.
     *
     * @param templateValue - A vector representing the feature values for a frame in the template.
     * @param signalValue - A vector representing the feature values for a frame in the signal.
     * @return A double value representing the calculated distance between the template and signal values.
     */
    double calculateDistance(const std::vector<double>& templateValue,
        const std::vector<double>& signalValue);

    /**
     * Compute the integral time distance matrix for dynamic programming.
     */
    void computeIntegralTimeDistanceMatrix();

    /**
     * Find position for signal start position in dynamic programming.
     * @return Minimum position index for signal start
     */
    int findPositionForSignalStart();

    /**
     * Find position for signal end position in dynamic programming.
     * @return Minimum position index for signal end
     */
    int findPositionForSignalEnd();

    /**
     * Perform backtracking to find the optimal path in dynamic programming.
     * @param signalStartPos Starting position in the signal for backtracking
     * @param signalEndPos Ending position in the signal for backtracking
     * @return Vector of indices representing the optimal path through the
     * distance matrix
     */
    std::vector<int> performBacktracking(int signalStartPos, int signalEndPos);
};

#endif // DPSERVICE_H