#ifndef UMPSERVICE_H
#define UMPSERVICE_H

#include "wavfileservice.h"
#include <string>
#include <vector>

/**
 * UMPService provides methods for calculating the Unified Melodic Profile (UMP)
 * based on pitch data and cue points, as well as comparing UMP portraits to
 * calculate similarity metrics. The service includes static methods for generating
 * UMPs using different interpolation techniques and for comparing UMPs using
 * correlation analysis and range normalization. This service is designed to be
 * used in the Speech Rate Meter application for analyzing melodic contours and providing
 * feedback on pitch accuracy.
 */
struct UMPResult {
    std::vector<double> ump;
    std::vector<CuePointData> processedCuePoints;
};

/**
 * UMPComparison struct contains the results of comparing two UMP portraits, including
 * the percentage of full range covered by each UMP, the similarity of their ranges,
 * and the shape similarity based on Pearson correlation.
 */
struct UMPComparison {
    double referenceRange; // 0-100: percentage of full range covered by reference
                           // UMP
    double userRange; // 0-100: percentage of full range covered by user UMP
    double rangeSimilarity; // 0-100: how similar the ranges are
    double shapeSimilarity; // 0-100: Pearson correlation between the two profiles
};

/**
 * The UMPService class provides methods for calculating the Unified Melodic Profile
 * (UMP) based on pitch data and cue points, as well as comparing UMP portraits to
 * calculate similarity metrics. The service includes static methods for generating
 * UMPs using different interpolation techniques and for comparing UMPs using
 * correlation analysis and range normalization. This service is designed to be
 * used in the Speech Rate Meter application for analyzing melodic contours and providing
 * feedback on pitch accuracy.
 */
class UMPService {
public:
    /**
     * @brief Calculates the Unified Melodic Profile (UMP) based on pitch data and
     * cue points.
     *
     * @param interpolationType The type of interpolation method to use for UMP
     * calculation (e.g., "linear", "cubic", etc.)
     * @param pitch A vector of pitch values that form the basis for UMP
     * generation.
     * @param cuePoints A vector of cue points containing structural markers or
     * control points for the UMP calculation.
     * @param pLength The length or number of pitch samples to process.
     * @param nLength The length parameter for normalization or note-related
     * processing.
     * @param tLength The length of the time window or temporal segment for
     * analysis.
     * @param waveDataSize The total size of the waveform data being processed.
     *
     * @return UMPResult A result object containing the calculated UMP data and
     * status information.
     *
     * @details This static method performs melodic profile analysis using the
     * provided pitch data and cue points, applying the specified interpolation
     * technique to generate a unified representation of the melodic contour.
     */
    static UMPResult getUMP(const std::string& interpolationType,
        const std::vector<double>& pitch,
        const std::vector<CuePointData>& cuePoints,
        int pLength, int nLength, int tLength,
        int waveDataSize,
        const std::string& smoothingType = "None",
        int smoothingWindowSize = 5,
        double smoothingGaussianSigma = 1.0,
        double smoothingSplinePenalty = 1.0);

    /**
     * @brief Compares two UMP portraits and calculates similarity metrics.
     *
     * @param referenceUmp The reference UMP vector to compare against.
     * @param userUmp The user UMP vector to compare.
     * @param fullMin The minimum value of the full range for normalization.
     * @param fullMax The maximum value of the full range for normalization.
     *
     * @return UMPComparison A result object containing:
     *         - referenceRange: Percentage of full range covered by reference UMP
     * (0-100)
     *         - userRange: Percentage of full range covered by user UMP (0-100)
     *         - rangeSimilarity: How well the ranges match (0-100)
     *         - shapeSimilarity: Pearson correlation between profiles (0-100)
     *
     * @details This static method performs profile comparison using correlation
     * analysis and range normalization. Both UMP vectors must have the same
     * length.
     *
     * @note Returns zero values for all fields if input vectors have mismatched
     * lengths or if either vector is empty.
     */
    static UMPComparison compareUMP(const std::vector<double>& referenceUmp,
        const std::vector<double>& userUmp,
        double fullMin, double fullMax);
};

#endif // UMPSERVICE_H
