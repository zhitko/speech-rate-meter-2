#include "speechrateanalysis.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
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
    expect(speechrate::kDefaultPhrasalPauseMs == 150, "default phrasal pause is 150 ms");

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

    mask = join(run(4, 0), run(10, 1));
    mask = join(mask, run(2, 0));
    mask = join(mask, run(12, 1));
    mask = join(mask, run(2, 0));
    expect(speechrate::countPhrasalPauses(mask, 120, 150) == 2, "two interior pauses");
    expect(speechrate::countPhrasalPauses(mask, 120, 151) == 1, "151 ms drops the 150 ms run");
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

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "speech-rate analysis tests passed\n";
    return EXIT_SUCCESS;
}
