#include "speechrateanalysis.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void expectNear(double actual, double expected, const std::string& message)
{
    if (!(std::fabs(actual - expected) <= 1e-9)) {
        std::cerr << "FAIL: " << message << " actual=" << actual << " expected=" << expected << '\n';
        ++g_failures;
    }
}

void testWorkedExample()
{
    speechrate::Config cfg;
    const speechrate::Metrics metrics = speechrate::metricsFromLengths({ 4, 10 }, { 40 }, 10.0, cfg, 0, 0);

    expect(metrics.valid, "worked example is valid");
    expectNear(metrics.speechDuration, 10.0, "T_s");
    expect(metrics.vowelCount == 2, "N_v");
    expect(metrics.gapCount == 1, "N_c");
    expectNear(metrics.vowelMean, 0.120, "T_v_mean");
    expectNear(metrics.vowelMedian, 0.105, "T_v_med");
    expectNear(metrics.vowelLength, 0.210, "T_v");
    expectNear(metrics.gapMean, 0.600, "T_c_mean");
    expectNear(metrics.gapMedian, 0.600, "T_c_med");
    expectNear(metrics.speechRate, 0.71 * 2.0 * 60.0 / 10.0, "R_s");
    expectNear(metrics.phrasePauses, 0.0, "P");
    expectNear(metrics.fillerScore, 100.0 * (0.120 - 0.105) / 0.105, "F");

    const double expectedArticulation = metrics.speechRate * 10.0 / (0.210 + 1.2 * 0.600);
    expectNear(metrics.articulationRate, expectedArticulation, "R_a");
    expect(metrics.articulationRate > metrics.speechRate, "articulation stays above speech rate");
    expectNear(speechrate::fillerPercent(metrics.fillerScore, 120, 240), 0.0, "filler label");
}

void testIntegerMedian()
{
    speechrate::Config cfg;
    const speechrate::Metrics metrics = speechrate::metricsFromLengths({ 4, 5 }, {}, 10.0, cfg);
    expectNear(metrics.vowelMedian, 4.0 * 120.0 / 8000.0, "even median uses integer division");
}

void testMinLengthFrames()
{
    expect(speechrate::minLengthFrames(120, 5) == 0, "default minimum truncates to 0");
}

void testSmoothEdgeRule()
{
    const std::vector<double> smoothed = speechrate::smooth({ 0, 1, 1, 1 }, 2);
    expect(smoothed.size() == 4, "smooth length");
    expectNear(smoothed[0], 0.0, "index 0 excluded and left edge dropped");
    expectNear(smoothed[1], 0.5, "one in-range tap");
    expectNear(smoothed[2], 1.0, "two in-range taps");
}

void testShortNucleusDropped()
{
    const std::vector<double> normalized { 1, 0 };
    const std::vector<double> smoothed { 0, 0 };
    const std::vector<speechrate::Run> kept = speechrate::vowelNuclei(normalized, smoothed, 0.009, 0);
    expect(kept.empty(), "a one-sample nucleus is dropped and an open tail is not flushed");

    const std::vector<double> longer { 1, 1, 0 };
    const std::vector<speechrate::Run> nuclei = speechrate::vowelNuclei(longer, { 0, 0, 0 }, 0.009, 0);
    expect(nuclei.size() == 1, "two-sample nucleus is kept");
    expect(nuclei[0].start == 0 && nuclei[0].length == 1, "stored length is run length minus one");
}

void testGapsIgnoreTails()
{
    const std::vector<speechrate::Run> gaps = speechrate::interiorGaps({ { 2, 1 }, { 10, 4 } });
    expect(gaps.size() == 1, "one interior gap");
    expect(gaps[0].start == 3 && gaps[0].length == 7, "gap is between nuclei only");
}

void testEmptyAndFlat()
{
    expect(!speechrate::analyze({}, {}).valid, "empty recording");
    expect(!speechrate::analyze(std::vector<float>(1600, 0), {}).valid, "flat recording");
}

void testCollectedPhrases()
{
    speechrate::Config cfg;
    speechrate::Parts first;
    first.vowelLengths = { 4, 10 };
    first.gapLengths = { 40 };
    first.speechDuration = 10;
    speechrate::Parts second;
    second.vowelLengths = { 6, 8, 9 };
    second.gapLengths = { 20, 30 };
    second.speechDuration = 5;
    second.vowelMaxFrames = 9;
    second.gapMaxFrames = 30;

    speechrate::Parts all = first;
    speechrate::appendParts(all, second);
    const speechrate::Metrics metrics = speechrate::metricsFromLengths(
        all.vowelLengths, all.gapLengths, all.speechDuration, cfg, all.vowelMaxFrames, all.gapMaxFrames);
    const speechrate::Metrics left = speechrate::metricsFromLengths(
        first.vowelLengths, first.gapLengths, first.speechDuration, cfg, 0, 0);
    const speechrate::Metrics right = speechrate::metricsFromLengths(
        second.vowelLengths, second.gapLengths, second.speechDuration, cfg, 9, 30);

    expect(metrics.vowelCount == 5, "collected vowel count");
    expect(metrics.gapCount == 3, "collected gap count");
    expectNear(metrics.speechDuration, 15.0, "collected speech time");
    expectNear(metrics.speechRate, cfg.k1 * 5.0 * 60.0 / 15.0, "collected speech rate");
    expectNear(metrics.speechRate, (left.speechRate * 10.0 + right.speechRate * 5.0) / 15.0,
        "speech rate weights longer phrases more");
    expectNear(metrics.vowelMax, 9.0 * 120.0 / 8000.0, "longest vowel across phrases");
    expect(metrics.phrasePauses != (left.phrasePauses + right.phrasePauses) / 2.0,
        "pauses are recomputed on the joined gaps");
}

void testSpeechDurationUsesSampleCount()
{
    std::vector<float> samples(80000, 0);
    samples[1000] = 1000;
    samples[2000] = 5000;
    samples[4000] = 1000;
    const speechrate::Metrics metrics = speechrate::analyze(samples, {});
    if (metrics.valid)
        expectNear(metrics.speechDuration, 10.0, "T_s is sample_count / 8000");
    else
        expect(true, "quiet impulse may be flat after normalization");
}

std::vector<float> noise(int count, double amplitude, unsigned seed)
{
    std::mt19937 rng(seed);
    std::normal_distribution<double> dist(0, amplitude);
    std::vector<float> samples(static_cast<std::size_t>(count));
    for (float& sample : samples)
        sample = static_cast<float>(dist(rng));
    return samples;
}

void addSyllables(std::vector<float>& samples, double perSecond, double amplitude)
{
    const int period = static_cast<int>(8000 / perSecond);
    const int length = 960;
    const int count = static_cast<int>(samples.size());
    for (int start = 0; start + length < count; start += period) {
        for (int index = 0; index < length; ++index) {
            const double envelope = 0.5 - 0.5 * std::cos(2 * M_PI * index / length);
            samples[start + index] += static_cast<float>(
                amplitude * envelope * std::sin(2 * M_PI * 150 * index / 8000.0));
        }
    }
}

void testSilenceIsNotSpeech()
{
    expect(!speechrate::analyze(noise(80000, 30, 1), {}).valid, "quiet background noise is not speech");
    expect(!speechrate::analyze(noise(80000, 200, 2), {}).valid, "loud background noise is not speech");
}

void testSyllableRateSurvivesGate()
{
    for (double rate : { 2.0, 4.0, 6.0 }) {
        std::vector<float> samples = noise(80000, 200, 3);
        addSyllables(samples, rate, 1500);
        const speechrate::Metrics metrics = speechrate::analyze(samples, {});
        expect(metrics.valid, "syllable train is speech");
        expect(metrics.vowelCount == static_cast<int>(rate * 10), "every syllable is one nucleus");
        expectNear(speechrate::fillerPercent(metrics.fillerScore, 120, 240), 0.0, "even syllables are not fillers");
    }
}

void testDurationStatistics()
{
    const speechrate::DurationStats empty = speechrate::durationStatistics({});
    expect(empty.count == 0, "empty duration count");
    expectNear(empty.mean, 0, "empty mean");
    expectNear(empty.stddev, 0, "empty stddev");
    expect(empty.histogram.empty(), "empty histogram");

    const speechrate::DurationStats one = speechrate::durationStatistics({ 0.08 });
    expect(one.count == 1, "single count");
    expectNear(one.mean, 0.08, "single mean");
    expectNear(one.median, 0.08, "single median");
    expectNear(one.min, 0.08, "single min");
    expectNear(one.max, 0.08, "single max");
    expectNear(one.stddev, 0, "single stddev is zero");

    const speechrate::DurationStats stats = speechrate::durationStatistics({ 0.10, 0.30, 0.20, 0.40 });
    expect(stats.count == 4, "duration count");
    expectNear(stats.mean, 0.25, "arithmetic mean");
    expectNear(stats.median, 0.25, "even median averages the two central values");
    expectNear(stats.p25, 0.175, "25th percentile interpolates");
    expectNear(stats.p75, 0.325, "75th percentile interpolates");
    expectNear(stats.min, 0.10, "min duration");
    expectNear(stats.max, 0.40, "max duration");
    expectNear(stats.stddev, std::sqrt(0.05 / 3.0), "sample standard deviation");

    const speechrate::DurationStats odd = speechrate::durationStatistics({ 0.10, 0.40, 0.20 });
    expectNear(odd.median, 0.20, "odd median is the middle value");
    expectNear(odd.mean, 0.70 / 3.0, "odd mean");

    const speechrate::DurationStats bins = speechrate::durationStatistics({ 0.0, 0.019, 0.020, 0.039 });
    expect(bins.histogram.size() == 2, "two 20 ms bins cover the longest vowel");
    expect(bins.histogram[0].count == 2, "first bin holds 0 and 19 ms");
    expect(bins.histogram[1].count == 2, "second bin holds 20 and 39 ms");
    expect(!bins.histogram[1].openEnded, "short vowels do not overflow the histogram");
    expectNear(bins.histogramBinSec, speechrate::kVowelHistogramBinSec, "histogram bin width");

    std::vector<double> overflow(1, speechrate::kVowelHistogramBinSec * (speechrate::kVowelHistogramMaxBins + 2));
    const speechrate::DurationStats longVowel = speechrate::durationStatistics(overflow);
    expect(static_cast<int>(longVowel.histogram.size()) == speechrate::kVowelHistogramMaxBins,
        "histogram stops at the bin cap");
    expect(longVowel.histogram.back().openEnded, "the last bin is open-ended");
    expect(longVowel.histogram.back().count == 1, "the long vowel lands in the last bin");
}

void testPhrasalPauseCounting()
{
    expect(speechrate::kDefaultPhrasalPauseMs == 200, "default phrasal pause is 200 ms");

    auto run = [](int frames, std::uint8_t silent) {
        return std::vector<std::uint8_t>(static_cast<std::size_t>(frames), silent);
    };
    auto join = [](std::vector<std::uint8_t> base, const std::vector<std::uint8_t>& extra) {
        base.insert(base.end(), extra.begin(), extra.end());
        return base;
    };

    // 120-sample hop at 8 kHz is 15 ms. 9 frames = 135 ms, 10 frames = 150 ms.
    std::vector<std::uint8_t> mask = run(5, 1);
    mask = join(mask, run(3, 0));
    mask = join(mask, run(9, 1));
    mask = join(mask, run(3, 0));
    mask = join(mask, run(10, 1));
    mask = join(mask, run(3, 0));
    mask = join(mask, run(20, 1));
    expect(speechrate::countPhrasalPauses(mask, 120, 150) == 1,
        "only the interior 150 ms run counts; 135 ms and the edges do not");
    const std::vector<double> onePause = speechrate::phrasalPauseDurations(mask, 120, 150);
    expect(onePause.size() == 1, "duration list matches the count");
    expectNear(onePause.front(), 10.0 * 120.0 / 8000.0, "150 ms run is 10 hops");

    mask = join(run(4, 0), run(10, 1));
    mask = join(mask, run(2, 0));
    mask = join(mask, run(12, 1));
    mask = join(mask, run(2, 0));
    expect(speechrate::countPhrasalPauses(mask, 120, 150) == 2, "two interior pauses");
    expect(speechrate::countPhrasalPauses(mask, 120, 151) == 1, "151 ms drops the 150 ms run");
    // 14 hops is 210 ms, the first multiple of the 15 ms hop at or above the 200 ms default.
    std::vector<std::uint8_t> atDefault = join(run(4, 0), run(10, 1));
    atDefault = join(atDefault, run(2, 0));
    atDefault = join(atDefault, run(14, 1));
    atDefault = join(atDefault, run(2, 0));
    expect(speechrate::countPhrasalPauses(atDefault, 120, speechrate::kDefaultPhrasalPauseMs) == 1,
        "the 200 ms default keeps the 210 ms run and drops the 150 ms run");
    expect(speechrate::countPhrasalPauses(mask, 120, 10000) == 0, "a huge threshold counts nothing");

    expect(speechrate::countPhrasalPauses(run(40, 1), 120, 150) == 0, "silence with no speech is not a pause");
    expect(speechrate::countPhrasalPauses({}, 120, 150) == 0, "empty mask");
    expect(speechrate::countPhrasalPauses(run(4, 0), 0, 150) == 0, "a zero hop is rejected");
}

void addTone(std::vector<float>& samples, int start, int length, double frequency, double amplitude)
{
    const int count = static_cast<int>(samples.size());
    for (int index = 0; index < length && start + index < count; ++index) {
        const double envelope = 0.5 - 0.5 * std::cos(2 * M_PI * index / length);
        samples[static_cast<std::size_t>(start + index)] += static_cast<float>(
            amplitude * envelope * std::sin(2 * M_PI * frequency * index / 8000.0));
    }
}

void testWholeRecordingSummary()
{
    constexpr int kRate = 8000;
    std::vector<float> samples(static_cast<std::size_t>(kRate * 2), 0);
    const int vowel = 960;
    int cursor = static_cast<int>(0.25 * kRate);
    addTone(samples, cursor, vowel, 150, 6000);
    cursor += vowel + static_cast<int>(0.40 * kRate);
    addTone(samples, cursor, vowel, 150, 6000);
    cursor += vowel + static_cast<int>(0.05 * kRate);
    addTone(samples, cursor, vowel, 180, 6000);

    const speechrate::RecordingSummary summary = speechrate::summarizeRecording(samples, {});
    expect(summary.valid, "framed recording is valid");
    expect(summary.vowelCount == 3, "three voiced bursts are three nuclei");
    expect(summary.phrasalPauseCount == 1, "only the 400 ms gap is a phrasal pause");
    expect(summary.pauseThresholdMs == speechrate::kDefaultPhrasalPauseMs, "default threshold is reported");
    expect(summary.vowelDurations.count == summary.vowelCount, "duration count matches nuclei");
    expect(summary.vowelDurations.min <= summary.vowelDurations.median
            && summary.vowelDurations.median <= summary.vowelDurations.max,
        "duration order");
    expect(summary.vowelDurations.min <= summary.vowelDurations.mean
            && summary.vowelDurations.mean <= summary.vowelDurations.max,
        "mean stays inside the range");
    expect(summary.vowelDurations.min > 0.02 && summary.vowelDurations.max < 0.30,
        "detected vowels are a few tens of milliseconds");
    expect(summary.vowelDurations.stddev >= 0, "stddev is non-negative");
    int histogramTotal = 0;
    for (const speechrate::HistogramBin& bin : summary.vowelDurations.histogram)
        histogramTotal += bin.count;
    expect(histogramTotal == summary.vowelCount, "histogram counts every vowel");

    const speechrate::RecordingSummary strict = speechrate::summarizeRecording(samples, {}, 2000);
    expect(strict.vowelCount == 3, "raising the pause threshold keeps the vowels");
    expect(strict.phrasalPauseCount == 0, "400 ms is below a 2 s pause threshold");

    std::vector<float> bridged(static_cast<std::size_t>(kRate), 0);
    addTone(bridged, 800, vowel, 150, 6000);
    const int noiseAt = 800 + vowel + 400;
    std::mt19937 rng(7);
    std::normal_distribution<double> dist(0, 4000);
    for (int index = 0; index < 1600 && noiseAt + index < static_cast<int>(bridged.size()); ++index)
        bridged[static_cast<std::size_t>(noiseAt + index)] = static_cast<float>(dist(rng));
    addTone(bridged, noiseAt + 1600 + 400, vowel, 150, 6000);
    const speechrate::RecordingSummary loudGap = speechrate::summarizeRecording(bridged, {});
    expect(loudGap.vowelCount == 2, "loud unvoiced noise is not a vowel nucleus");
    expect(loudGap.phrasalPauseCount == 0, "a loud unvoiced gap is not a phrasal pause");

    expect(!speechrate::summarizeRecording({}, {}).valid, "empty summary is not valid");
    const speechrate::RecordingSummary quiet = speechrate::summarizeRecording(std::vector<float>(kRate, 0), {});
    expect(quiet.valid, "silence can still be framed");
    expect(quiet.vowelCount == 0 && quiet.phrasalPauseCount == 0, "silence has no vowels and no interior pause");
}

void addSteady(std::vector<float>& samples, int start, int length, double frequency, double amplitude)
{
    const int count = static_cast<int>(samples.size());
    for (int index = 0; index < length && start + index < count; ++index) {
        samples[static_cast<std::size_t>(start + index)] += static_cast<float>(
            amplitude * std::sin(2 * M_PI * frequency * index / 8000.0));
    }
}

void testPauseTotalsJoin()
{
    speechrate::Parts left;
    left.pauseCount = 1;
    left.pauseTotalSec = 0.4;
    speechrate::Parts right;
    right.pauseCount = 2;
    right.pauseTotalSec = 0.5;
    left.speechSpanSec = 1.5;
    right.speechSpanSec = 2.5;
    speechrate::appendParts(left, right);
    expect(left.pauseCount == 3, "joined pause counts add");
    expectNear(left.pauseTotalSec, 0.9, "joined pause time adds");
    expectNear(left.speechSpanSec, 4.0, "joined speech spans add");
}

void testSpeechSpan()
{
    const std::vector<std::uint8_t> silent { 1, 1, 0, 0, 1, 0, 1 };
    expectNear(speechrate::speechSpanSec(silent, 8000), 4.0, "span runs from the first speech frame through the last");
    expectNear(speechrate::speechSpanSec({}, 120), 0.0, "an empty mask has no span");
    expectNear(speechrate::speechSpanSec({ 1, 1 }, 120), 0.0, "silence has no span");
    expectNear(speechrate::speechSpanSec({ 0 }, 8000), 1.0, "one speech frame lasts one hop");
}

void testDirectSpeechRatePublishing()
{
    speechrate::Config cfg;
    speechrate::Metrics metrics = speechrate::metricsFromLengths({ 4, 10 }, { 40 }, 10.0, cfg);
    speechrate::applyPauseStats(metrics, 1, 2.0, 8.0, cfg);
    expectNear(metrics.legacySpeechRate, 0.71 * 2.0 * 60.0 / 10.0, "legacy speech rate uses the whole buffer");
    expectNear(metrics.directSpeechRate, 0.71 * 2.0 * 60.0 / 8.0, "direct speech rate uses the span");
    expectNear(metrics.speechRate, metrics.directSpeechRate, "the default speech method publishes the direct rate");
    expectNear(metrics.speechSpanSec, 8.0, "the span is kept");
    expectNear(metrics.directArticulationRate, 0.71 * 2.0 * 60.0 / 6.0,
        "direct articulation removes pauses from the span");
    expect(metrics.directArticulationRate > metrics.directSpeechRate,
        "pauses make direct articulation faster than direct speech rate");
    expect(metrics.articulationRate + 1e-9 >= metrics.speechRate,
        "published articulation stays at or above published speech rate");

    speechrate::Config legacy = cfg;
    legacy.speechRateMethod = speechrate::SpeechMethod::Legacy;
    legacy.articulationMethod = speechrate::ArticulationMethod::Legacy;
    speechrate::Metrics legacyMetrics = speechrate::metricsFromLengths({ 4, 10 }, { 40 }, 10.0, legacy);
    const double legacyArticulation = legacyMetrics.articulationRate;
    speechrate::applyPauseStats(legacyMetrics, 1, 2.0, 8.0, legacy);
    expectNear(legacyMetrics.speechRate, legacyMetrics.legacySpeechRate,
        "legacy method publishes the whole-buffer rate");
    expectNear(legacyMetrics.articulationRate, legacyArticulation,
        "legacy method publishes the length formula");
    expectNear(legacyMetrics.directSpeechRate, metrics.directSpeechRate,
        "the direct speech rate is still computed in legacy mode");

    speechrate::Metrics missing = speechrate::metricsFromLengths({ 4, 10 }, { 40 }, 10.0, cfg);
    speechrate::applyPauseStats(missing, 0, 0.0, 0.0, cfg);
    expectNear(missing.directSpeechRate, missing.legacySpeechRate,
        "a missing span uses the whole buffer");
    expectNear(missing.directArticulationRate, missing.directSpeechRate,
        "no pauses keeps the two direct rates equal");

    expectNear(speechrate::directSpeechRate(10, 8.0, 0.71), 0.71 * 60.0 * 10.0 / 8.0, "direct formula");
    expectNear(speechrate::directSpeechRate(10, 0.0, 0.71), 0.0, "a non-positive span is a zero rate");
}

void testDirectArticulationFormula()
{
    expectNear(speechrate::directArticulationRate(10, 10.0, 0.0, 0.71),
        0.71 * 60.0 * 10.0 / 10.0, "no pauses leaves articulation equal to speech rate");
    expectNear(speechrate::directArticulationRate(10, 10.0, 2.5, 0.71),
        0.71 * 60.0 * 10.0 / 7.5, "speaking time excludes the pause sum");
    expectNear(speechrate::directArticulationRate(10, 10.0, 10.0, 0.71),
        0.71 * 60.0 * 10.0 / 10.0, "a fully paused buffer falls back to speech rate");
    expectNear(speechrate::directArticulationRate(0, 10.0, 2.0, 0.71), 0.0, "no vowels is a zero rate");
    expect(speechrate::directArticulationRate(4, 8.0, 1.0, 0.71)
            > speechrate::directArticulationRate(4, 8.0, 0.0, 0.71),
        "removing pauses raises articulation");
}

void testSilenceThresholdAdapts()
{
    speechrate::Config cfg;
    const std::vector<double> flat(50, 10.0);
    expectNear(speechrate::silenceThreshold(flat, cfg), speechrate::speechGateLevel(flat, cfg),
        "a flat contour keeps the noise-floor gate");

    std::vector<double> quiet(200, 5.0);
    for (int index = 160; index < 200; ++index)
        quiet[static_cast<std::size_t>(index)] = 200.0;
    std::vector<double> loud = quiet;
    for (double& value : loud)
        value *= 8.0;
    const double quietLevel = speechrate::silenceThreshold(quiet, cfg);
    const double loudLevel = speechrate::silenceThreshold(loud, cfg);
    expect(loudLevel > quietLevel * 2.0, "a louder take raises the silence level");
    expect(quietLevel < 200.0 * speechrate::kSilenceMaxFraction, "the level stays under the speech peak");
}

std::vector<float> threeBursts()
{
    constexpr int kRate = 8000;
    std::vector<float> samples(static_cast<std::size_t>(kRate * 2), 0);
    const int vowel = 960;
    int cursor = static_cast<int>(0.20 * kRate);
    addTone(samples, cursor, vowel, 150, 6000);
    cursor += vowel + static_cast<int>(0.08 * kRate);
    addTone(samples, cursor, vowel, 150, 6000);
    cursor += vowel + static_cast<int>(0.40 * kRate);
    addTone(samples, cursor, vowel, 160, 6000);
    return samples;
}

void testDirectPausesFromSignal()
{
    const std::vector<float> samples = threeBursts();
    speechrate::Config direct;
    const speechrate::Measurement measured = speechrate::measure(samples, direct);
    expect(measured.metrics.valid, "three bursts are measurable speech");
    expect(measured.metrics.pauseCount == 1, "the 80 ms closure is not a pause; the 400 ms gap is");
    expect(measured.metrics.pauseTotal > 0.25 && measured.metrics.pauseTotal < 0.55,
        "the counted pause is about 400 ms");
    expectNear(measured.metrics.pauseMean, measured.metrics.pauseTotal, "one pause, mean equals the total");
    expect(measured.metrics.speechSpanSec > 0.0
            && measured.metrics.speechSpanSec < measured.metrics.speechDuration,
        "the span drops leading and trailing silence");
    expectNear(measured.metrics.directSpeechRate,
        direct.k1 * measured.metrics.vowelCount * 60.0 / measured.metrics.speechSpanSec,
        "direct speech rate uses K1 and the span");
    expectNear(measured.metrics.speechRate, measured.metrics.directSpeechRate,
        "the default speech method publishes the direct rate");
    expectNear(measured.metrics.directArticulationRate,
        direct.k1 * measured.metrics.vowelCount * 60.0
            / (measured.metrics.speechSpanSec - measured.metrics.pauseTotal),
        "direct articulation uses K1 and the span minus the pause sum");
    expect(measured.metrics.directArticulationRate > measured.metrics.directSpeechRate,
        "articulation is above speech rate when a pause was removed");
    expectNear(measured.metrics.articulationRate, measured.metrics.directArticulationRate,
        "the default articulation method publishes the direct rate");
    expectNear(measured.metrics.phrasePauses, measured.metrics.pauseMean,
        "the default pause tile is the direct mean");

    const speechrate::Metrics legacyOnly = speechrate::metricsFromLengths(
        measured.parts.vowelLengths,
        measured.parts.gapLengths,
        measured.parts.speechDuration,
        direct,
        measured.parts.vowelMaxFrames,
        measured.parts.gapMaxFrames);
    expectNear(measured.metrics.legacySpeechRate, legacyOnly.speechRate,
        "legacy speech rate is the whole buffer");
    expect(measured.metrics.directSpeechRate > measured.metrics.legacySpeechRate,
        "leading silence makes the direct rate higher");
    expectNear(measured.metrics.legacyArticulationRate, legacyOnly.articulationRate,
        "legacy articulation is the length formula");
    expectNear(measured.metrics.legacyPauseScore, legacyOnly.phrasePauses,
        "legacy pause score is the indirect gap score");

    speechrate::Config legacy = direct;
    legacy.speechRateMethod = speechrate::SpeechMethod::Legacy;
    legacy.articulationMethod = speechrate::ArticulationMethod::Legacy;
    const speechrate::Measurement legacyMeasured = speechrate::measure(samples, legacy);
    expectNear(legacyMeasured.metrics.speechRate, legacyMeasured.metrics.legacySpeechRate,
        "legacy method publishes the old speech rate");
    expectNear(legacyMeasured.metrics.articulationRate, legacyMeasured.metrics.legacyArticulationRate,
        "legacy method publishes the old articulation rate");
    expectNear(legacyMeasured.metrics.phrasePauses, legacyMeasured.metrics.legacyPauseScore,
        "legacy method publishes the old pause score");
    expectNear(legacyMeasured.metrics.directSpeechRate, measured.metrics.directSpeechRate,
        "the direct speech rate is still computed in legacy mode");
    expectNear(legacyMeasured.metrics.directArticulationRate, measured.metrics.directArticulationRate,
        "the direct articulation rate is still computed in legacy mode");
    expect(std::fabs(legacyMeasured.metrics.articulationRate - measured.metrics.articulationRate) > 1.0,
        "the two methods disagree once a real pause is present");

    const speechrate::RecordingSummary summary = speechrate::summarizeRecording(samples, direct);
    expect(summary.phrasalPauseCount == measured.metrics.pauseCount, "summary pause count matches the tempo path");
    expectNear(summary.pauseTotalSec, measured.metrics.pauseTotal, "summary pause sum matches");
    expectNear(summary.directPauseMeanSec, summary.pauseDurations.mean, "shown mean is the distribution mean");
    expectNear(summary.pauseRatio, summary.pauseTotalSec / summary.speechDurationSec, "pause ratio is sum over duration");
    expect(summary.paceValid, "the compare block has a pace");
    expectNear(summary.directSpeechRate, measured.metrics.directSpeechRate, "summary direct speech rate");
    expectNear(summary.legacySpeechRate, measured.metrics.legacySpeechRate, "summary legacy speech rate");
    expectNear(summary.speechRate, measured.metrics.speechRate, "summary publishes the selected speech rate");
    expectNear(summary.directArticulationRate, measured.metrics.directArticulationRate, "summary direct rate");
    expectNear(summary.legacyArticulationRate, measured.metrics.legacyArticulationRate, "summary legacy rate");
    expectNear(summary.legacyPauseScore, measured.metrics.legacyPauseScore, "summary legacy pause score");
    expect(summary.pauseDurations.p25 <= summary.pauseDurations.median
            && summary.pauseDurations.median <= summary.pauseDurations.p75,
        "pause percentiles are ordered");
    int histogramTotal = 0;
    for (const speechrate::HistogramBin& bin : summary.pauseDurations.histogram)
        histogramTotal += bin.count;
    expect(histogramTotal == summary.phrasalPauseCount, "pause histogram counts every pause");
    expectNear(summary.pauseDurations.histogramBinSec, speechrate::kPauseHistogramBinSec, "pause bars are 50 ms");

    std::vector<float> quieter = samples;
    std::vector<float> louder = samples;
    for (float& sample : quieter)
        sample = static_cast<float>(sample * 0.3);
    for (float& sample : louder)
        sample = static_cast<float>(sample * 3.0);
    const speechrate::RecordingSummary quietSummary = speechrate::summarizeRecording(quieter, direct);
    const speechrate::RecordingSummary loudSummary = speechrate::summarizeRecording(louder, direct);
    expect(quietSummary.phrasalPauseCount == 1 && loudSummary.phrasalPauseCount == 1,
        "pause count does not follow the recording gain");
    expect(std::fabs(quietSummary.pauseTotalSec - summary.pauseTotalSec) <= 0.02
            && std::fabs(loudSummary.pauseTotalSec - summary.pauseTotalSec) <= 0.02,
        "pause time stays within one hop when the gain changes");

    std::vector<float> voicedGap(static_cast<std::size_t>(8000 * 2), 0);
    addTone(voicedGap, 1600, 960, 150, 6000);
    addSteady(voicedGap, 1600 + 960, static_cast<int>(0.40 * 8000), 150, 180);
    addTone(voicedGap, 1600 + 960 + static_cast<int>(0.40 * 8000), 960, 150, 6000);
    const speechrate::RecordingSummary voiced = speechrate::summarizeRecording(voicedGap, direct);
    expect(voiced.phrasalPauseCount == 0, "a quiet voiced stretch is not a pause");
}

speechrate::WeightedSample sample(double value, double weight)
{
    speechrate::WeightedSample item;
    item.value = value;
    item.weight = weight;
    return item;
}

void testSessionSummary()
{
    expect(speechrate::summaryMethodOf(-1) == speechrate::SummaryMethod::Median,
        "an unknown summary falls back to the median");
    expect(speechrate::summaryMethodOf(99) == speechrate::SummaryMethod::Median,
        "a summary past the last choice falls back to the median");
    expect(speechrate::summaryMethodOf(2) == speechrate::SummaryMethod::Median, "median id is kept");

    const std::vector<speechrate::WeightedSample> empty;
    expectNear(speechrate::summarizeSamples(empty, speechrate::SummaryMethod::Mean), 0, "empty mean");
    expectNear(speechrate::summarizeSamples(empty, speechrate::SummaryMethod::Median), 0, "empty median");

    const std::vector<speechrate::WeightedSample> one { sample(4, 2) };
    expectNear(speechrate::summarizeSamples(one, speechrate::SummaryMethod::WeightedMean), 4, "one weighted mean");
    expectNear(speechrate::summarizeSamples(one, speechrate::SummaryMethod::QuadraticMean), 4, "one quadratic mean");

    const std::vector<speechrate::WeightedSample> phrases {
        sample(10, 1), sample(20, 1), sample(30, 1), sample(100, 1)
    };
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::WeightedMean), 40, "equal weights are the mean");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::Mean), 40, "arithmetic mean");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::Median), 25, "even median averages the center");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::WeightedMedian), 25,
        "equal weights make the weighted median the median");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::HarmonicMean),
        4.0 / (0.1 + 0.05 + 1.0 / 30.0 + 0.01), "harmonic mean");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::GeometricMean),
        std::pow(10.0 * 20.0 * 30.0 * 100.0, 0.25), "geometric mean");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::TrimmedMean), 25,
        "four phrases drop one from each end");
    expectNear(speechrate::summarizeSamples(phrases, speechrate::SummaryMethod::QuadraticMean),
        std::sqrt((100.0 + 400.0 + 900.0 + 10000.0) / 4.0), "quadratic mean");

    const std::vector<speechrate::WeightedSample> weighted { sample(100, 2), sample(180, 4) };
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::WeightedMean),
        (100.0 * 2.0 + 180.0 * 4.0) / 6.0, "longer phrase pulls the weighted mean");
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::Mean), 140, "unweighted mean");
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::Median), 140, "two-phrase median");
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::WeightedMedian), 180,
        "the longer phrase is the weighted median");
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::HarmonicMean),
        2.0 / (1.0 / 100.0 + 1.0 / 180.0), "two-phrase harmonic mean");
    expectNear(speechrate::summarizeSamples(weighted, speechrate::SummaryMethod::TrimmedMean), 140,
        "fewer than four phrases keeps the mean");

    const std::vector<speechrate::WeightedSample> withZero { sample(10, 1), sample(0, 1), sample(30, 1) };
    expectNear(speechrate::summarizeSamples(withZero, speechrate::SummaryMethod::HarmonicMean), 0,
        "a zero phrase makes the harmonic mean zero");
    expectNear(speechrate::summarizeSamples(withZero, speechrate::SummaryMethod::GeometricMean), 0,
        "a zero phrase makes the geometric mean zero");
    expectNear(speechrate::summarizeSamples(withZero, speechrate::SummaryMethod::Mean), 40.0 / 3.0,
        "the arithmetic mean keeps a zero");
    expectNear(speechrate::summarizeSamples(withZero, speechrate::SummaryMethod::Median), 10, "zero stays in the median");

    const std::vector<speechrate::WeightedSample> five {
        sample(1, 1), sample(2, 9), sample(3, 1), sample(4, 1), sample(100, 1)
    };
    expectNear(speechrate::summarizeSamples(five, speechrate::SummaryMethod::TrimmedMean), 3,
        "trimmed mean drops the outer quarter");
    expectNear(speechrate::summarizeSamples(five, speechrate::SummaryMethod::WeightedMedian), 2,
        "a heavy middle phrase is the weighted median");

    const std::vector<speechrate::WeightedSample> dirty {
        sample(10, 0), sample(std::numeric_limits<double>::quiet_NaN(), 4), sample(30, -2)
    };
    expectNear(speechrate::summarizeSamples(dirty, speechrate::SummaryMethod::WeightedMean), 20,
        "non-finite values are skipped and a bad weight counts as one");
}

} // namespace

int main()
{
    testWorkedExample();
    testIntegerMedian();
    testMinLengthFrames();
    testSmoothEdgeRule();
    testShortNucleusDropped();
    testGapsIgnoreTails();
    testEmptyAndFlat();
    testCollectedPhrases();
    testSpeechDurationUsesSampleCount();
    testSilenceIsNotSpeech();
    testSyllableRateSurvivesGate();
    testDurationStatistics();
    testPhrasalPauseCounting();
    testWholeRecordingSummary();
    testPauseTotalsJoin();
    testSpeechSpan();
    testDirectSpeechRatePublishing();
    testDirectArticulationFormula();
    testSilenceThresholdAdapts();
    testDirectPausesFromSignal();
    testSessionSummary();

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "speech-rate analysis tests passed\n";
    return EXIT_SUCCESS;
}
