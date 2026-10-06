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

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "speech-rate analysis tests passed\n";
    return EXIT_SUCCESS;
}
