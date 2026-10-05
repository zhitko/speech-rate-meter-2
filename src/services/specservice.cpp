#include "specservice.h"
#include "SPTK/analysis/fast_fourier_transform_cepstral_analysis.h"
#include "SPTK/analysis/spectrum_extraction.h"
#include "helpers/logger.h"
#include <iostream>

SpecService::SpecService()
{
    LOG_DEBUG() << "Start: SpecService constructor";
    LOG_DEBUG() << "Finish: SpecService constructor";
}

SpecService::~SpecService()
{
    LOG_DEBUG() << "Start: SpecService destructor";
    LOG_DEBUG() << "Finish: SpecService destructor";
}

std::vector<std::vector<double>>
SpecService::getSpec(const std::vector<double>& inputWaveData,
    const std::vector<double>& f0, int fftLength,
    int frameShift, double sampleRate, bool f0Refinement)
{
    LOG_DEBUG() << "Start: getSpec - inputWaveData.size=" << inputWaveData.size()
                << ", f0.size=" << f0.size() << ", fftLength=" << fftLength
                << ", frameShift=" << frameShift << ", sampleRate=" << sampleRate
                << ", f0Refinement=" << f0Refinement;

    std::vector<std::vector<double>> result;

    // Validate FFT length - WORLD/CheapTrick typically requires minimum 1024 for
    // audio
    if (fftLength < 1024) {
        LOG_WARNING() << "FFT length " << fftLength
                      << " may be too small for reliable spectrum extraction. "
                      << "Recommended minimum is 1024 for " << sampleRate
                      << " Hz sample rate. Attempting anyway...";
    }

    // Create SpectrumExtraction instance using WORLD algorithm (CheapTrick)
    sptk::SpectrumExtraction specExtractor(fftLength, frameShift, sampleRate,
        f0Refinement,
        sptk::SpectrumExtraction::kWorld);

    if (!specExtractor.IsValid()) {
        LOG_CRITICAL() << "SpectrumExtraction initialization failed - FFT length: "
                       << fftLength << ", Frame shift: " << frameShift
                       << ", Sample rate: " << sampleRate;
        LOG_CRITICAL() << "Check if FFT length is appropriate for the sample rate "
                          "(try 1024, 2048, or 4096)";
        return result;
    }

    // Run spectrum extraction
    if (!specExtractor.Run(inputWaveData, f0, &result)) {
        LOG_CRITICAL() << "Spectrum extraction failed during execution";
        LOG_CRITICAL() << "Input size: " << inputWaveData.size()
                       << ", F0 frames: " << f0.size();

        // Check for WORLD/CheapTrick constraints
        double minSupportedF0 = 2.0 * sampleRate / fftLength;
        LOG_CRITICAL()
            << "Note: WORLD algorithm requires F0 > 2.0 * sampleRate / fftLength ("
            << minSupportedF0 << " Hz)";
        LOG_CRITICAL() << "Your current settings might produce F0 values below "
                          "this threshold. Try increasing FFT length or minF0.";
        return result;
    }

    LOG_DEBUG() << "Finish: getSpec - result.size=" << result.size();
    if (!result.empty()) {
        LOG_DEBUG() << "First spectrum frame size=" << result[0].size();
    }

    return result;
}

std::vector<std::vector<double>>
SpecService::getCepstr(const std::vector<double>& inputWaveData,
    const std::vector<double>& f0, int fftLength,
    int frameShift, double sampleRate, int numOrder,
    bool f0Refinement)
{
    LOG_DEBUG() << "Start: getCepstr - inputWaveData.size="
                << inputWaveData.size() << ", f0.size=" << f0.size()
                << ", fftLength=" << fftLength << ", frameShift=" << frameShift
                << ", sampleRate=" << sampleRate << ", numOrder=" << numOrder
                << ", f0Refinement=" << f0Refinement;

    // 1. Get power spectrum first
    std::vector<std::vector<double>> specData = getSpec(
        inputWaveData, f0, fftLength, frameShift, sampleRate, f0Refinement);

    if (specData.empty()) {
        LOG_WARNING() << "Failed to get spectrum for cepstrum calculation";
        return {};
    }

    // 2. Initialize cepstral analysis
    // Recommended settings: iterations=3, acceleration=0.0
    int numIteration = 3;
    double accelerationFactor = 0.0;

    sptk::FastFourierTransformCepstralAnalysis cepstrAnalysis(
        fftLength, numOrder, numIteration, accelerationFactor);

    if (!cepstrAnalysis.IsValid()) {
        LOG_CRITICAL()
            << "FastFourierTransformCepstralAnalysis initialization failed";
        return {};
    }

    sptk::FastFourierTransformCepstralAnalysis::Buffer buffer;
    std::vector<std::vector<double>> cepstrData;
    cepstrData.reserve(specData.size());

    // 3. Convert each spectrum frame to cepstrum
    for (const auto& specFrame : specData) {
        std::vector<double> cepstrFrame;
        if (!cepstrAnalysis.Run(specFrame, &cepstrFrame, &buffer)) {
            LOG_CRITICAL() << "Cepstrum calculation failed for a frame";
            return {};
        }
        cepstrData.push_back(std::move(cepstrFrame));
    }

    LOG_DEBUG() << "Finish: getCepstr - result.size=" << cepstrData.size();
    if (!cepstrData.empty()) {
        LOG_DEBUG() << "First cepstrum frame size=" << cepstrData[0].size();
    }

    return cepstrData;
}
