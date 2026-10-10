#ifndef SPEECHRATEANALYSIS_H
#define SPEECHRATEANALYSIS_H

#include <cstdint>
#include <vector>

/**
 * Whole-recording summary (after Stop, and for Open File).
 *
 * Vowel nuclei reuse the live intensity detector, in the spirit of
 * de Jong & Wempe: short-time mean absolute amplitude, min-max normalized,
 * smoothed, then runs that rise above that smooth contour by peakMargin.
 * A nucleus is kept only when its raw peak clears the speech gate and the
 * loudest frame is voiced. Voicing is the peak normalized autocorrelation
 * at lags for kVoicingMinHz…kVoicingMaxHz; a frame is voiced at or above
 * kVoicingNacf.
 *
 * Durations are those stored run lengths converted to seconds. The stored
 * length is one frame shorter than the inclusive run (the same convention
 * as the live tempo formulas). Unlike the live detector, a nucleus that is
 * still open on the last frame is kept, because the recording is finished.
 * The statistics are the arithmetic mean, the statistical median, min, max,
 * and the sample standard deviation (n − 1). They are not the power mean
 * the tempo formulas use. The histogram uses fixed bins of
 * kVowelHistogramBinSec, with at most kVowelHistogramMaxBins bins; the last
 * bin is open-ended when durations run past that.
 *
 * A phrasal pause is an interior run of silent/unvoiced frames lasting at
 * least pauseThresholdMs (default kDefaultPhrasalPauseMs). A frame is silent
 * when its intensity is below the speech-gate level and it is not voiced.
 * Frames at or below minSpeechLevel are silence without a voicing test.
 * Silence before the first non-silent frame and after the last is not a
 * pause. Loud unvoiced consonants stay above the gate, so they are not pauses.
 * Run duration is frame count times the intensity hop.
 *
 * Limitations: hop quantization (15 ms at the default shift), the one-frame
 * short nucleus length, and the minimum-length rule can drop or shorten very
 * short vowels. Quiet speech near the noise floor can look like a pause.
 * The 150 ms floor is a conventional phrasal-pause cut and should be tuned
 * on real slow, medium, and fast samples. This path does not change the live
 * speech-rate, articulation, filler, or pause-score numbers.
 */
namespace speechrate {

constexpr int kDefaultPhrasalPauseMs = 150;
constexpr double kVoicingNacf = 0.45;
constexpr double kVoicingMinHz = 75.0;
constexpr double kVoicingMaxHz = 400.0;
constexpr double kVowelHistogramBinSec = 0.020;
constexpr int kVowelHistogramMaxBins = 30;

/**
 * Speech-rate analysis from vowel-like intensity peaks.
 * Time base is the constant 8000 Hz described in the technical specification.
 */
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
/**
 * Intensity peaks above the smooth contour.
 * flushTail keeps a nucleus that is still open on the last frame.
 * The live tempo path leaves the tail unflushed; the whole-recording
 * summary flushes it.
 */
std::vector<Run> vowelNuclei(const std::vector<double>& normalized,
    const std::vector<double>& smoothed,
    double margin,
    std::uint32_t minFrames,
    bool flushTail = false);
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

struct HistogramBin {
    double startSec = 0;
    double endSec = 0;
    int count = 0;
    // The last bin collects every duration at or above startSec when a
    // recording's vowels run past kVowelHistogramMaxBins.
    bool openEnded = false;
};

struct DurationStats {
    int count = 0;
    double mean = 0;
    double median = 0;
    double min = 0;
    double max = 0;
    double stddev = 0;
    double histogramBinSec = kVowelHistogramBinSec;
    std::vector<HistogramBin> histogram;
};

struct RecordingSummary {
    bool valid = false;
    int vowelCount = 0;
    int phrasalPauseCount = 0;
    int pauseThresholdMs = kDefaultPhrasalPauseMs;
    DurationStats vowelDurations;
};

/**
 * Arithmetic mean, statistical median, min, max, and sample standard
 * deviation of durations in seconds, plus a fixed-width histogram.
 * An empty list returns zeros. Non-finite values are ignored.
 */
DurationStats durationStatistics(const std::vector<double>& durationsSec);

/**
 * Interior silent/unvoiced runs whose duration is at least thresholdMs.
 * silent[i] != 0 marks a silent frame. Duration of n frames is
 * n * shiftSamples / 8000. Leading and trailing silence is ignored.
 * A buffer with no non-silent frame, or only one, has no phrasal pause.
 */
int countPhrasalPauses(const std::vector<std::uint8_t>& silent,
    int shiftSamples,
    int thresholdMs);

/**
 * One-shot summary of a whole recording. Does not affect live metrics.
 * An empty buffer or a non-positive frame/shift returns valid == false.
 * A buffer that can be framed is valid even when it holds no vowels.
 */
RecordingSummary summarizeRecording(const std::vector<float>& samples,
    const Config& cfg,
    int pauseThresholdMs = kDefaultPhrasalPauseMs);

} // namespace speechrate

#endif
