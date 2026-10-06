#ifndef SPEECHRATEANALYSIS_H
#define SPEECHRATEANALYSIS_H

#include <cstdint>
#include <vector>

/**
 * Speech-rate analysis from vowel-like intensity peaks.
 * Time base is the constant 8000 Hz described in the technical specification.
 */
namespace speechrate {

struct Config {
    int frame = 240;
    int shift = 120;
    int smooth = 120;
    int minLengthMs = 5;
    int degree = 3;
    double k1 = 0.71;
    double k2 = 1.2;
    double k3 = 0.30;
    double k4 = 100.0;
    double peakMargin = 0.009;
    // Speech gate. A nucleus counts only when its raw intensity peak reaches
    // max(minSpeechLevel, noise floor * speechOverNoise). The noise floor is the
    // noiseFloorPercentile of the raw intensity contour of the analyzed buffer.
    double speechOverNoise = 3.0;
    double minSpeechLevel = 80.0;
    double noiseFloorPercentile = 0.10;
    // Fewer gated nuclei than this means the buffer holds no measurable speech.
    int minVowels = 3;
};

struct Run {
    int start = 0;
    int length = 0;
};

struct Moments {
    double mean = 0;
    double variance = 0;
    double skewness = 0;
    double kurtosis = 0;
};

struct Metrics {
    bool valid = false;
    double speechDuration = 0; // T_s
    double speechRate = 0; // R_s
    double articulationRate = 0; // R_a
    double phrasePauses = 0; // P
    double fillerScore = 0; // raw F
    int vowelCount = 0;
    int gapCount = 0;
    double vowelLength = 0; // T_v
    double vowelMean = 0;
    double vowelMedian = 0;
    double gapLength = 0; // T_c
    double gapMean = 0;
    double gapMedian = 0;
    double vowelMax = 0;
    double gapMax = 0;
    double vowelsPerSecond = 0;
    Moments vowelMoments;
    Moments gapMoments;
};

std::vector<double> intensity(const std::vector<float>& samples, int frame, int shift);
std::vector<double> normalize(const std::vector<double>& values);
std::vector<double> smooth(const std::vector<double>& values, int smoothLength);
std::vector<Run> vowelNuclei(const std::vector<double>& normalized,
    const std::vector<double>& smoothed,
    double margin,
    std::uint32_t minFrames);
std::vector<Run> interiorGaps(const std::vector<Run>& nuclei);
/** Drops nuclei whose raw intensity peak stays below the speech gate. */
std::vector<Run> gateNuclei(const std::vector<Run>& nuclei,
    const std::vector<double>& contour,
    const Config& cfg);
double speechGateLevel(const std::vector<double>& contour, const Config& cfg);
Moments sampleMoments(const std::vector<int>& lengths);

double secondsFromFrames(double frameCount, int shift);
std::uint32_t minLengthFrames(int shift, int minLengthMs);

/**
 * Nucleus and gap lengths for one buffer.
 * speechDuration is T_s in seconds (sample count / 8000).
 * Max-run fields are in intensity frames.
 */
struct Parts {
    std::vector<int> vowelLengths;
    std::vector<int> gapLengths;
    double speechDuration = 0;
    double vowelMaxFrames = 0;
    double gapMaxFrames = 0;
};

struct Measurement {
    Metrics metrics;
    Parts parts;
};

/** Append one phrase onto a session collection. */
void appendParts(Parts& base, const Parts& extra);

/** True when both configs produce lengths that can be joined. */
bool sameMeasurement(const Config& left, const Config& right);

/**
 * Duration-weighted blend of two results.
 * Used when phrase settings differ and the length lists cannot be joined.
 */
Metrics blendMetrics(const Metrics& left, const Metrics& right);

/** Headline and detail metrics from stored nucleus and gap lengths.
 * speechDuration is T_s in seconds (sample count / 8000).
 */
Metrics metricsFromLengths(const std::vector<int>& vowelLengths,
    const std::vector<int>& gapLengths,
    double speechDuration,
    const Config& cfg,
    double vowelMaxFrames = 0,
    double gapMaxFrames = 0);

/** Full pipeline, including the lengths a session can join later. */
Measurement measure(const std::vector<float>& samples, const Config& cfg);

/**
 * Full pipeline. An empty, flat, or speechless recording (fewer than
 * cfg.minVowels gated nuclei) returns valid == false.
 */
Metrics analyze(const std::vector<float>& samples, const Config& cfg);

/** 0…100 label after clamping F into [fillerMin, fillerMax]. */
double fillerPercent(double fillerScore, double fillerMin, double fillerMax);

} // namespace speechrate

#endif
