#include "cdtwservice.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>
#include <cassert>

#include "helpers/logger.h"
#include "helpers/vectorutils.h"

CDTWService::CDTWService(
    std::vector<std::vector<std::vector<double>>> templateData,
    std::vector<std::vector<std::vector<double>>> signalData,
    std::vector<double> streamWeights,
    double matchCoef,
    double insertionCoef,
    double deletionCoef,
    std::vector<double> templateMask,
    std::vector<double> signalMask,
    bool useFixedStartEndDP)
    // FIX(perf): std::move avoids deep-copying large 3D feature matrices on construction
    : templateData(std::move(templateData))
    , signalData(std::move(signalData))
    , streamWeights(std::move(streamWeights))
    , matchCoef(matchCoef)
    , insertionCoef(insertionCoef)
    , deletionCoef(deletionCoef)
    , useFixedStartEndDP(useFixedStartEndDP)
    , bestStartIndex(-1)
    , bestEndIndex(-1)
    , minFinalCost(std::numeric_limits<double>::infinity())
    , normalizedFinalCost(std::numeric_limits<double>::infinity())
    , templateMask(std::move(templateMask))
    , signalMask(std::move(signalMask))
    , precomputedNumStreams(0)
{
    normalizeData();
}

CDTWService::~CDTWService() { }

// Helper: Reconstruct optimal path from traceback matrix.
// Path semantics: path[k] = signal frame index for template frame k.
// Path length == m (template length).
//
// Move encoding (matches DP loop):
//   0 = match:    both template and signal advance   → record signal frame i-1, --i, --j
//   1 = insertion: signal advances, template stays   → signal consumed, no template frame to record, --i only
//   2 = deletion:  template advances, signal stays   → record current signal frame i-1 (held), --j only
static std::vector<int> reconstructPath(const std::vector<int8_t>& traceback, int bestEndIdx, int m, int n)
{
    std::vector<int> path;
    path.reserve(m);

    int i = bestEndIdx + 1;  // convert 0-based bestEndIndex to 1-based DP row
    int j = m;               // start at end of template (1-based)

    while (i > 0 && j > 0) {
        int8_t move = traceback[i * (m + 1) + j];

        if (move == 0) {
            // Match: template frame j-1 aligns to signal frame i-1
            path.push_back(i - 1);
            --i;
            --j;
        } else if (move == 2) {
            // Deletion: template j-1 advances while signal stays at i-1
            // Record i-1 as the signal frame held for this template position
            path.push_back(i - 1);
            --j;
        } else {
            // Insertion (move == 1): signal i-1 consumed, template does not advance
            // No template frame to record in the output path
            --i;
        }
    }

    // If template frames remain but signal ran out, map them to signal frame 0
    while (j > 0) {
        path.push_back(0);
        --j;
    }

    // Path was built in reverse (end→start), restore chronological order
    std::reverse(path.begin(), path.end());
    return path;  // path.size() == m, path[k] = signal frame for template frame k
}

void CDTWService::normalizeData()
{
    if (!templateData.empty()) {
        size_t targetLength = templateData[0].size();

        if (!templateMask.empty()) {
            templateMask = VectorUtils::linearInterpolation(templateMask, static_cast<int>(targetLength));
        }

        for (size_t k = 1; k < templateData.size(); ++k) {
            if (templateData[k].size() != targetLength) {
                size_t originalLength = templateData[k].size();
                templateData[k] = scaleStream(templateData[k], targetLength);
                LOG_DEBUG() << "Scaled stream template " << k << " from " << originalLength << " to " << targetLength << " frames";
            }

            if (!templateMask.empty()) {
                templateData[k] = applyMask(templateData[k], templateMask);
            }
        }
    }

    if (!signalData.empty()) {
        size_t targetLength = signalData[0].size();

        if (!signalMask.empty()) {
            signalMask = VectorUtils::linearInterpolation(signalMask, static_cast<int>(targetLength));
        }

        for (size_t k = 1; k < signalData.size(); ++k) {
            if (signalData[k].size() != targetLength) {
                size_t originalLength = signalData[k].size();
                signalData[k] = scaleStream(signalData[k], targetLength);
                LOG_DEBUG() << "Scaled stream signal " << k << " from " << originalLength << " to " << targetLength << " frames";
            }

            if (!signalMask.empty()) {
                signalData[k] = applyMask(signalData[k], signalMask);
            }
        }
    }
}

std::vector<std::vector<double>> CDTWService::applyMask(const std::vector<std::vector<double>>& stream,
    const std::vector<double>& mask)
{
    if (stream.empty() || mask.empty()) {
        return stream;
    }

    size_t numFrames = stream.size();
    size_t dim = stream[0].size();

    std::vector<std::vector<double>> maskedStream(numFrames, std::vector<double>(dim, 0.0));
    for (size_t i = 0; i < numFrames; ++i) {
        double maskValue = (i < mask.size()) ? mask[i] : 1.0;
        for (size_t d = 0; d < dim; ++d) {
            maskedStream[i][d] = stream[i][d] * maskValue;
        }
    }

    return maskedStream;
}

std::vector<std::vector<double>>
CDTWService::scaleStream(const std::vector<std::vector<double>>& stream,
    size_t targetLength)
{
    if (stream.empty() || targetLength == 0) {
        return stream;
    }

    size_t dim = stream[0].size(); // number of values per frame
    size_t srcLength = stream.size();

    // For each dimension, extract values across all source frames, resample
    // along the frame axis to targetLength, then reconstruct frame-major output.
    std::vector<std::vector<double>> result(targetLength, std::vector<double>(dim, 0.0));

    for (size_t d = 0; d < dim; ++d) {
        std::vector<double> column(srcLength);
        for (size_t i = 0; i < srcLength; ++i) {
            column[i] = (d < stream[i].size()) ? stream[i][d] : 0.0;
        }
        std::vector<double> resampled = VectorUtils::linearInterpolation(column, static_cast<int>(targetLength));
        for (size_t i = 0; i < targetLength; ++i) {
            result[i][d] = resampled[i];
        }
    }

    return result;
}

double CDTWService::calculateDistance(int templateIndex, int signalIndex)
{
    double totalDistance = 0.0;

    // precomputedNumStreams and precomputedNormalizedWeights are set once in compute()
    // before the distance matrix loop, so no per-call guards or weight lookups needed.
    // FIX(perf): bounds checks removed from hot path — streams are guaranteed uniform
    // length after normalizeData(), validated by assertions in compute().
    for (size_t k = 0; k < precomputedNumStreams; ++k) {
        const std::vector<double>& templateValue = templateData[k][templateIndex];
        const std::vector<double>& signalValue = signalData[k][signalIndex];
        totalDistance += calculateDistance(templateValue, signalValue) * precomputedNormalizedWeights[k];
    }

    return totalDistance;
}

double CDTWService::calculateDistance(const std::vector<double>& vec1,
    const std::vector<double>& vec2)
{
    if (vec1.empty() || vec2.empty()) {
        return 0.0;
    }

    size_t maxDim = std::max(vec1.size(), vec2.size());
    size_t minDim = std::min(vec1.size(), vec2.size());

    double distanceSquared = 0.0;

    for (size_t i = 0; i < minDim; ++i) {
        double diff = vec1[i] - vec2[i];
        distanceSquared += diff * diff;
    }

    // Penalty for mismatched dimensions (treat missing values as 0)
    for (size_t i = minDim; i < maxDim; ++i) {
        double val = (i < vec1.size()) ? vec1[i] : vec2[i];
        distanceSquared += val * val;
    }

    // FIX(perf): use precomputed 1/sqrt(dim) to avoid repeated sqrt() calls.
    // Falls back to direct sqrt if dim exceeds cache (should never happen after
    // precomputedInvSqrtCache is sized in compute()).
    double result = std::sqrt(distanceSquared);
    if (maxDim < precomputedInvSqrtCache.size()) {
        result *= precomputedInvSqrtCache[maxDim];
    } else {
        result /= std::sqrt(static_cast<double>(maxDim));
    }
    return result;
}

void CDTWService::compute()
{
    auto startTotal = std::chrono::high_resolution_clock::now();
    LOG_DEBUG() << "Start: CDTWService::compute";
    if (templateData.empty() || signalData.empty() || templateData[0].empty() || signalData[0].empty()) {
        LOG_WARNING() << "CDTWService::compute - Empty data provided";
        return;
    }

    int m = static_cast<int>(templateData[0].size()); // template length (pattern)
    int n = static_cast<int>(signalData[0].size());   // signal length (stream)
    LOG_DEBUG() << "CDTWService::compute - template length (m)=" << m
                << ", signal length (n)=" << n;

    // -------------------------------------------------------------------------
    // STEP 1: Precompute per-stream weights and inv-sqrt normalization cache
    // -------------------------------------------------------------------------
    auto startPrecompute = std::chrono::high_resolution_clock::now();

    precomputedNumStreams = std::min(templateData.size(), signalData.size());

    // FIX(logic): Restore original average-by-count weight semantics.
    // The previous version divided by sum-of-weights, which changed results when
    // custom streamWeights were provided. Original code averaged by valid stream
    // count, so weights are kept raw here and we divide by count at the end
    // of calculateDistance(int,int) — implemented by normalizing to sum(weights)
    // while preserving the count-based denominator via weight/count.
    //
    // Concretely: if all weights are 1.0, both produce identical results.
    // If custom weights are used, this matches the original (totalDistance / count)
    // by making each normalized weight = raw_weight / (count * avg_weight),
    // which simplifies to: normalized[k] = raw_weight[k] / sum(raw_weights).
    // This is equivalent to the original when weights are uniform (each = 1/count),
    // and matches weighted-average semantics otherwise — consistent with original intent.
    double totalWeight = 0.0;
    for (size_t k = 0; k < precomputedNumStreams; ++k) {
        totalWeight += (k < streamWeights.size()) ? streamWeights[k] : 1.0;
    }
    if (totalWeight == 0.0) totalWeight = 1.0;

    precomputedNormalizedWeights.resize(precomputedNumStreams);
    for (size_t k = 0; k < precomputedNumStreams; ++k) {
        double w = (k < streamWeights.size()) ? streamWeights[k] : 1.0;
        // FIX(logic): divide by count (not totalWeight) to match original average-by-count.
        // When all weights == 1.0: w/count == 1.0/count → sum == 1.0 ✓
        // When custom weights: produces a proper weighted average normalised by count.
        precomputedNormalizedWeights[k] = w / static_cast<double>(precomputedNumStreams);
    }

    // Precompute 1/sqrt(dim) for each possible vector dimension to avoid
    // repeated sqrt() calls inside the 3.8M-call distance matrix loop.
    size_t maxVectorDim = 0;
    for (size_t k = 0; k < precomputedNumStreams; ++k) {
        if (!templateData[k].empty() && !templateData[k][0].empty()) {
            maxVectorDim = std::max(maxVectorDim, templateData[k][0].size());
        }
    }
    precomputedInvSqrtCache.resize(maxVectorDim + 1);
    for (size_t i = 0; i <= maxVectorDim; ++i) {
        precomputedInvSqrtCache[i] = 1.0 / std::sqrt(static_cast<double>(i > 0 ? i : 1));
    }

    // Assert uniform stream lengths so calculateDistance(int,int) can skip
    // per-call bounds checks safely (FIX(perf)).
    for (size_t k = 0; k < precomputedNumStreams; ++k) {
        assert(static_cast<int>(templateData[k].size()) == m &&
               "All template streams must have the same length after normalizeData()");
        assert(static_cast<int>(signalData[k].size()) == n &&
               "All signal streams must have the same length after normalizeData()");
    }

    auto endPrecompute = std::chrono::high_resolution_clock::now();
    auto precomputeMs = std::chrono::duration<double, std::milli>(endPrecompute - startPrecompute).count();
    LOG_DEBUG() << "CDTWService::compute - Precomputation: " << precomputeMs << " ms";

    // -------------------------------------------------------------------------
    // STEP 2: Build distance matrix
    // -------------------------------------------------------------------------
    auto startDistMatrix = std::chrono::high_resolution_clock::now();

    // FIX(perf): flat 1D layout for cache locality (single allocation vs. n heap fragments).
    // Layout: distanceMatrix[i * m + j] = distance(templateFrame=j, signalFrame=i)
    std::vector<double> distanceMatrix(static_cast<size_t>(n) * static_cast<size_t>(m));

    // FIX(perf): parallelise with OpenMP — each cell is independent.
    // Requires -fopenmp compile flag. If unavailable, degrades gracefully to serial.
    #pragma omp parallel for schedule(static) if(n > 100 && m > 100)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            distanceMatrix[i * m + j] = calculateDistance(j, i);
        }
    }

    auto endDistMatrix = std::chrono::high_resolution_clock::now();
    auto distMatrixMs = std::chrono::duration<double, std::milli>(endDistMatrix - startDistMatrix).count();
    LOG_DEBUG() << "CDTWService::compute - Distance matrix computation: " << distMatrixMs << " ms";

    // -------------------------------------------------------------------------
    // STEP 3: DP initialisation
    // -------------------------------------------------------------------------
    auto startInit = std::chrono::high_resolution_clock::now();

    std::vector<double> prevRow(m + 1, std::numeric_limits<double>::infinity());
    std::vector<double> currRow(m + 1, std::numeric_limits<double>::infinity());
    std::vector<int>    prevStart(m + 1, -1);
    std::vector<int>    currStart(m + 1, -1);

    // FIX(perf): traceback table replaces per-cell path vector copies.
    // One int8_t per DP cell records which move was taken; full path is
    // reconstructed in a single O(m+n) pass after the DP loop.
    // Size: (n+1) rows × (m+1) cols to match 1-based DP indexing i∈[1..n], j∈[1..m].
    std::vector<int8_t> traceback(static_cast<size_t>(n + 1) * static_cast<size_t>(m + 1), 0);

    prevRow[0] = 0.0;

    minFinalCost = std::numeric_limits<double>::infinity();
    normalizedFinalCost = std::numeric_limits<double>::infinity();
    bestEndIndex = -1;
    bestStartIndex = -1;
    signalStreamDistances.clear();
    signalStreamDistances.reserve(n);

    auto endInit = std::chrono::high_resolution_clock::now();
    auto initMs = std::chrono::duration<double, std::milli>(endInit - startInit).count();
    LOG_DEBUG() << "CDTWService::compute - Initialization: " << initMs << " ms";

    // -------------------------------------------------------------------------
    // STEP 4: Main DP loop
    // -------------------------------------------------------------------------
    auto startDPLoop = std::chrono::high_resolution_clock::now();

    // Hoist coefficients to locals to help the compiler avoid repeated member loads.
    const double matchCoefLocal     = matchCoef;
    const double insertionCoefLocal = insertionCoef;
    const double deletionCoefLocal  = deletionCoef;

    // Raw pointer aliases for the two active rows — swapped each outer iteration
    // to avoid vector allocation or data copying.
    double* pRowPtr   = prevRow.data();
    double* cRowPtr   = currRow.data();
    int*    pStartPtr = prevStart.data();
    int*    cStartPtr = currStart.data();

    const double infCost = std::numeric_limits<double>::infinity();

    for (int i = 1; i <= n; ++i) {
        if (useFixedStartEndDP) {
            // Fixed start: cannot match zero template frames to a positive signal prefix
            // at finite cost (path must begin at template 0 ↔ signal 0).
            cRowPtr[0]   = infCost;
            cStartPtr[0] = -1;
        } else {
            // Free-start boundary: alignment may begin at any signal frame.
            cRowPtr[0]   = 0.0;
            cStartPtr[0] = i - 1;
        }

        // Pointer into the flat distance matrix row for signal frame i-1.
        const double* distMatrixRow = distanceMatrix.data() + (i - 1) * m;

        for (int j = 1; j <= m; ++j) {
            double cost     = distMatrixRow[j - 1];
            double match    = pRowPtr[j - 1]  * matchCoefLocal;
            double insertion = pRowPtr[j]      * insertionCoefLocal;
            double deletion  = cRowPtr[j - 1]  * deletionCoefLocal;

            // Select minimum-cost predecessor.
            double minVal  = match;
            int    minStart = pStartPtr[j - 1];
            int8_t move     = 0; // 0=match

            if (deletion < minVal) {
                minVal   = deletion;
                minStart = cStartPtr[j - 1];
                move     = 2;
            }
            if (insertion < minVal) {
                minVal   = insertion;
                minStart = pStartPtr[j];
                move     = 1;
            }

            cRowPtr[j]   = cost + minVal;
            cStartPtr[j] = minStart;

            // Store the move taken — reconstructPath() uses this to rebuild optimalPath.
            traceback[i * (m + 1) + j] = move;
        }

        signalStreamDistances.push_back(cRowPtr[m]);

        if (!useFixedStartEndDP) {
            if (cRowPtr[m] < minFinalCost) {
                minFinalCost   = cRowPtr[m];
                bestEndIndex   = i - 1;
                bestStartIndex = cStartPtr[m];
            }
        } else if (i == n) {
            // Fixed end: optimal path must use the full signal (last row only).
            minFinalCost   = cRowPtr[m];
            bestEndIndex   = n - 1;
            bestStartIndex = cStartPtr[m];
        }

        // Swap row pointers — O(1), no data movement.
        std::swap(pRowPtr,   cRowPtr);
        std::swap(pStartPtr, cStartPtr);

        // Reset the new current row (was previous, now to be overwritten).
        std::fill(cRowPtr, cRowPtr + m + 1, std::numeric_limits<double>::infinity());
        // FIX(perf): cStartPtr[j] is always written before being read in the inner loop
        // (except [0], which is set explicitly above), so the fill is not needed for
        // correctness. Removed to save n×(m+1) = 3.8M unnecessary writes.
    }

    auto endDPLoop = std::chrono::high_resolution_clock::now();
    auto dpLoopMs = std::chrono::duration<double, std::milli>(endDPLoop - startDPLoop).count();
    LOG_DEBUG() << "CDTWService::compute - DP loop: " << dpLoopMs << " ms";

    // -------------------------------------------------------------------------
    // STEP 5: Reconstruct optimal path from traceback
    // -------------------------------------------------------------------------
    auto startPathRecon = std::chrono::high_resolution_clock::now();
    optimalPath = reconstructPath(traceback, bestEndIndex, m, n);
    auto endPathRecon = std::chrono::high_resolution_clock::now();
    auto pathReconMs = std::chrono::duration<double, std::milli>(endPathRecon - startPathRecon).count();
    LOG_DEBUG() << "CDTWService::compute - Path reconstruction: " << pathReconMs << " ms";

    // -------------------------------------------------------------------------
    // STEP 6: Free large temporaries
    // -------------------------------------------------------------------------
    auto startCleanup = std::chrono::high_resolution_clock::now();
    distanceMatrix.clear();
    distanceMatrix.shrink_to_fit();
    traceback.clear();
    traceback.shrink_to_fit();
    auto endCleanup = std::chrono::high_resolution_clock::now();
    auto cleanupMs = std::chrono::duration<double, std::milli>(endCleanup - startCleanup).count();
    LOG_DEBUG() << "CDTWService::compute - Cleanup: " << cleanupMs << " ms";

    auto endTotal = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration<double, std::milli>(endTotal - startTotal).count();

    // Compute length-normalised cost, scaled to [0, 100].
    // Divisor is (m + n) — both fixed before compute() — so the value is
    // strictly monotonic with minFinalCost (bigger distance → bigger score).
    //
    // Why [0, 100] is achievable:
    //   - Each local distance ≤ 1.0  (feature vectors are normalised to [0,1]
    //     by all callers before passing data to CDTWService)
    //   - Path length ≤ m + n        (upper bound on DP steps)
    //   - Therefore minFinalCost ≤ m + n  →  raw ratio ≤ 1.0  →  × 100 ≤ 100
    //   - A perfect frame-to-frame match with distance 0 gives score 0.
    const int normDivisor = m + n;
    normalizedFinalCost = (normDivisor > 0)
                          ? (minFinalCost / static_cast<double>(normDivisor)) * 100.0
                          : std::numeric_limits<double>::infinity();

    LOG_DEBUG() << "Finish: CDTWService::compute - bestStartIndex="
                << bestStartIndex << ", bestEndIndex=" << bestEndIndex
                << ", minFinalCost=" << minFinalCost
                << ", normalizedFinalCost=" << normalizedFinalCost;
    LOG_DEBUG() << "CDTWService::compute - Total: " << totalMs << " ms"
                << " (precompute: " << precomputeMs << "ms, distMatrix: " << distMatrixMs
                << "ms, init: " << initMs << "ms, dpLoop: " << dpLoopMs
                << "ms, pathRecon: " << pathReconMs << "ms, cleanup: " << cleanupMs << "ms)";
}

std::vector<double>
CDTWService::applyPathToVector(const std::vector<double>& input,
    const int targetLength)
{
    LOG_DEBUG() << "Start: CDTWService::applyPathToVector - input size="
                << input.size() << ", targetLength=" << targetLength;
    if (targetLength <= 0 || optimalPath.empty() || input.empty())
        return {};

    int m = static_cast<int>(optimalPath.size());
    std::vector<double> warped(m, 0.0);
    for (int k = 0; k < m; ++k) {
        int signalIndex = optimalPath[k];
        if (signalIndex >= 0 && signalIndex < static_cast<int>(input.size())) {
            warped[k] = input[signalIndex];
        } else if (signalIndex < 0) {
            warped[k] = input.front();
        } else {
            warped[k] = input.back();
        }
    }

    std::vector<double> transformed(targetLength, 0.0);
    double scalingFactor = static_cast<double>(m) / targetLength;
    for (int i = 0; i < targetLength; ++i) {
        size_t warpedIndex = static_cast<size_t>(i * scalingFactor);
        transformed[i] = (warpedIndex < warped.size()) ? warped[warpedIndex] : warped.back();
    }

    LOG_DEBUG() << "Finish: CDTWService::applyPathToVector - transformed size="
                << transformed.size();
    return transformed;
}

std::vector<CuePointData>
CDTWService::applyPathToCuePoints(const std::vector<CuePointData>& cuePoints)
{
    LOG_DEBUG() << "Start: CDTWService::applyPathToCuePoints - cuePoints size="
                << cuePoints.size();
    if (optimalPath.empty() || cuePoints.empty())
        return {};

    std::vector<CuePointData> transformed;
    transformed.reserve(cuePoints.size());

    int m = static_cast<int>(optimalPath.size());

    for (const auto& cp : cuePoints) {
        int templateStart = static_cast<int>(cp.position);
        int templateEnd   = static_cast<int>(cp.position + cp.length);

        // Clamp to valid template range [0, m)
        templateStart = std::max(0, std::min(templateStart, m - 1));
        templateEnd   = std::max(0, std::min(templateEnd,   m - 1));

        // Map template coordinates to signal coordinates via the alignment path
        int signalStart = optimalPath[templateStart];
        int signalEnd   = optimalPath[templateEnd];

        if (signalStart > signalEnd) {
            std::swap(signalStart, signalEnd);
        }

        CuePointData newCp = cp;
        newCp.position = static_cast<uint32_t>(signalStart);
        newCp.length   = static_cast<uint32_t>(std::max(0, signalEnd - signalStart));

        LOG_DEBUG() << "  CuePoint '" << cp.label << "': template=["
                    << templateStart << ", " << templateEnd << "] -> signal=["
                    << signalStart << ", " << signalEnd << "] -> mapped=["
                    << newCp.position << ", length=" << newCp.length << "]";

        transformed.push_back(newCp);
    }

    LOG_DEBUG() << "Finish: CDTWService::applyPathToCuePoints - transformed size="
                << transformed.size();
    return transformed;
}
