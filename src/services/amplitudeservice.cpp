#include "amplitudeservice.h"
#include "helpers/logger.h"
#include "helpers/vectorutils.h"
#include <cmath>

AmplitudeService::AmplitudeService()
{
    LOG_DEBUG() << "Start: AmplitudeService constructor";
    LOG_DEBUG() << "Finish: AmplitudeService constructor";
}

AmplitudeService::~AmplitudeService()
{
    LOG_DEBUG() << "Start: AmplitudeService destructor";
    LOG_DEBUG() << "Finish: AmplitudeService destructor";
}

std::vector<double>
AmplitudeService::getAmplitude(const std::vector<double>& inputWaveData,
    int window, int shift)
{
    LOG_DEBUG() << "Start: getAmplitude - inputWaveData.size="
                << inputWaveData.size() << ", window=" << window
                << ", shift=" << shift;

    std::vector<double> result;
    if (window <= 0 || shift <= 0) {
        LOG_WARNING() << "getAmplitude called with non-positive window/shift";
        return result;
    }

    const size_t total = inputWaveData.size();
    int halfWindow = window / 2;

    for (size_t pos = 0; pos < total; pos += shift) {
        double sum = 0.0;
        int start = static_cast<int>(pos) - halfWindow;
        int end = start + window;

        for (int i = start; i < end; ++i) {
            if (i >= 0 && i < static_cast<int>(total)) {
                sum += std::abs(inputWaveData[i]);
            }
        }
        result.push_back(sum / window);
    }

    result = VectorUtils::normalizeFromTo(0.0, 1.0, result);

    LOG_DEBUG() << "Finish: getAmplitude - result.size=" << result.size();
    return result;
}

std::vector<double> AmplitudeService::getAmplitudeDerivative(
    const std::vector<double>& inputWaveData, int window, int shift)
{
    LOG_DEBUG() << "Start: getAmplitudeDerivative - inputWaveData.size="
                << inputWaveData.size() << ", window=" << window
                << ", shift=" << shift;

    std::vector<double> amp = getAmplitude(inputWaveData, window, shift);
    std::vector<double> deriv;
    deriv.reserve(amp.size());

    if (!amp.empty()) {
        deriv.push_back(0.0);
        for (size_t i = 1; i < amp.size(); ++i) {
            deriv.push_back(amp[i] - amp[i - 1]);
        }
    }

    // normalize derivative vector as well
    deriv = VectorUtils::normalizeFromTo(0.0, 1.0, deriv);

    LOG_DEBUG() << "Finish: getAmplitudeDerivative - deriv.size=" << deriv.size();
    return deriv;
}
