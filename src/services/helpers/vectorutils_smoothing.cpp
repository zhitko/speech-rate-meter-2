#include "dataanalysis.h"
#include "interpolation.h"
#include "linalg.h"
#include "logger.h"
#include "vectorutils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

std::vector<double>
VectorUtils::smoothMovingAverage(const std::vector<double>& data,
    int windowSize, bool skipZeros)
{
    LOG_DEBUG() << "Start: smoothMovingAverage - data.size=" << data.size()
                << ", windowSize=" << windowSize;

    if (data.empty())
        return {};
    if (windowSize <= 1)
        return data;

    int n = data.size();
    std::vector<double> result;
    result.reserve(n);
    int hw = windowSize / 2;

    for (int i = 0; i < n; ++i) {
        // Keep original value for zero (unvoiced) frames
        if (skipZeros && std::abs(data[i]) <= std::numeric_limits<double>::epsilon()) {
            result.push_back(0.0);
            continue;
        }

        double sum = 0.0;
        int count = 0;
        // Symmetric window around i, clipped to bounds
        int start = std::max(0, i - hw);
        int end = std::min(n - 1, i + hw);

        for (int j = start; j <= end; ++j) {
            // Skip zero (unvoiced) values to avoid pulling average toward zero
            if (!skipZeros || std::abs(data[j]) > std::numeric_limits<double>::epsilon()) {
                sum += data[j];
                count++;
            }
        }

        if (count > 0) {
            result.push_back(sum / count);
        } else {
            result.push_back(data[i]);
        }
    }

    LOG_DEBUG() << "Finish: smoothMovingAverage - result.size=" << result.size();
    return result;
}

std::vector<double> VectorUtils::smoothMedian(const std::vector<double>& data,
    int windowSize, bool skipZeros)
{
    LOG_DEBUG() << "Start: smoothMedian - data.size=" << data.size()
                << ", windowSize=" << windowSize;

    if (data.empty())
        return {};
    if (windowSize <= 1)
        return data;

    int n = data.size();
    std::vector<double> result;
    result.reserve(n);
    int hw = windowSize / 2;
    std::vector<double> windowRef;
    windowRef.reserve(windowSize);

    for (int i = 0; i < n; ++i) {
        // Keep original value for zero (unvoiced) frames
        if (skipZeros && std::abs(data[i]) <= std::numeric_limits<double>::epsilon()) {
            result.push_back(0.0);
            continue;
        }

        windowRef.clear();
        int start = std::max(0, i - hw);
        int end = std::min(n - 1, i + hw);

        for (int j = start; j <= end; ++j) {
            // Skip zero (unvoiced) values to avoid pulling median toward zero
            if (!skipZeros || std::abs(data[j]) > std::numeric_limits<double>::epsilon()) {
                windowRef.push_back(data[j]);
            }
        }

        if (windowRef.empty()) {
            result.push_back(data[i]);
            continue;
        }

        // Compute true median: average the two middle elements for even-sized
        // windows.
        size_t sz = windowRef.size();
        size_t mid = sz / 2;
        std::nth_element(windowRef.begin(), windowRef.begin() + mid, windowRef.end());
        double median = windowRef[mid];
        result.push_back(median);
    }

    LOG_DEBUG() << "Finish: smoothMedian - result.size=" << result.size();
    return result;
}

std::vector<double> VectorUtils::smoothGaussian(const std::vector<double>& data,
    int windowSize, double sigma, bool skipZeros)
{
    LOG_DEBUG() << "Start: smoothGaussian - data.size=" << data.size()
                << ", windowSize=" << windowSize << ", sigma=" << sigma
                << ", skipZeros=" << skipZeros;

    if (data.empty())
        return {};
    if (windowSize <= 1 || sigma <= 0.0)
        return data;

    // Ensure windowSize is odd for symmetry
    if (windowSize % 2 == 0)
        windowSize++;
    int hw = windowSize / 2;

    // Generate kernel
    std::vector<double> kernel(windowSize);
    double sumKernel = 0.0;
    for (int i = 0; i < windowSize; ++i) {
        double x = i - hw;
        double g = std::exp(-(x * x) / (2 * sigma * sigma));
        kernel[i] = g;
        sumKernel += g;
    }
    // Normalize kernel
    for (double& v : kernel)
        v /= sumKernel;

    int n = data.size();
    std::vector<double> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i) {
        // Keep original value for zero (unvoiced) frames
        if (skipZeros && std::abs(data[i]) <= std::numeric_limits<double>::epsilon()) {
            result.push_back(0.0);
            continue;
        }

        double sum = 0.0;
        double sumWeights = 0.0;

        for (int k = 0; k < windowSize; ++k) {
            int idx = i - hw + k;
            // Valid boundary check: use only valid indices.
            // To handle edges properly with normalized kernel, we only sum weights of
            // valid neighbors and re-normalize.
            if (idx >= 0 && idx < n) {
                // Skip zero (unvoiced) values to avoid pulling average toward zero
                if (!skipZeros || std::abs(data[idx]) > std::numeric_limits<double>::epsilon()) {
                    sum += data[idx] * kernel[k];
                    sumWeights += kernel[k];
                }
            }
        }

        if (sumWeights > std::numeric_limits<double>::epsilon()) {
            result.push_back(sum / sumWeights);
        } else {
            result.push_back(data[i]); // Fallback
        }
    }

    LOG_DEBUG() << "Finish: smoothGaussian - result.size=" << result.size();
    return result;
}

namespace {
struct SplineData {
    std::vector<double> xVec;
    std::vector<double> yVec;
    double firstNonZeroIdx = -1.0;
    double lastNonZeroIdx = -1.0;
};

SplineData prepareSplineVectors(const std::vector<double>& data)
{
    SplineData sd;
    sd.xVec.reserve(data.size());
    sd.yVec.reserve(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        if (std::abs(data[i]) > std::numeric_limits<double>::epsilon()) {
            if (sd.firstNonZeroIdx < -0.5)
                sd.firstNonZeroIdx = static_cast<double>(i);
            sd.lastNonZeroIdx = static_cast<double>(i);
            sd.xVec.push_back(static_cast<double>(i));
            sd.yVec.push_back(data[i]);
        }
    }
    return sd;
}
} // namespace

std::vector<double> VectorUtils::smoothSpline(const std::vector<double>& data,
    double penalty)
{
    LOG_DEBUG() << "Start: smoothSpline - data.size=" << data.size()
                << ", penalty=" << penalty;

    // Use Alglib's spline1dfitpenalized
    if (data.empty())
        return {};

    SplineData sd = prepareSplineVectors(data);
    if (sd.xVec.size() < 2)
        return data;

    alglib::real_1d_array x, y;
    x.setcontent(sd.xVec.size(), sd.xVec.data());
    y.setcontent(sd.yVec.size(), sd.yVec.data());

    // M: number of basis functions. The more, the closer to interpolation.
    // Recommended M is usually N/2 or N (but penalized).
    // If M is small, it acts as regression.
    // We'll set M = min(50, N) to allow reasonable flexibility.
    int m = std::min((int)sd.xVec.size(), 50);
    if (m < 4)
        m = sd.xVec.size(); // minimum 4 points for cubic spline

    alglib::spline1dinterpolant s;
    alglib::spline1dfitreport rep;
    double rho = penalty;
    // Note: rho is usually log scale or so. In alglib:
    // Minimize S = E + rho*C (C is curvature).
    // If rho is -ve, it's automatically selected? No, documentation says rho >=
    // 0. If rho is very large -> linear fit. If rho -> 0, natural spline
    // interpolation.

    alglib::ae_int_t info;
    try {
        alglib::spline1dfitpenalized(x, y, m, rho, info, s, rep);
    } catch (...) {
        LOG_CRITICAL() << "Alglib error in smoothSpline";
        return data;
    }

    int n = data.size();
    std::vector<double> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i) {
        double t = static_cast<double>(i);
        // Only evaluate if within range of valid points?
        // Spline is defined on [x_min, x_max].
        // Our xVec is subset of indices.
        if (t < sd.firstNonZeroIdx || t > sd.lastNonZeroIdx) {
            // Outside valid data range, keep 0 or original?
            // Since prepareSplineVectors skipped zeros/small values, those were
            // considered 'missing'. We should probably keep them as 0.0 or
            // interpolate? If we smooth, we probably want to fill gaps? But existing
            // code sets them to 0.0.
            result.push_back(0.0);
        } else {
            // Evaluate spline
            result.push_back(alglib::spline1dcalc(s, t));
        }
    }

    LOG_DEBUG() << "Finish: smoothSpline - result.size=" << result.size();
    return result;
}

std::vector<double> VectorUtils::smooth(const std::string& type,
    const std::vector<double>& data,
    double param1, double param2, bool skipZeros)
{
    if (type == "MovingAverage")
        return smoothMovingAverage(data, static_cast<int>(param1), skipZeros);
    if (type == "Median")
        return smoothMedian(data, static_cast<int>(param1), skipZeros);
    if (type == "Gaussian")
        return smoothGaussian(data, static_cast<int>(param1),
            param2 > 0 ? param2 : 1.0);

    if (type == "Spline")
        return smoothSpline(data, param1);

    // Default or unknown
    return smoothMovingAverage(data, static_cast<int>(param1), skipZeros);
}
