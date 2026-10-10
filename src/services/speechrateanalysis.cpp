#include "speechrateanalysis.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace speechrate {
namespace {

constexpr double kSampleRate = 8000.0;

int longestRun(const std::vector<int>& mask, int value)
{
    int best = 0;
    int current = 0;
    for (int sample : mask) {
        if (sample == value) {
            ++current;
            best = std::max(best, current);
        } else {
            current = 0;
        }
    }
    return best;
}

double powerMean(const std::vector<int>& lengths, int degree)
{
    if (lengths.empty() || degree == 0)
        return 0;
    if (lengths.size() == 1)
        return lengths.front();

    double acc = 0;
    for (int length : lengths)
        acc += std::pow(static_cast<double>(length), static_cast<double>(degree));
    acc /= static_cast<double>(lengths.size());
    const double mean = degree == 3
        ? std::cbrt(acc)
        : std::pow(acc, 1.0 / static_cast<double>(degree));
    // Keep an integer result on the integer side of trunc(), despite pow() noise.
    const double nearest = std::round(mean);
    if (std::fabs(mean - nearest) < 1e-8)
        return nearest;
    return mean;
}

double integerMedian(const std::vector<int>& lengths)
{
    if (lengths.empty())
        return 0;
    std::vector<int> sorted = lengths;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t count = sorted.size();
    if (count % 2 == 1)
        return sorted[count / 2];
    const int left = sorted[count / 2 - 1];
    const int right = sorted[count / 2];
    return (left + right) / 2;
}

double sumLengths(const std::vector<int>& lengths)
{
    double sum = 0;
    for (int length : lengths)
        sum += length;
    return sum;
}

std::vector<int> lengthsOf(const std::vector<Run>& runs)
{
    std::vector<int> lengths;
    lengths.reserve(runs.size());
    for (const Run& run : runs)
        lengths.push_back(run.length);
    return lengths;
}

} // namespace

double secondsFromFrames(double frameCount, int shift)
{
    return std::trunc(frameCount) * static_cast<double>(shift) / kSampleRate;
}

std::uint32_t minLengthFrames(int shift, int minLengthMs)
{
    if (shift <= 0)
        return 0;
    const double frames = (kSampleRate / static_cast<double>(shift)) / 1000.0 * minLengthMs;
    if (frames <= 0)
        return 0;
    return static_cast<std::uint32_t>(frames);
}

std::vector<double> intensity(const std::vector<float>& samples, int frame, int shift)
{
    std::vector<double> contour;
    if (frame <= 0 || shift <= 0 || samples.empty())
        return contour;

    const int half = static_cast<int>(std::lround(frame / 2.0));
    const int count = static_cast<int>(samples.size());
    int start = -half;
    int stop = half;
    while (stop < count - half) {
        double acc = 0;
        for (int index = start; index < stop; ++index) {
            if (index >= 0 && index < count)
                acc += std::fabs(static_cast<double>(samples[index]));
        }
        contour.push_back(acc / static_cast<double>(frame));
        start += shift;
        stop += shift;
    }
    return contour;
}

std::vector<double> normalize(const std::vector<double>& values)
{
    if (values.empty())
        return {};
    const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    const double minValue = *minIt;
    const double maxValue = *maxIt;
    if (maxValue == minValue)
        return {};
    std::vector<double> normalized;
    normalized.reserve(values.size());
    const double span = maxValue - minValue;
    for (double value : values)
        normalized.push_back((value - minValue) / span);
    return normalized;
}

std::vector<double> smooth(const std::vector<double>& values, int smoothLength)
{
    if (values.empty())
        return {};
    const int half = static_cast<int>(std::ceil(smoothLength / 2.0));
    const int window = half * 2;
    if (window <= 0)
        return values;

    const int count = static_cast<int>(values.size());
    std::vector<double> smoothed;
    smoothed.reserve(values.size());
    for (int index = 0; index < count; ++index) {
        double acc = 0;
        for (int tap = 0; tap < window; ++tap) {
            // Unsigned indexes underflow when index < half, and index 0 is excluded.
            if (tap + index < half)
                continue;
            const int position = tap + index - half;
            if (position > 0 && position < count)
                acc += values[position];
        }
        smoothed.push_back(acc / static_cast<double>(window));
    }
    return smoothed;
}

std::vector<Run> vowelNuclei(const std::vector<double>& normalized,
    const std::vector<double>& smoothed,
    double margin,
    std::uint32_t minFrames,
    bool flushTail)
{
    std::vector<Run> nuclei;
    const int count = static_cast<int>(std::min(normalized.size(), smoothed.size()));
    bool open = false;
    int start = 0;
    int length = 0;
    for (int index = 0; index < count; ++index) {
        const double difference = normalized[index] - smoothed[index] - margin;
        if (difference > 0 && !open) {
            start = index;
            length = 0;
            open = true;
        } else if (difference > 0 && open) {
            ++length;
        } else if (difference < 0 && open) {
            if (static_cast<std::uint32_t>(length) > minFrames)
                nuclei.push_back(Run { start, length });
            open = false;
        }
    }
    if (flushTail && open && static_cast<std::uint32_t>(length) > minFrames)
        nuclei.push_back(Run { start, length });
    return nuclei;
}

std::vector<Run> interiorGaps(const std::vector<Run>& nuclei)
{
    std::vector<Run> gaps;
    if (nuclei.size() < 2)
        return gaps;
    int cursor = nuclei.front().start + nuclei.front().length;
    for (std::size_t index = 1; index < nuclei.size(); ++index) {
        const Run& next = nuclei[index];
        gaps.push_back(Run { cursor, next.start - cursor });
        cursor = next.start + next.length;
    }
    return gaps;
}

double speechGateLevel(const std::vector<double>& contour, const Config& cfg)
{
    if (contour.empty())
        return cfg.minSpeechLevel;
    std::vector<double> sorted = contour;
    const double fraction = std::clamp(cfg.noiseFloorPercentile, 0.0, 1.0);
    const auto rank = static_cast<std::size_t>(fraction * static_cast<double>(sorted.size() - 1));
    std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(rank), sorted.end());
    return std::max(cfg.minSpeechLevel, sorted[rank] * cfg.speechOverNoise);
}

std::vector<Run> gateNuclei(const std::vector<Run>& nuclei,
    const std::vector<double>& contour,
    const Config& cfg)
{
    const double level = speechGateLevel(contour, cfg);
    const int count = static_cast<int>(contour.size());
    std::vector<Run> kept;
    kept.reserve(nuclei.size());
    for (const Run& nucleus : nuclei) {
        double peak = 0;
        const int end = std::min(nucleus.start + nucleus.length, count - 1);
        for (int index = std::max(0, nucleus.start); index <= end; ++index)
            peak = std::max(peak, contour[index]);
        if (peak >= level)
            kept.push_back(nucleus);
    }
    return kept;
}

Moments sampleMoments(const std::vector<int>& lengths)
{
    Moments moments;
    const int count = static_cast<int>(lengths.size());
    if (count < 1)
        return moments;

    double sum = 0;
    for (int length : lengths)
        sum += length;
    moments.mean = sum / static_cast<double>(count);
    if (count < 2)
        return moments;

    double square = 0;
    for (int length : lengths) {
        const double delta = length - moments.mean;
        square += delta * delta;
    }
    moments.variance = square / static_cast<double>(count - 1);
    const double deviation = std::sqrt(moments.variance);
    if (deviation == 0)
        return moments;

    double skew = 0;
    double kurt = 0;
    for (int length : lengths) {
        const double z = (length - moments.mean) / deviation;
        const double z2 = z * z;
        skew += z2 * z;
        kurt += z2 * z2;
    }
    moments.skewness = skew / static_cast<double>(count);
    moments.kurtosis = kurt / static_cast<double>(count) - 3.0;
    return moments;
}

Metrics metricsFromLengths(const std::vector<int>& vowelLengths,
    const std::vector<int>& gapLengths,
    double speechDuration,
    const Config& cfg,
    double vowelMaxFrames,
    double gapMaxFrames)
{
    Metrics metrics;
    if (!(speechDuration > 0) || cfg.shift <= 0)
        return metrics;

    metrics.speechDuration = speechDuration;
    metrics.vowelCount = static_cast<int>(vowelLengths.size());
    metrics.gapCount = static_cast<int>(gapLengths.size());
    metrics.vowelMoments = sampleMoments(vowelLengths);
    metrics.gapMoments = sampleMoments(gapLengths);

    const double vowelPower = powerMean(vowelLengths, cfg.degree);
    const double gapPower = powerMean(gapLengths, cfg.degree);
    const double vowelMedian = integerMedian(vowelLengths);
    const double gapMedian = integerMedian(gapLengths);

    metrics.vowelLength = secondsFromFrames(sumLengths(vowelLengths), cfg.shift);
    metrics.vowelMean = secondsFromFrames(vowelPower, cfg.shift);
    metrics.vowelMedian = secondsFromFrames(vowelMedian, cfg.shift);
    metrics.gapLength = secondsFromFrames(sumLengths(gapLengths), cfg.shift);
    metrics.gapMean = secondsFromFrames(gapPower, cfg.shift);
    metrics.gapMedian = secondsFromFrames(gapMedian, cfg.shift);
    metrics.vowelMax = secondsFromFrames(vowelMaxFrames, cfg.shift);
    metrics.gapMax = secondsFromFrames(gapMaxFrames, cfg.shift);

    metrics.speechRate = cfg.k1 * metrics.vowelCount * 60.0 / speechDuration;
    metrics.vowelsPerSecond = metrics.vowelCount / speechDuration;

    const double denominator = metrics.vowelLength
        + cfg.k2 * metrics.gapMedian * metrics.gapCount;
    if (denominator == 0) {
        metrics.articulationRate = metrics.speechRate;
    } else {
        metrics.articulationRate = metrics.speechRate * speechDuration / denominator;
        if (metrics.speechRate > metrics.articulationRate)
            metrics.articulationRate = metrics.speechRate;
    }

    if (metrics.gapMean < metrics.gapMedian || metrics.gapMedian == 0)
        metrics.phrasePauses = 0;
    else
        metrics.phrasePauses = cfg.k3 * (metrics.gapMean - metrics.gapMedian) / metrics.gapMedian;

    if (metrics.vowelMean < metrics.vowelMedian || metrics.vowelMedian == 0)
        metrics.fillerScore = 0;
    else
        metrics.fillerScore = cfg.k4 * (metrics.vowelMean - metrics.vowelMedian) / metrics.vowelMedian;

    metrics.valid = std::isfinite(metrics.speechRate) && std::isfinite(metrics.articulationRate)
        && std::isfinite(metrics.phrasePauses) && std::isfinite(metrics.fillerScore);
    return metrics;
}

void appendParts(Parts& base, const Parts& extra)
{
    base.vowelLengths.insert(base.vowelLengths.end(), extra.vowelLengths.begin(), extra.vowelLengths.end());
    base.gapLengths.insert(base.gapLengths.end(), extra.gapLengths.begin(), extra.gapLengths.end());
    base.speechDuration += extra.speechDuration;
    base.vowelMaxFrames = std::max(base.vowelMaxFrames, extra.vowelMaxFrames);
    base.gapMaxFrames = std::max(base.gapMaxFrames, extra.gapMaxFrames);
}

bool sameMeasurement(const Config& left, const Config& right)
{
    return left.frame == right.frame
        && left.shift == right.shift
        && left.smooth == right.smooth
        && left.minLengthMs == right.minLengthMs
        && left.degree == right.degree
        && left.k1 == right.k1
        && left.k2 == right.k2
        && left.k3 == right.k3
        && left.k4 == right.k4
        && left.speechOverNoise == right.speechOverNoise
        && left.minSpeechLevel == right.minSpeechLevel
        && left.noiseFloorPercentile == right.noiseFloorPercentile;
}

Metrics blendMetrics(const Metrics& left, const Metrics& right)
{
    if (!left.valid)
        return right;
    if (!right.valid)
        return left;
    const double leftTime = left.speechDuration;
    const double rightTime = right.speechDuration;
    const double total = leftTime + rightTime;
    if (!(total > 0))
        return {};

    const auto mix = [&](double leftValue, double rightValue) {
        return (leftValue * leftTime + rightValue * rightTime) / total;
    };

    Metrics out;
    out.speechDuration = total;
    out.speechRate = mix(left.speechRate, right.speechRate);
    out.articulationRate = mix(left.articulationRate, right.articulationRate);
    out.phrasePauses = mix(left.phrasePauses, right.phrasePauses);
    out.fillerScore = mix(left.fillerScore, right.fillerScore);
    out.vowelCount = left.vowelCount + right.vowelCount;
    out.gapCount = left.gapCount + right.gapCount;
    out.vowelLength = left.vowelLength + right.vowelLength;
    out.gapLength = left.gapLength + right.gapLength;
    out.vowelMax = std::max(left.vowelMax, right.vowelMax);
    out.gapMax = std::max(left.gapMax, right.gapMax);
    out.vowelMean = mix(left.vowelMean, right.vowelMean);
    out.vowelMedian = mix(left.vowelMedian, right.vowelMedian);
    out.gapMean = mix(left.gapMean, right.gapMean);
    out.gapMedian = mix(left.gapMedian, right.gapMedian);
    out.vowelsPerSecond = out.vowelCount / total;
    out.valid = std::isfinite(out.speechRate) && std::isfinite(out.articulationRate)
        && std::isfinite(out.phrasePauses) && std::isfinite(out.fillerScore);
    return out;
}

Measurement measure(const std::vector<float>& samples, const Config& cfg)
{
    Measurement measurement;
    if (samples.empty() || cfg.frame <= 0 || cfg.shift <= 0)
        return measurement;

    const std::vector<double> contour = intensity(samples, cfg.frame, cfg.shift);
    if (contour.empty())
        return {};
    const std::vector<double> normalized = normalize(contour);
    if (normalized.empty())
        return {};
    const std::vector<double> smoothed = smooth(normalized, cfg.smooth);
    const std::uint32_t minFrames = minLengthFrames(cfg.shift, cfg.minLengthMs);
    const std::vector<Run> nuclei = gateNuclei(
        vowelNuclei(normalized, smoothed, cfg.peakMargin, minFrames), contour, cfg);
    if (static_cast<int>(nuclei.size()) < std::max(1, cfg.minVowels))
        return {};
    const std::vector<Run> gaps = interiorGaps(nuclei);

    std::vector<int> mask(normalized.size(), 2);
    bool any = false;
    int low = 0;
    int high = 0;
    for (const Run& nucleus : nuclei) {
        const int end = nucleus.start + nucleus.length;
        if (!any) {
            low = nucleus.start;
            high = end;
            any = true;
        } else {
            low = std::min(low, nucleus.start);
            high = std::max(high, end);
        }
        for (int index = nucleus.start; index <= end && index < static_cast<int>(mask.size()); ++index) {
            if (index >= 0)
                mask[index] = 1;
        }
    }
    if (any) {
        const int last = std::min(high, static_cast<int>(mask.size()) - 1);
        for (int index = std::max(0, low); index <= last; ++index) {
            if (mask[index] == 2)
                mask[index] = 0;
        }
    }

    measurement.parts.vowelLengths = lengthsOf(nuclei);
    measurement.parts.gapLengths = lengthsOf(gaps);
    measurement.parts.speechDuration = static_cast<double>(samples.size()) / kSampleRate;
    measurement.parts.vowelMaxFrames = any ? longestRun(mask, 1) : 0;
    measurement.parts.gapMaxFrames = any ? longestRun(mask, 0) : 0;
    measurement.metrics = metricsFromLengths(measurement.parts.vowelLengths,
        measurement.parts.gapLengths,
        measurement.parts.speechDuration,
        cfg,
        measurement.parts.vowelMaxFrames,
        measurement.parts.gapMaxFrames);
    return measurement;
}

Metrics analyze(const std::vector<float>& samples, const Config& cfg)
{
    return measure(samples, cfg).metrics;
}

double fillerPercent(double fillerScore, double fillerMin, double fillerMax)
{
    if (!(fillerMax > fillerMin))
        return 0;
    const double clamped = std::clamp(fillerScore, fillerMin, fillerMax);
    return (clamped - fillerMin) / (fillerMax - fillerMin) * 100.0;
}

namespace {

double peakNacf(const std::vector<float>& samples, int sampleStart, int length)
{
    if (length <= 0 || samples.empty())
        return 0;
    const int available = static_cast<int>(samples.size());
    if (sampleStart < 0) {
        length += sampleStart;
        sampleStart = 0;
    }
    if (sampleStart >= available)
        return 0;
    if (sampleStart + length > available)
        length = available - sampleStart;
    if (length <= 0)
        return 0;

    const int lagMin = static_cast<int>(std::ceil(kSampleRate / kVoicingMaxHz));
    const int lagMax = static_cast<int>(std::floor(kSampleRate / kVoicingMinHz));
    if (lagMin < 1 || lagMax < lagMin || length <= lagMax)
        return 0;

    const float* window = samples.data() + sampleStart;
    double energy = 0;
    for (int index = 0; index < length; ++index) {
        const double sample = window[index];
        energy += sample * sample;
    }
    // Below this, autocorrelation of numerical dust is not voicing.
    if (energy < 1.0)
        return 0;

    double best = 0;
    for (int lag = lagMin; lag <= lagMax; ++lag) {
        double correlation = 0;
        const int overlap = length - lag;
        for (int index = 0; index < overlap; ++index) {
            correlation += static_cast<double>(window[index])
                * static_cast<double>(window[index + lag]);
        }
        best = std::max(best, correlation / energy);
    }
    return best;
}

bool frameVoiced(const std::vector<float>& samples, int frameIndex, int frame, int shift)
{
    const int half = static_cast<int>(std::lround(frame / 2.0));
    return peakNacf(samples, frameIndex * shift - half, frame) >= kVoicingNacf;
}

} // namespace

DurationStats durationStatistics(const std::vector<double>& durationsSec)
{
    DurationStats stats;
    std::vector<double> durations;
    durations.reserve(durationsSec.size());
    for (double duration : durationsSec) {
        if (std::isfinite(duration))
            durations.push_back(std::max(0.0, duration));
    }
    stats.count = static_cast<int>(durations.size());
    if (stats.count < 1)
        return stats;

    stats.min = durations.front();
    stats.max = durations.front();
    double sum = 0;
    for (double duration : durations) {
        sum += duration;
        stats.min = std::min(stats.min, duration);
        stats.max = std::max(stats.max, duration);
    }
    stats.mean = sum / static_cast<double>(stats.count);

    std::vector<double> sorted = durations;
    std::sort(sorted.begin(), sorted.end());
    const int mid = stats.count / 2;
    if (stats.count % 2 == 1)
        stats.median = sorted[static_cast<std::size_t>(mid)];
    else
        stats.median = 0.5 * (sorted[static_cast<std::size_t>(mid - 1)] + sorted[static_cast<std::size_t>(mid)]);

    if (stats.count >= 2) {
        double square = 0;
        for (double duration : durations) {
            const double delta = duration - stats.mean;
            square += delta * delta;
        }
        stats.stddev = std::sqrt(square / static_cast<double>(stats.count - 1));
    }

    const double binWidth = kVowelHistogramBinSec;
    stats.histogramBinSec = binWidth;
    int needed = static_cast<int>(std::floor(stats.max / binWidth)) + 1;
    if (needed < 1)
        needed = 1;
    const bool overflow = needed > kVowelHistogramMaxBins;
    const int bins = overflow ? kVowelHistogramMaxBins : needed;
    stats.histogram.resize(static_cast<std::size_t>(bins));
    for (int index = 0; index < bins; ++index) {
        HistogramBin& bin = stats.histogram[static_cast<std::size_t>(index)];
        bin.startSec = index * binWidth;
        bin.endSec = (index + 1) * binWidth;
        bin.openEnded = overflow && index == bins - 1;
    }
    for (double duration : durations) {
        int binIndex = static_cast<int>(std::floor(duration / binWidth));
        if (binIndex < 0)
            binIndex = 0;
        if (binIndex >= bins)
            binIndex = bins - 1;
        ++stats.histogram[static_cast<std::size_t>(binIndex)].count;
    }
    return stats;
}

int countPhrasalPauses(const std::vector<std::uint8_t>& silent, int shiftSamples, int thresholdMs)
{
    if (shiftSamples <= 0 || silent.empty())
        return 0;
    const int count = static_cast<int>(silent.size());
    int firstSpeech = -1;
    int lastSpeech = -1;
    for (int index = 0; index < count; ++index) {
        if (silent[static_cast<std::size_t>(index)] == 0) {
            if (firstSpeech < 0)
                firstSpeech = index;
            lastSpeech = index;
        }
    }
    if (firstSpeech < 0 || lastSpeech <= firstSpeech)
        return 0;

    if (thresholdMs < 0)
        thresholdMs = 0;
    const double frameSec = static_cast<double>(shiftSamples) / kSampleRate;
    const double thresholdSec = static_cast<double>(thresholdMs) / 1000.0;

    int pauses = 0;
    int run = 0;
    const auto closeRun = [&]() {
        if (run > 0 && static_cast<double>(run) * frameSec + 1e-9 >= thresholdSec)
            ++pauses;
        run = 0;
    };
    for (int index = firstSpeech + 1; index < lastSpeech; ++index) {
        if (silent[static_cast<std::size_t>(index)] != 0)
            ++run;
        else
            closeRun();
    }
    closeRun();
    return pauses;
}

RecordingSummary summarizeRecording(const std::vector<float>& samples,
    const Config& cfg,
    int pauseThresholdMs)
{
    RecordingSummary summary;
    summary.pauseThresholdMs = pauseThresholdMs < 0 ? 0 : pauseThresholdMs;
    if (samples.empty() || cfg.frame <= 0 || cfg.shift <= 0)
        return summary;

    const std::vector<double> contour = intensity(samples, cfg.frame, cfg.shift);
    summary.valid = true;
    if (contour.empty())
        return summary;

    const int frames = static_cast<int>(contour.size());
    const double gate = speechGateLevel(contour, cfg);
    std::vector<std::uint8_t> silent(static_cast<std::size_t>(frames), 0);
    std::vector<std::uint8_t> voiced(static_cast<std::size_t>(frames), 0);
    for (int index = 0; index < frames; ++index) {
        const double level = contour[static_cast<std::size_t>(index)];
        if (level < cfg.minSpeechLevel) {
            silent[static_cast<std::size_t>(index)] = 1;
            continue;
        }
        if (frameVoiced(samples, index, cfg.frame, cfg.shift))
            voiced[static_cast<std::size_t>(index)] = 1;
        if (level < gate && voiced[static_cast<std::size_t>(index)] == 0)
            silent[static_cast<std::size_t>(index)] = 1;
    }

    std::vector<Run> nuclei;
    const std::vector<double> normalized = normalize(contour);
    if (!normalized.empty()) {
        const std::vector<double> smoothed = smooth(normalized, cfg.smooth);
        const std::uint32_t minFrames = minLengthFrames(cfg.shift, cfg.minLengthMs);
        nuclei = gateNuclei(
            vowelNuclei(normalized, smoothed, cfg.peakMargin, minFrames, true), contour, cfg);
    }

    std::vector<Run> kept;
    kept.reserve(nuclei.size());
    for (const Run& nucleus : nuclei) {
        int peak = nucleus.start;
        double best = -1;
        const int end = std::min(nucleus.start + nucleus.length, frames - 1);
        for (int index = std::max(0, nucleus.start); index <= end; ++index) {
            if (contour[static_cast<std::size_t>(index)] > best) {
                best = contour[static_cast<std::size_t>(index)];
                peak = index;
            }
        }
        if (peak >= 0 && peak < frames && voiced[static_cast<std::size_t>(peak)] != 0)
            kept.push_back(nucleus);
    }

    std::vector<double> durations;
    durations.reserve(kept.size());
    for (const Run& nucleus : kept)
        durations.push_back(secondsFromFrames(nucleus.length, cfg.shift));

    summary.vowelCount = static_cast<int>(kept.size());
    summary.vowelDurations = durationStatistics(durations);
    summary.phrasalPauseCount = countPhrasalPauses(silent, cfg.shift, summary.pauseThresholdMs);
    return summary;
}

} // namespace speechrate
