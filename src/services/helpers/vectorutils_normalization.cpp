#include "logger.h"
#include "vectorutils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

std::vector<double>
VectorUtils::normalizeFromTo(double from, double to,
    const std::vector<double>& data)
{
    LOG_DEBUG() << "Start: normalizeFromTo - from=" << from << ", to=" << to
                << ", data.size=" << data.size();

    if (data.empty()) {
        LOG_DEBUG() << "Finish: normalizeFromTo - result.size=0 (empty input)";
        return {};
    }

    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    for (double val : data) {
        if (val < minVal)
            minVal = val;
        if (val > maxVal)
            maxVal = val;
    }

    if (std::abs(maxVal - minVal) < std::numeric_limits<double>::epsilon()) {
        // All values are the same, return a vector of 'from'
        return std::vector<double>(data.size(), from);
    }

    std::vector<double> result;
    result.reserve(data.size());

    double range = maxVal - minVal;
    double targetRange = to - from;

    for (double val : data) {
        double normalized = from + ((val - minVal) / range) * targetRange;
        result.push_back(normalized);
    }

    LOG_DEBUG() << "Finish: normalizeFromTo - result.size=" << result.size();
    return result;
}

std::vector<double>
VectorUtils::normalizeByMinMax(const std::vector<double>& data,
    bool skipZeros)
{
    LOG_DEBUG() << "Start: normalizeByMinMax - data.size=" << data.size()
                << ", skipZeros=" << skipZeros;

    if (data.empty()) {
        LOG_DEBUG() << "Finish: normalizeByMinMax - result.size=0 (empty input)";
        return {};
    }

    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            if (val > maxVal)
                maxVal = val;
            continue;
        }
        if (val < minVal)
            minVal = val;
        if (val > maxVal)
            maxVal = val;
    }

    if (std::abs(maxVal - minVal) < std::numeric_limits<double>::epsilon()) {
        // All values are the same, return a vector of 'from'
        return std::vector<double>(data.size(), 0.0);
    }

    std::vector<double> result;
    result.reserve(data.size());

    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            result.push_back(0.0);
            continue;
        }
        double normalized = val - minVal;
        result.push_back(normalized);
    }

    LOG_DEBUG() << "Finish: normalizeByMinMax - result.size=" << result.size();
    return result;
}

std::vector<double> VectorUtils::normalizeTo(double to,
    const std::vector<double>& data)
{
    LOG_DEBUG() << "Start: normalizeTo - to=" << to
                << ", data.size=" << data.size();

    if (data.empty()) {
        LOG_DEBUG() << "Finish: normalizeTo - result.size=0 (empty input)";
        return {};
    }

    double maxVal = std::numeric_limits<double>::lowest();
    for (double val : data) {
        if (val > maxVal)
            maxVal = val;
    }

    if (std::abs(maxVal) < std::numeric_limits<double>::epsilon()) {
        // Max value is 0, cannot scale. Return 0s.
        return std::vector<double>(data.size(), 0.0);
    }

    std::vector<double> result;
    result.reserve(data.size());

    for (double val : data) {
        result.push_back((val / maxVal) * to);
    }

    LOG_DEBUG() << "Finish: normalizeTo - result.size=" << result.size();
    return result;
}

std::vector<double>
VectorUtils::normalizeByMean(const std::vector<double>& data, bool skipZeros)
{
    LOG_DEBUG() << "Start: normalizeByMean - data.size=" << data.size()
                << ", skipZeros=" << skipZeros;

    if (data.empty()) {
        LOG_DEBUG() << "Finish: normalizeByMean - result.size=0 (empty input)";
        return {};
    }

    double sum = 0.0;
    int count = 0;
    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            continue;
        }
        sum += val;
        count++;
    }

    // If all values are zero (or count is 0), mean is 0
    double mean = (count > 0) ? (sum / count) : 0.0;

    std::vector<double> result;
    result.reserve(data.size());

    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            result.push_back(0.0);
            continue;
        }
        result.push_back(val - mean);
    }

    LOG_DEBUG() << "Finish: normalizeByMean - result.size=" << result.size();
    return result;
}

std::vector<double>
VectorUtils::normalizeByMeanDeviation(const std::vector<double>& data,
    bool skipZeros)
{
    LOG_DEBUG() << "Start: normalizeByMeanDeviation - data.size=" << data.size()
                << ", skipZeros=" << skipZeros;

    if (data.empty()) {
        LOG_DEBUG()
            << "Finish: normalizeByMeanDeviation - result.size=0 (empty input)";
        return {};
    }

    double sum = 0.0;
    int count = 0;
    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            continue;
        }
        sum += val;
        count++;
    }

    // If all values are zero (or count is 0), mean is 0
    double mean = (count > 0) ? (sum / count) : 0.0;

    double deviationSum = 0.0;
    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            continue;
        }
        deviationSum += std::abs(val - mean);
    }

    // Calculate deviation only from non-zero values
    double deviation = (count > 0) ? (deviationSum / count) : 0.0;

    if (std::abs(deviation) < std::numeric_limits<double>::epsilon()) {
        // Deviation is 0 (all values are the same), return 0s
        return std::vector<double>(data.size(), 0.0);
    }

    std::vector<double> result;
    result.reserve(data.size());

    for (double val : data) {
        if (skipZeros && std::abs(val) < std::numeric_limits<double>::epsilon()) {
            // Skip zero values when skipZeros is true
            result.push_back(0.0);
            continue;
        }
        result.push_back((val - mean) / deviation);
    }

    LOG_DEBUG() << "Finish: normalizeByMeanDeviation - result.size="
                << result.size();
    return result;
}

std::vector<std::vector<double>>
VectorUtils::normalizeFromTo2D(double from, double to,
    const std::vector<std::vector<double>>& data)
{
    if (data.empty()) {
        return data;
    }

    // Find global minimum and maximum across all frames
    double globalMin = std::numeric_limits<double>::max();
    double globalMax = std::numeric_limits<double>::lowest();

    for (const auto& frame : data) {
        for (double val : frame) {
            if (val < globalMin)
                globalMin = val;
            if (val > globalMax)
                globalMax = val;
        }
    }

    // If all values are the same, return data as-is
    if (globalMin == globalMax) {
        return data;
    }

    // Normalize all frames using global min/max
    std::vector<std::vector<double>> result;
    result.reserve(data.size());

    double range = globalMax - globalMin;
    for (const auto& frame : data) {
        std::vector<double> normalizedFrame;
        normalizedFrame.reserve(frame.size());
        for (double val : frame) {
            double normalized = from + (val - globalMin) / range * (to - from);
            normalizedFrame.push_back(normalized);
        }
        result.push_back(normalizedFrame);
    }

    return result;
}
