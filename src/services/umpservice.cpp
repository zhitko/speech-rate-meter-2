#include "umpservice.h"
#include "helpers/logger.h"
#include "helpers/vectorutils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

UMPResult UMPService::getUMP(const std::string& interpolationType,
    const std::vector<double>& pitch,
    const std::vector<CuePointData>& cuePoints,
    int pLength, int nLength, int tLength,
    int waveDataSize,
    const std::string& smoothingType,
    int smoothingWindowSize,
    double smoothingGaussianSigma,
    double smoothingSplinePenalty)
{
    LOG_DEBUG() << "Start: getUMP - pitch.size=" << pitch.size()
                << ", cuePoints.size=" << cuePoints.size()
                << ", pLength=" << pLength << ", nLength=" << nLength
                << ", tLength=" << tLength << ", waveDataSize=" << waveDataSize
                << ", smoothingType=" << smoothingType
                << ", smoothingWindowSize=" << smoothingWindowSize;

    std::vector<double> result;

    const int pitchSize = pitch.size();

    if (pitch.empty() || cuePoints.empty() || pLength <= 0 || nLength <= 0 || tLength <= 0 || pitchSize <= 0) {
        LOG_WARNING() << "  Early return due to invalid input";
        UMPResult emptyOut;
        return emptyOut;
    }

    // -----------------------
    // If we have single PRE_NUCLEUS or POST_NUCLEUS between two NUCLEUS,
    // we should split it to PRE_NUCLEUS + POST_NUCLEUS with the same position and
    // length as the original one.

    std::vector<CuePointData> processedCuePoints;
    processedCuePoints.push_back(cuePoints[0]);

    // Ensure we have at least 3 cue points to avoid underflow
    if (cuePoints.size() < 3) {
        processedCuePoints.push_back(cuePoints.back());
        UMPResult emptyOut;
        emptyOut.processedCuePoints = processedCuePoints;
        return emptyOut;
    }

    for (int i = 1; i < static_cast<int>(cuePoints.size()) - 1; ++i) {
        const CuePointData& cuePoint = cuePoints[i];
        if (cuePoint.type == CuePointType::NUCLEUS) {
            // Just add NUCLEUS points directly to the processed list
            processedCuePoints.push_back(cuePoint);
        } else if (cuePoint.type == CuePointType::PRE_NUCLEUS || cuePoint.type == CuePointType::POST_NUCLEUS) {
            // Check if this PRE_NUCLEUS or POST_NUCLEUS is between two NUCLEUS points
            const CuePointData& currentCuePoint = cuePoints[i];
            const CuePointData& prevCuePoint = cuePoints[i - 1];
            const CuePointData& nextCuePoint = cuePoints[i + 1];

            if (prevCuePoint.type == CuePointType::NUCLEUS && nextCuePoint.type == CuePointType::NUCLEUS) {
                // Split the current cue point into PRE_NUCLEUS and POST_NUCLEUS
                CuePointData preCuePoint = currentCuePoint;
                preCuePoint.type = CuePointType::POST_NUCLEUS;
                preCuePoint.label = "_t"; // Mark the POST_NUCLEUS with _t suffix for future reference
                preCuePoint.length = currentCuePoint.length / 2;

                LOG_DEBUG() << "  Splitting cue point ID " << currentCuePoint.id
                            << " into POST_NUCLEUS and POST_NUCLEUS";
                LOG_DEBUG() << "    Original: position=" << currentCuePoint.position
                            << ", length=" << currentCuePoint.length;
                LOG_DEBUG() << "    POST_NUCLEUS: position=" << preCuePoint.position
                            << ", length=" << preCuePoint.length;

                CuePointData postCuePoint = currentCuePoint;
                postCuePoint.type = CuePointType::PRE_NUCLEUS;
                postCuePoint.label = "_p"; // Mark the PRE_NUCLEUS with _p suffix for future reference
                postCuePoint.position += currentCuePoint.length / 2;
                postCuePoint.length = currentCuePoint.length / 2;

                LOG_DEBUG() << "    PRE_NUCLEUS: position=" << postCuePoint.position
                            << ", length=" << postCuePoint.length;

                processedCuePoints.push_back(preCuePoint);
                processedCuePoints.push_back(postCuePoint);
            } else {
                // If the PRE_NUCLEUS or POST_NUCLEUS is not between two NUCLEUS, we
                // keep it as is
                processedCuePoints.push_back(cuePoint);
            }
        } else {
            LOG_WARNING() << "  Unknown cue point type for ID " << cuePoint.id
                          << ", label: " << cuePoint.label;
        }
    }
    processedCuePoints.push_back(cuePoints.back());
    // -----------------------

    // Now we should extract segments for each cue point and interpolate them to
    // the target length, then concatenate to form the final UMP vector. For
    // PRE_NUCLEUS and POST_NUCLEUS we should interpalate to make smooth
    // transition between segments.
    int cuePointIndex = 0;
    for (const auto& cuePoint : processedCuePoints) {
        LOG_DEBUG() << "  Processing cue point " << cuePointIndex << " (ID "
                    << cuePoint.id << ", label: " << cuePoint.label << ")";
        LOG_DEBUG() << "    Position: " << cuePoint.position
                    << ", Length: " << cuePoint.length;

        int length = nLength;
        if (cuePoint.type == CuePointType::PRE_NUCLEUS) {
            length = pLength;
        } else if (cuePoint.type == CuePointType::POST_NUCLEUS) {
            length = tLength;
        }

        // Calculate start and end frame indices
        // cuePoint.position is in waveDataSize
        // cuePoint.length is in waveDataSize

        int startFrame = static_cast<int>(
            std::floor(pitchSize * cuePoint.position / waveDataSize));
        int endFrame = static_cast<int>(std::floor(
            pitchSize * (cuePoint.position + cuePoint.length) / waveDataSize));

        LOG_DEBUG() << "    Calculated startFrame: " << startFrame
                    << ", endFrame: " << endFrame;

        // Clamp indices to pitch vector bounds
        startFrame = std::max(0, std::min(startFrame, pitchSize));
        endFrame = std::max(0, std::min(endFrame, pitchSize));

        LOG_DEBUG() << "    Clamped startFrame: " << startFrame
                    << ", endFrame: " << endFrame;

        // If startFrame >= endFrame, we have no data for this segment.
        // We still need to produce 'length' samples to maintain the structure.
        if (startFrame >= endFrame) {
            LOG_DEBUG() << "    No data for this segment, filling with zeros";
            for (int i = 0; i < length; ++i) {
                result.push_back(0.0);
            }
            cuePointIndex++;
            continue;
        }

        // Extract segment
        std::vector<double> segment;
        if (startFrame < endFrame) {
            // if cuePoint is autogenerated, just fill the segment with zeros to avoid
            // introducing noise from the pitch data
            if (cuePoint.label == "_p" || cuePoint.label == "_t") {
                LOG_DEBUG()
                    << "    Autogenerated cue point, filling segment with zeros";
                segment.resize(endFrame - startFrame, 0.0);
                // Replace cue point label removing the _p or _t suffix to indicate that
                // it has been processed
                if (cuePoint.label == "_p") {
                    processedCuePoints[cuePointIndex].label = "p";
                } else if (cuePoint.label == "_t") {
                    processedCuePoints[cuePointIndex].label = "t";
                }
            } else {
                segment.assign(pitch.begin() + startFrame, pitch.begin() + endFrame);
            }
        }

        LOG_DEBUG() << "    Segment size: " << segment.size() << " frames";

        std::vector<double> interpolatedSegment = VectorUtils::interpolate(interpolationType, segment, length);
        LOG_DEBUG() << "    Interpolated segment size: "
                    << interpolatedSegment.size() << " samples";
        result.insert(result.end(), interpolatedSegment.begin(),
            interpolatedSegment.end());

        cuePointIndex++;
    }

    // Find segments with 0 values and replace them with interpolated values to avoid having long flat lines
    // in the UMP which can be problematic for visualization and comparison. We can use linear interpolation for this.
    std::vector<double> interpolatedResult = result;
    {
        const int sz = static_cast<int>(interpolatedResult.size());
        int i = 0;
        while (i < sz) {
            if (std::abs(interpolatedResult[i]) <= std::numeric_limits<double>::epsilon()) {
                // Find the end of this zero run
                int zeroStart = i;
                while (i < sz && std::abs(interpolatedResult[i]) <= std::numeric_limits<double>::epsilon()) {
                    ++i;
                }
                int zeroEnd = i; // exclusive

                // Determine boundary values
                double leftVal  = (zeroStart > 0) ? interpolatedResult[zeroStart - 1] : interpolatedResult[zeroEnd];
                double rightVal = (zeroEnd   < sz) ? interpolatedResult[zeroEnd]       : interpolatedResult[zeroStart - 1];

                // If both boundaries are zero (no non-zero data on either side)
                // leave the run as zeros — nothing to interpolate from.
                if (std::abs(leftVal) <= std::numeric_limits<double>::epsilon()
                    && std::abs(rightVal) <= std::numeric_limits<double>::epsilon()) {
                    continue;
                }

                // Linear interpolation across the zero run
                int runLen = zeroEnd - zeroStart;
                for (int j = 0; j < runLen; ++j) {
                    double t = static_cast<double>(j + 1) / static_cast<double>(runLen + 1);
                    interpolatedResult[zeroStart + j] = leftVal + t * (rightVal - leftVal);
                }
            } else {
                ++i;
            }
        }
    }

    // Apply smoothing as the final step
    if (smoothingType != "None" && !interpolatedResult.empty()) {
        double param1 = static_cast<double>(smoothingWindowSize);
        double param2 = 0.0;
        if (smoothingType == "Gaussian") {
            param2 = smoothingGaussianSigma;
        } else if (smoothingType == "Spline") {
            param1 = smoothingSplinePenalty;
        }
        interpolatedResult = VectorUtils::smooth(smoothingType, interpolatedResult, param1, param2);
        LOG_DEBUG() << "Applied UMP smoothing: type=" << smoothingType
                    << ", result.size=" << interpolatedResult.size();
    }

    LOG_DEBUG() << "Finish: getUMP - result.size=" << result.size();
    UMPResult out;
    out.ump = std::move(interpolatedResult);
    out.processedCuePoints = std::move(processedCuePoints);
    return out;
}

UMPComparison UMPService::compareUMP(const std::vector<double>& referenceUmp,
    const std::vector<double>& userUmp,
    double fullMin, double fullMax)
{
    UMPComparison result = { 0.0, 0.0, 0.0, 0.0 };

    // Validate inputs
    if (referenceUmp.empty() || userUmp.empty() || referenceUmp.size() != userUmp.size() || fullMin >= fullMax) {
        LOG_WARNING() << "compareUMP: Invalid input - reference size="
                      << referenceUmp.size() << ", user size=" << userUmp.size()
                      << ", fullMin=" << fullMin << ", fullMax=" << fullMax;
        return result;
    }

    const double fullRange = fullMax - fullMin;

    // Calculate min/max values for each UMP
    auto refMin = *std::min_element(referenceUmp.begin(), referenceUmp.end());
    auto refMax = *std::max_element(referenceUmp.begin(), referenceUmp.end());
    auto userMin = *std::min_element(userUmp.begin(), userUmp.end());
    auto userMax = *std::max_element(userUmp.begin(), userUmp.end());

    // Calculate range spans as percentage of full range
    double refRangeSpan = (refMax - refMin) / fullRange;
    double userRangeSpan = (userMax - userMin) / fullRange;

    // Clamp to [0, 1] and scale to [0, 100]
    result.referenceRange = std::min(100.0, refRangeSpan * 100.0);
    result.userRange = std::min(100.0, userRangeSpan * 100.0);

    // Calculate range similarity: 100 - abs difference in range percentages
    double rangeDifference = std::abs(result.referenceRange - result.userRange);
    result.rangeSimilarity = std::max(0.0, 100.0 - rangeDifference);

    // Calculate shape similarity using Pearson correlation coefficient
    double correlation = VectorUtils::pearsonCorrelation(referenceUmp, userUmp);

    // Convert correlation from [-1, 1] to [0, 100]
    // -1 (completely opposite) -> 0
    // 0 (no correlation) -> 0
    // 1 (perfect match) -> 100
    result.shapeSimilarity = std::max(0.0, correlation) * 100.0;

    LOG_DEBUG() << "compareUMP completed: refRange=" << result.referenceRange
                << "%, userRange=" << result.userRange
                << "%, rangeSim=" << result.rangeSimilarity
                << "%, shapeSim=" << result.shapeSimilarity << "%";

    return result;
}
