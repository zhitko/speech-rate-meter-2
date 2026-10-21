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
 * A phrasal pause is an interior run of frames that are both quiet and
 * unvoiced, lasting at least pauseThresholdMs (default kDefaultPhrasalPauseMs).
 * Quiet is below silenceThreshold(): about 25 dB under the 98th percentile of
 * the intensity contour, raised toward the noise floor and capped so it cannot
 * climb into ordinary speech. A flat contour keeps the older noise-floor gate.
 * A frame below that level still counts as speech when it is voiced, so a
 * quiet vowel is not a pause. An unvoiced consonant with energy (a fricative)
 * stays above the level. A stop closure is quiet and unvoiced, and the
 * minimum length is what keeps a typical closure out of the pause list.
 * Silence before the first non-silent frame and after the last is not a
 * pause. Run duration is frame count times the intensity hop.
 *
 * Direct speech rate uses that same mask. T_span runs from the first
 * non-silent frame through the last, so interior pauses stay in and leading
 * and trailing silence stay out:
 *   R_s = K1 * N_v * 60 / T_span
 * Direct articulation then removes the pause sum from that span:
 *   R_a = K1 * N_v * 60 / (T_span - sum of pause durations)
 * K1 is shared, so the two direct rates match when there is no pause. N_v is
 * the tempo vowel count, not the stricter summary count. The legacy speech
 * rate keeps the whole buffer as T_s. The legacy articulation formula and
 * the legacy pause score stay available.
 *
 * Window articulation uses the analyzed buffer as T_window, not T_span:
 *   R_a = K5 * N_v * 60 / (T_window - sum of pause durations)
 * While recording, that buffer is the analysis window, or the shorter
 * buffer at the start of a session. A phrase or a whole file uses its own
 * length, so the vowel count and the pause sum stay on the same interval.
 * Leading and trailing silence stay in T_window and are not pauses, so they
 * count as articulation time. K5 defaults to 0.71, the same scale as K1.
 * Unlike Direct and Legacy, this rate is not raised to the speech rate.
 *
 * Live metrics use the current analysis window (the same audio as speech
 * rate), not a total since Start. The whole-recording summary is the
 * cumulative view: it includes silence between phrases and reports the
 * pause-duration distribution plus both pause estimates.
 *
 * Limitations: hop quantization (15 ms at the default shift) and the one-frame
 * short nucleus length can drop or shorten very short vowels. The shortest-vowel
 * setting compares that stored duration, in milliseconds, and keeps a run when
 * it is at least that long. 0 ms also keeps a single intensity point. The
 * default 5 ms still drops that point. A stop closure longer than the pause
 * minimum is counted.
 * Leading and trailing silence stay in T_s (legacy speech rate, and the
 * Speech time) and out of T_span. They are not part of the pause sum.
 */
namespace speechrate {

constexpr int kDefaultPhrasalPauseMs = 200;
constexpr double kVoicingNacf = 0.45;
constexpr double kVoicingMinHz = 75.0;
constexpr double kVoicingMaxHz = 400.0;
constexpr double kVowelHistogramBinSec = 0.020;
constexpr int kVowelHistogramMaxBins = 30;
// Pause bars are wider than vowel bars: the default minimum is 200 ms.
constexpr double kPauseHistogramBinSec = 0.050;
constexpr int kPauseHistogramMaxBins = 40;
// Speech reference for the adaptive silence level (see silenceThreshold).
constexpr double kSilenceSpeechPercentile = 0.98;
// 10^(-25/20): 25 dB below that reference.
constexpr double kSilenceRelative = 0.05623413251903491;
constexpr double kSilenceContrast = 1.5;
constexpr double kSilenceMaxFraction = 0.20;

/** Which articulation rate and pause tile the tempo path publishes. */
enum class ArticulationMethod {
    Direct = 0,
    Legacy = 1,
    Window = 2,
};

/** Values other than Legacy and Window stay Direct, including a missing field. */
inline ArticulationMethod articulationMethodOf(int method)
{
    if (method == static_cast<int>(ArticulationMethod::Legacy))
        return ArticulationMethod::Legacy;
    if (method == static_cast<int>(ArticulationMethod::Window))
        return ArticulationMethod::Window;
    return ArticulationMethod::Direct;
}

/** Which speech rate the tempo path publishes. */
enum class SpeechMethod {
    Direct = 0,
    Legacy = 1,
};

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
    // Window articulation scale. 0.71 matches K1, so the two rates share a scale.
    double k5 = 0.71;
    double peakMargin = 0.009;
    // Direct articulation subtracts interior pauses at least this long.
    int pauseThresholdMs = kDefaultPhrasalPauseMs;
    ArticulationMethod articulationMethod = ArticulationMethod::Direct;
    SpeechMethod speechRateMethod = SpeechMethod::Direct;
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
    double speechDuration = 0; // T_s, the whole buffer
    double speechSpanSec = 0; // T_span used by the direct rate
    double speechRate = 0; // published R_s (direct or legacy)
    double articulationRate = 0; // published R_a (direct, legacy, or window)
    double phrasePauses = 0; // published pause value (direct mean, or legacy P)
    double directSpeechRate = 0;
    double legacySpeechRate = 0;
    double directArticulationRate = 0;
    double legacyArticulationRate = 0;
    double windowArticulationRate = 0;
    double legacyPauseScore = 0; // P from the indirect gap formula
    int pauseCount = 0; // N_p
    double pauseTotal = 0; // sum of interior pause durations, seconds
    double pauseMean = 0; // direct T_p
    double pauseRatio = 0; // pauseTotal / T_s
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
 * minFrames is an inclusive minimum stored length. 0 keeps a single
 * intensity point (stored length 0).
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
/**
 * Inclusive minimum stored length, in intensity frames, for a shortest-vowel
 * setting. vowelNuclei keeps a run when `length >=` this value, which is the
 * same as stored duration >= minLengthMs. 0 ms returns 0. At a 120-sample
 * shift, 5 ms returns 1 (15 ms) and matches the old truncated gate.
 */
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
    // Interior phrasal pauses measured on this buffer. Joining phrases sums them.
    int pauseCount = 0;
    double pauseTotalSec = 0;
    // Utterance span for this buffer. Joining phrases sums the spans.
    // 0 means the span was not measured; the direct rate then uses T_s.
    double speechSpanSec = 0;
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
    double p25 = 0;
    double p75 = 0;
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
    double speechDurationSec = 0; // T_s of the whole buffer
    double pauseTotalSec = 0; // Σ T_p
    double pauseRatio = 0; // Σ T_p / T_s
    DurationStats vowelDurations;
    DurationStats pauseDurations;
    // Tempo comparison on the same buffer. paceValid is false when the
    // vowel gate rejects the buffer (fewer than minVowels).
    bool paceValid = false;
    int paceVowelCount = 0; // N_v used by R_s and R_a
    double speechSpanSec = 0;
    double speechRate = 0; // published R_s
    double directSpeechRate = 0;
    double legacySpeechRate = 0;
    double directArticulationRate = 0;
    double legacyArticulationRate = 0;
    double windowArticulationRate = 0;
    double directPauseMeanSec = 0;
    double legacyPauseScore = 0;
    double silenceLevel = 0;
};

/**
 * Arithmetic mean, statistical median, 25th and 75th percentiles, min, max,
 * and sample standard deviation of durations in seconds, plus a fixed-width
 * histogram. Percentiles use the linear rank (n − 1) * p. An empty list
 * returns zeros. Non-finite values are ignored. binSec and maxBins default
 * to the vowel histogram; pass the pause constants for pause durations.
 */
DurationStats durationStatistics(const std::vector<double>& durationsSec,
    double binSec = kVowelHistogramBinSec,
    int maxBins = kVowelHistogramMaxBins);

/**
 * Level below which a frame may be silence. Adaptive to the recording:
 * 25 dB under the 98th percentile of intensity, not below the noise-floor
 * gate, and not above 20% of that percentile. A contour with no contrast
 * returns the older speech-gate level.
 */
double silenceThreshold(const std::vector<double>& contour, const Config& cfg);

/**
 * R_s = K1 * N_v * 60 / T_span. A non-positive span returns 0.
 */
double directSpeechRate(int vowelCount, double speechSpanSec, double k1);

/**
 * R_a = K1 * N_v * 60 / (T_span − Σ T_p). speechSpanSec is the utterance
 * span, not the whole buffer. When the speaking time is not positive, or
 * the result would fall below the direct speech rate on that span, returns
 * that speech rate.
 */
double directArticulationRate(int vowelCount,
    double speechSpanSec,
    double pauseTotalSec,
    double k1);

/**
 * R_a = K5 * N_v * 60 / (T_window − Σ T_p). windowSec is the analyzed
 * buffer, which is the analysis window while recording (or the shorter
 * buffer at the start) and the phrase or file length otherwise. It is not
 * T_span: leading and trailing silence stay in the denominator. When that
 * speaking time is not positive, returns 0. The result is not raised to
 * the speech rate.
 */
double windowArticulationRate(int vowelCount,
    double windowSec,
    double pauseTotalSec,
    double k5);

/**
 * Time from the first non-silent frame through the last, in seconds.
 * A frame is non-silent when silent[i] == 0. Duration is frame count times
 * shiftSamples / 8000. An empty mask, or a mask with no speech frame,
 * returns 0. A single speech frame lasts one hop.
 */
double speechSpanSec(const std::vector<std::uint8_t>& silent, int shiftSamples);

/**
 * Copies the legacy headlines, fills the direct and window rates, and
 * publishes the selected speech rate, articulation rate, and pause value.
 * speechSpanSec is T_span for this buffer; 0 uses T_s, which is how a
 * segment stored before the span was recorded is read. Window articulation
 * uses T_s of this buffer as T_window. metricsFromLengths leaves the legacy
 * speech rate and the legacy articulation pair in the published fields.
 */
void applyPauseStats(Metrics& metrics,
    int pauseCount,
    double pauseTotalSec,
    double speechSpanSec,
    const Config& cfg);

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
 * Durations, in seconds, of the runs countPhrasalPauses would count.
 * Order is the order they occur. n frames last n * shiftSamples / 8000.
 */
std::vector<double> phrasalPauseDurations(const std::vector<std::uint8_t>& silent,
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

/**
 * How several phrase headlines become the session summary after Stop.
 * Weighted mean and weighted median use each sample's weight (phrase
 * duration). The others count each phrase once. Harmonic and geometric
 * means are 0 when any value is not positive. A trimmed mean drops the
 * lowest and highest quarter, and with fewer than 4 values it is the mean.
 */
enum class SummaryMethod {
    WeightedMean = 0,
    Mean = 1,
    Median = 2,
    WeightedMedian = 3,
    HarmonicMean = 4,
    GeometricMean = 5,
    TrimmedMean = 6,
    QuadraticMean = 7,
};

constexpr int kSummaryMethodCount = 8;

struct WeightedSample {
    double value = 0;
    double weight = 1;
};

/** Values outside 0 … kSummaryMethodCount − 1 become Median, the default. */
SummaryMethod summaryMethodOf(int method);

/**
 * Combine phrase headlines. An empty list, and a list of only non-finite
 * values, returns 0. A non-positive or non-finite weight is treated as 1.
 */
double summarizeSamples(const std::vector<WeightedSample>& samples, SummaryMethod method);

} // namespace speechrate

#endif
