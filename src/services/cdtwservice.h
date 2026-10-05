#ifndef CDTWSERVICE_H
#define CDTWSERVICE_H

#include "wavfileservice.h"
#include <vector>

/**
 * CDTWService implements a constrained dynamic time warping algorithm for
 * aligning a signal to a template. It computes the optimal alignment path and
 * allows for applying this path to transform vectors (e.g., pitch contours) and
 * cue points. The service is designed to handle multi-dimensional feature streams
 * and provides methods for normalizing and scaling data as needed.
 *
 * With useFixedStartEndDP, alignment is constrained to start at the first template
 * and signal frames and end at the last frames (standard endpoint DTW / morphing).
 * Otherwise, a free start along the signal and free end are used (sliding-window style).
 */
class CDTWService {
public:
    /**
     * Constructs a CDTWService object with the given template and signal data.
     * The templateData and signalData parameters are expected to be 3D vectors
     * where the dimensions represent [feature][frame][value]. The constructor
     * initializes the internal state of the CDTWService object and prepares it for
     * computing the dynamic time warping distance and optimal path.
     *
     * @param templateData - A 3D vector containing the feature data for the
     * template. The dimensions should be organized as [feature][frame][value].
     * @param signalData - A 3D vector containing the feature data for the signal.
     * The dimensions should be organized as [feature][frame][value].
     */
    CDTWService(std::vector<std::vector<std::vector<double>>> templateData,
        std::vector<std::vector<std::vector<double>>> signalData,
        std::vector<double> streamWeights = {},
        double matchCoef = 1.0,
        double insertionCoef = 1.0,
        double deletionCoef = 1.0,
        std::vector<double> templateMask = {},
        std::vector<double> signalMask = {},
        bool useFixedStartEndDP = false);
    ~CDTWService();

    /**
     * Computes the constrained dynamic time warping distance between the template
     * and signal data, and finds the optimal alignment path. The method updates
     * the internal state of the CDTWService object with the best start and end
     * indices of the alignment, the minimum final cost, the optimal path, and
     * the per-signal-frame alignment cost vector (signalStreamDistances).
     * This method should be called after initializing the CDTWService with the
     * template and signal data.
     */
    void compute();

    // Apply the computed DP path to a given vector (e.g., pitch vector) and
    // return the transformed vector
    std::vector<double> applyPathToVector(const std::vector<double>& input,
        const int targetLength);

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
    std::vector<CuePointData>
    applyPathToCuePoints(const std::vector<CuePointData>& cuePoints);

    /**
     * Getters for the best start index, best end index, and minimum final cost
     * of the computed dynamic programming alignment. These values are updated
     * after calling the compute() method and provide information about the
     * optimal alignment found between the template and signal data.
     *
     * @return The best start index, best end index, and minimum final cost of
     * the computed alignment.
     */
    int getBestStartIndex() const { return bestStartIndex; }
    /**
     * @return The best end index of the computed alignment.
     */
    int getBestEndIndex() const { return bestEndIndex; }
    /**
     * @return The minimum final cost of the computed alignment.
     */
    double getMinFinalCost() const { return minFinalCost; }
    /**
     * @return The minimum final cost normalized to the range [0, 100].
     * Computed as (minFinalCost / (m + n)) * 100, where m is the template
     * length and n is the signal length (both fixed before compute() runs).
     * 0 = perfect match, 100 = worst possible alignment.
     * Strictly monotonic with minFinalCost — directly usable as a threshold
     * independent of template and recording duration.
     */
    double getNormalizedFinalCost() const { return normalizedFinalCost; }
    /**
     * @return A vector of DTW alignment costs at the full template length for
     * each signal frame (index i corresponds to signal frame i). This represents
     * the cost "curve" of the sliding-window alignment across the signal stream
     * and is populated by compute().
     */
    std::vector<double> getSignalStreamDistances() const { return signalStreamDistances; }
    std::vector<std::vector<std::vector<double>>> getTemplateData() const { return templateData; }
    std::vector<std::vector<std::vector<double>>> getSignalData() const { return signalData; }

private:
    /**
     * Normalizes the template and signal data to ensure that the values are on
     * a comparable scale. This method should be called before computing the
     * dynamic programming distance to improve the accuracy of the alignment.
     * The normalization process may involve scaling the values to a specific
     * range or applying other normalization techniques as needed.
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
    
    /**
     * Applies a mask to the input stream, multiplying each frame by the corresponding mask value.
     * This method is used to focus the alignment on specific regions of the feature streams.
     *
     * @param stream - A 2D vector representing the feature stream to be masked,
     * where the dimensions are organized as [frame][value].
     * @param mask - A vector of double values representing the mask, where each element
     * corresponds to a frame in the stream.
     * @return A 2D vector containing the masked feature stream, where each frame has been multiplied 
     * by the corresponding mask value.
     */
    std::vector<std::vector<double>> applyMask(const std::vector<std::vector<double>>& stream,
        const std::vector<double>& mask);

    // Template and signal data
    std::vector<std::vector<std::vector<double>>> templateData;
    std::vector<std::vector<std::vector<double>>> signalData;
    std::vector<double> streamWeights;

    // DP editing coefficients
    double matchCoef;
    double insertionCoef;
    double deletionCoef;
    bool useFixedStartEndDP;

    int bestStartIndex;
    int bestEndIndex;
    double minFinalCost;
    double normalizedFinalCost;
    std::vector<int> optimalPath;
    std::vector<double> signalStreamDistances;

    std::vector<double> templateMask;
    std::vector<double> signalMask;

    // Precomputed values for distance calculations (set during compute())
    size_t precomputedNumStreams;
    std::vector<double> precomputedNormalizedWeights;
    std::vector<double> precomputedInvSqrtCache;

protected:
    /**
     * Calculates the distance between two frames of feature data at the specified
     * template and signal indices. This method is used as part of the dynamic
     * programming algorithm to compute the cost of aligning specific frames from
     * the template and signal data. The distance calculation may involve
     * comparing multiple features and combining their differences into a single
     * distance value that represents the similarity between the two frames.
     *
     * @param templateIndex - The index of the frame in the template data to be compared.
     * @param signalIndex - The index of the frame in the signal data to be compared.
     * @return A double value representing the calculated distance between the two
     * frames of feature data.
     */
    double calculateDistance(int templateIndex, int signalIndex);

    /**
     * Calculates the distance between two vectors of feature values. This method
     * is used to compute the distance between corresponding frames of feature data
     * from the template and signal, and may involve comparing multiple features
     * and combining their differences into a single distance value.
     *
     * @param vec1 - A vector of double values representing the features of one frame (e.g., from the template).
     * @param vec2 - A vector of double values representing the features of another frame (e.g., from the signal).
     * @return A double value representing the calculated distance between the two
     * vectors of feature values.
     */
    double calculateDistance(const std::vector<double>& vec1,
        const std::vector<double>& vec2);
};

#endif // CDTWSERVICE_H
