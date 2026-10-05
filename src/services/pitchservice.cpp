#include "pitchservice.h"
#include "SPTK/analysis/pitch_extraction.h"
#include "helpers/logger.h"
#include <cmath>
#include <iostream>

PitchService::PitchService()
{
    LOG_DEBUG() << "Start: PitchService constructor";
    LOG_DEBUG() << "Finish: PitchService constructor";
}

PitchService::~PitchService()
{
    LOG_DEBUG() << "Start: PitchService destructor";
    LOG_DEBUG() << "Finish: PitchService destructor";
}

std::vector<double> PitchService::getPitch(
    const std::vector<double>& inputWaveData, PitchAlgorithm algorithm,
    double frameShift, double sampleRate, double minF0, double maxF0,
    double voicingThreshold, PitchOutputFormat outputFormat)
{
    LOG_DEBUG() << "Start: getPitch - inputWaveData.size=" << inputWaveData.size()
                << ", algorithm=" << static_cast<int>(algorithm)
                << ", frameShift=" << frameShift << ", sampleRate=" << sampleRate
                << ", minF0=" << minF0 << ", maxF0=" << maxF0
                << ", voicingThreshold=" << voicingThreshold
                << ", outputFormat=" << static_cast<int>(outputFormat);

    std::vector<double> result;

    // Map PitchAlgorithm to SPTK's Algorithms
    sptk::PitchExtraction::Algorithms sptkAlgorithm;
    switch (algorithm) {
    case PitchAlgorithm::RAPT:
        sptkAlgorithm = sptk::PitchExtraction::kRapt;
        break;
    case PitchAlgorithm::SWIPE:
        sptkAlgorithm = sptk::PitchExtraction::kSwipe;
        break;
    case PitchAlgorithm::REAPER:
        sptkAlgorithm = sptk::PitchExtraction::kReaper;
        break;
    case PitchAlgorithm::DIO:
        sptkAlgorithm = sptk::PitchExtraction::kDio;
        break;
    case PitchAlgorithm::Harvest:
        sptkAlgorithm = sptk::PitchExtraction::kHarvest;
        break;
    case PitchAlgorithm::NumAlgorithm:
        sptkAlgorithm = sptk::PitchExtraction::kNumAlgorithms;
        break;
    default:
        LOG_WARNING() << "Unknown algorithm";
        return result;
    }

    // Create PitchExtraction instance
    // Note: frame_shift is in samples (points)
    int frameShiftSamples = static_cast<int>(frameShift);
    sptk::PitchExtraction pitchExtractor(frameShiftSamples, sampleRate, minF0,
        maxF0, voicingThreshold, sptkAlgorithm);

    if (!pitchExtractor.IsValid()) {
        LOG_CRITICAL() << "PitchExtraction initialization failed";
        return result;
    }

    // Run pitch extraction
    std::vector<double> f0;
    std::vector<double> epochs;
    sptk::PitchExtractionInterface::Polarity polarity;

    if (!pitchExtractor.Run(inputWaveData, &f0, &epochs, &polarity)) {
        LOG_CRITICAL() << "Pitch extraction failed";
        return result;
    }

    // Convert output format
    switch (outputFormat) {
    case PitchOutputFormat::PITCH:
    case PitchOutputFormat::F0:
        // F0 values are already in Hz, return as is
        result = f0;
        break;
    case PitchOutputFormat::LOG_F0:
        // Convert to log(F0)
        result.reserve(f0.size());
        for (double value : f0) {
            if (value > 0.0) {
                result.push_back(std::log(value));
            } else {
                // For unvoiced frames (F0 = 0), keep as 0 or use a sentinel value
                result.push_back(0.0);
            }
        }
        break;
    default:
        LOG_WARNING() << "Unknown output format";
        return result;
    }

    LOG_DEBUG() << "Finish: getPitch - result.size=" << result.size();
    return result;
}

std::vector<double> PitchService::getPitchDerivative(
    const std::vector<double>& inputWaveData, PitchAlgorithm algorithm,
    double frameShift, double sampleRate, double minF0, double maxF0,
    double voicingThreshold, PitchOutputFormat outputFormat)
{
    LOG_DEBUG() << "Start: getPitchDerivative - inputWaveData.size=" << inputWaveData.size();

    std::vector<double> pitch = getPitch(inputWaveData, algorithm, frameShift,
        sampleRate, minF0, maxF0, voicingThreshold, outputFormat);

    std::vector<double> derivative(pitch.size(), 0.0);
    for (size_t i = 1; i < pitch.size(); ++i) {
        derivative[i] = pitch[i] - pitch[i - 1];
    }

    LOG_DEBUG() << "Finish: getPitchDerivative - result.size=" << derivative.size();
    return derivative;
}

std::vector<double> PitchService::keepCuePointSectors(
    const std::vector<double>& pitch,
    const std::vector<CuePointData>& cuePoints,
    double frameShift,
    double sampleRate)
{
    LOG_DEBUG() << "Start: keepCuePointSectors - pitch.size=" << pitch.size()
                << ", cuePoints.size=" << cuePoints.size();

    std::vector<double> result(pitch.size(), 0.0);

    // frameShift is in samples (same convention as getPitch uses internally)
    // Frame i covers sample range [i * frameShift, (i+1) * frameShift)
    for (size_t i = 0; i < pitch.size(); ++i) {
        uint32_t frameSample = static_cast<uint32_t>(std::round(
            static_cast<double>(i) * frameShift));

        for (const CuePointData& cue : cuePoints) {
            if (frameSample >= cue.position
                && frameSample < cue.position + cue.length) {
                result[i] = pitch[i];
                break;
            }
        }
    }

    LOG_DEBUG() << "Finish: keepCuePointSectors - result.size=" << result.size();
    return result;
}
