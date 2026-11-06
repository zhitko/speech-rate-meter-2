#include "helpers/settings.h"
#include "livecadence.h"
#include "speechrateanalysis.h"
#include "wavloader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// Diagnostic only. Checks whether the current capture buffer, intensity
// contour, normalization, and WAV round-trip invent a falling envelope.
// A constant-amplitude input must stay constant. A programmed decay must
// stay that decay, not be added on top.

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void report(const std::string& line)
{
    std::cout << line << '\n';
}

double rmsOf(const std::vector<float>& samples, std::size_t begin, std::size_t end)
{
    if (end <= begin || end > samples.size())
        return -1;
    double sum = 0;
    for (std::size_t index = begin; index < end; ++index) {
        const double value = samples[index];
        sum += value * value;
    }
    return std::sqrt(sum / static_cast<double>(end - begin));
}

double peakOf(const std::vector<float>& samples, std::size_t begin, std::size_t end)
{
    double peak = 0;
    for (std::size_t index = begin; index < end && index < samples.size(); ++index)
        peak = std::max(peak, std::fabs(static_cast<double>(samples[index])));
    return peak;
}

std::vector<qint16> sine(int count, double amplitude, double hz, int sampleRate)
{
    std::vector<qint16> samples(static_cast<std::size_t>(count));
    const double step = 2.0 * std::acos(-1.0) * hz / static_cast<double>(sampleRate);
    for (int index = 0; index < count; ++index) {
        const double value = amplitude * std::sin(step * index);
        samples[static_cast<std::size_t>(index)] = static_cast<qint16>(std::lround(value));
    }
    return samples;
}

std::vector<float> asFloat(const std::vector<qint16>& samples)
{
    std::vector<float> out(samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index)
        out[index] = static_cast<float>(samples[index]);
    return out;
}

struct Kept {
    std::vector<float> samples;
};

Kept recordContinuous(const std::vector<qint16>& input, int windowSec)
{
    LiveCapture take;
    take.start(60, windowSec);
    Kept kept;
    std::vector<float> lastReading;
    std::size_t lastFed = 0;
    const int chunk = LiveCapture::kSampleRate / 10;
    qint64 now = 0;
    for (std::size_t offset = 0; offset < input.size();) {
        const int count = static_cast<int>(std::min<std::size_t>(chunk, input.size() - offset));
        take.append(input.data() + offset, count);
        const LiveCapture::Update update = take.advanceContinuous(now);
        for (const LiveCapture::ClosedSegment& closed : update.closed)
            kept.samples.insert(kept.samples.end(), closed.samples.begin(), closed.samples.end());
        if (update.hasReading) {
            lastReading = update.reading.samples;
            lastFed = offset + static_cast<std::size_t>(count);
        }
        take.trim();
        offset += static_cast<std::size_t>(count);
        now += 100;
    }
    if (!lastReading.empty() && lastReading.size() <= lastFed && lastFed <= input.size()) {
        const std::size_t start = lastFed - lastReading.size();
        bool same = true;
        for (std::size_t index = 0; index < lastReading.size(); ++index) {
            if (lastReading[index] != static_cast<float>(input[start + index])) {
                same = false;
                break;
            }
        }
        report("live reading samples=" + std::to_string(lastReading.size())
            + " fed=" + std::to_string(lastFed)
            + " unscaled=" + std::to_string(same ? 1 : 0));
        expect(same, "the analysis-window reading is the last unscaled samples");
    }
    const LiveCapture::Update stopped = take.closeForStop();
    for (const LiveCapture::ClosedSegment& closed : stopped.closed)
        kept.samples.insert(kept.samples.end(), closed.samples.begin(), closed.samples.end());
    return kept;
}

void testConstantToneIsNotFaded()
{
    const int rate = LiveCapture::kSampleRate;
    const int seconds = 20;
    const double amplitude = 8000;
    const std::vector<qint16> input = sine(seconds * rate, amplitude, 200.0, rate);
    const Kept kept = recordContinuous(input, 10);

    report("constant 200 Hz, amplitude 8000, 20 s");
    report("stored samples=" + std::to_string(kept.samples.size())
        + " input=" + std::to_string(input.size()));
    expect(kept.samples.size() == input.size(), "a 20 s take is kept in full across the 15 s cut");

    bool identical = kept.samples.size() == input.size();
    double minRms = 1e300;
    double maxRms = 0;
    for (int second = 0; second < seconds; ++second) {
        const std::size_t begin = static_cast<std::size_t>(second) * static_cast<std::size_t>(rate);
        const double rms = rmsOf(kept.samples, begin, begin + static_cast<std::size_t>(rate));
        minRms = std::min(minRms, rms);
        maxRms = std::max(maxRms, rms);
        if (second == 0 || second == 9 || second == 14 || second == 15 || second == 19)
            report("  second " + std::to_string(second) + " rms=" + std::to_string(rms));
        if (identical) {
            for (std::size_t index = begin; index < begin + static_cast<std::size_t>(rate); ++index) {
                if (kept.samples[index] != static_cast<float>(input[index])) {
                    identical = false;
                    break;
                }
            }
        }
    }
    report("  rms min=" + std::to_string(minRms) + " max=" + std::to_string(maxRms)
        + " ratio=" + std::to_string(maxRms / minRms));
    expect(identical, "stored PCM matches the constant input sample for sample");
    expect(maxRms / minRms < 1.01, "per-second RMS of a constant tone stays flat");

    const std::vector<double> contour = speechrate::intensity(asFloat(input), 64, 32);
    double interiorMin = 1e300;
    double interiorMax = 0;
    double earlySum = 0;
    double lateSum = 0;
    const std::size_t interior = contour.size() > 2 ? contour.size() - 2 : 0;
    const std::size_t half = interior / 2;
    for (std::size_t index = 2; index < contour.size(); ++index) {
        interiorMin = std::min(interiorMin, contour[index]);
        interiorMax = std::max(interiorMax, contour[index]);
        if (index < 2 + half)
            earlySum += contour[index];
        else
            lateSum += contour[index];
    }
    const double earlyMean = half > 0 ? earlySum / static_cast<double>(half) : 0;
    const double lateCount = static_cast<double>(contour.size() - 2 - half);
    const double lateMean = lateCount > 0 ? lateSum / lateCount : 0;
    report("intensity points=" + std::to_string(contour.size())
        + " first=" + std::to_string(contour.front())
        + " interiorMin=" + std::to_string(interiorMin)
        + " interiorMax=" + std::to_string(interiorMax)
        + " earlyMean=" + std::to_string(earlyMean)
        + " lateMean=" + std::to_string(lateMean)
        + " last=" + std::to_string(contour.back()));
    expect(contour.front() < interiorMin * 0.7, "the first intensity frame is the known half window");
    // A 64-sample window is not an integer number of 200 Hz periods, so
    // successive frames ripple. That ripple must not be a drift toward 0.
    expect(interiorMin > interiorMax * 0.9, "intensity ripple of a constant tone stays within 10 percent");
    expect(lateMean > earlyMean * 0.99 && lateMean < earlyMean * 1.01,
        "mean intensity of a constant tone does not drift");
    const std::vector<double> normalized = speechrate::normalize(contour);
    expect(!normalized.empty(), "a constant tone still normalizes because the first frame is lower");
    if (!normalized.empty()) {
        report("normalized first=" + std::to_string(normalized.front())
            + " last=" + std::to_string(normalized.back()));
        expect(normalized.front() == 0.0, "normalization pins the short first frame to 0");
        expect(normalized.back() > 0.90, "normalization does not fade the end of a constant tone");
    }
    const speechrate::Metrics metrics = speechrate::analyze(asFloat(input), speechrate::Config {});
    report("constant-tone vowelCount=" + std::to_string(metrics.vowelCount)
        + " valid=" + std::to_string(metrics.valid ? 1 : 0));
}

struct Burst {
    int toneSamples = 0;
    int gapSamples = 0;
    std::vector<double> amplitudes;
};

std::vector<qint16> burstsOf(const Burst& burst, int sampleRate)
{
    std::vector<qint16> samples;
    const double step = 2.0 * std::acos(-1.0) * 180.0 / static_cast<double>(sampleRate);
    for (double amplitude : burst.amplitudes) {
        for (int index = 0; index < burst.toneSamples; ++index) {
            const double value = amplitude * std::sin(step * index);
            samples.push_back(static_cast<qint16>(std::lround(value)));
        }
        samples.insert(samples.end(), static_cast<std::size_t>(burst.gapSamples), 0);
    }
    return samples;
}

double contourPeak(const std::vector<double>& contour, int shift, int begin, int end)
{
    double peak = 0;
    const int first = std::max(0, begin / shift);
    const int last = std::min(static_cast<int>(contour.size()) - 1, end / shift);
    for (int index = first; index <= last; ++index)
        peak = std::max(peak, contour[static_cast<std::size_t>(index)]);
    return peak;
}

void testEqualAndDecayingBursts()
{
    const int rate = LiveCapture::kSampleRate;
    Burst shape;
    shape.toneSamples = rate * 200 / 1000;
    shape.gapSamples = rate * 350 / 1000;
    shape.amplitudes.assign(9, 10000.0);
    const std::vector<qint16> equal = burstsOf(shape, rate);
    const Kept kept = recordContinuous(equal, 10);
    expect(kept.samples.size() == equal.size(), "nine equal bursts are stored in full");

    report("nine equal 180 Hz bursts, 200 ms on / 350 ms off, amplitude 10000");
    const int span = shape.toneSamples + shape.gapSamples;
    double minPeak = 1e300;
    double maxPeak = 0;
    const std::vector<double> contour = speechrate::intensity(asFloat(equal), 64, 32);
    const std::vector<double> normalized = speechrate::normalize(contour);
    for (int index = 0; index < 9; ++index) {
        const std::size_t begin = static_cast<std::size_t>(index * span);
        const double peak = peakOf(kept.samples, begin, begin + static_cast<std::size_t>(shape.toneSamples));
        const double level = contourPeak(contour, 32, index * span, index * span + shape.toneSamples);
        const double unit = contourPeak(normalized, 32, index * span, index * span + shape.toneSamples);
        minPeak = std::min(minPeak, peak);
        maxPeak = std::max(maxPeak, peak);
        report("  burst " + std::to_string(index + 1)
            + " pcmPeak=" + std::to_string(peak)
            + " intensity=" + std::to_string(level)
            + " normalized=" + std::to_string(unit));
    }
    expect(maxPeak - minPeak < 1.0, "equal bursts keep the same PCM peak");
    const speechrate::Metrics equalMetrics = speechrate::analyze(asFloat(equal), speechrate::Config {});
    report("equal bursts vowelCount=" + std::to_string(equalMetrics.vowelCount)
        + " valid=" + std::to_string(equalMetrics.valid ? 1 : 0));
    expect(equalMetrics.valid && equalMetrics.vowelCount == 9, "all nine equal bursts stay above the speech gate");

    Burst fading = shape;
    for (int index = 0; index < 9; ++index)
        fading.amplitudes[static_cast<std::size_t>(index)] = 10000.0 * (1.0 - 0.996 * index / 8.0);
    const std::vector<qint16> faded = burstsOf(fading, rate);
    const Kept fadedKept = recordContinuous(faded, 10);
    expect(fadedKept.samples.size() == faded.size(), "a decaying take is stored in full");
    const std::vector<double> fadedContour = speechrate::intensity(asFloat(faded), 64, 32);
    report("nine bursts with programmed amplitude 10000 down to about 40");
    for (int index = 0; index < 9; ++index) {
        const std::size_t begin = static_cast<std::size_t>(index * span);
        const double peak = peakOf(fadedKept.samples, begin, begin + static_cast<std::size_t>(shape.toneSamples));
        const double level = contourPeak(fadedContour, 32, index * span, index * span + shape.toneSamples);
        report("  burst " + std::to_string(index + 1)
            + " pcmPeak=" + std::to_string(peak)
            + " intensity=" + std::to_string(level)
            + " requested=" + std::to_string(fading.amplitudes[static_cast<std::size_t>(index)]));
        expect(std::fabs(peak - fading.amplitudes[static_cast<std::size_t>(index)]) <= 1.0,
            "stored peak follows the programmed decay and does not add another");
    }
    const speechrate::Metrics fadedMetrics = speechrate::analyze(asFloat(faded), speechrate::Config {});
    report("faded bursts vowelCount=" + std::to_string(fadedMetrics.vowelCount)
        + " valid=" + std::to_string(fadedMetrics.valid ? 1 : 0));
    expect(fadedMetrics.valid && fadedMetrics.vowelCount == 8,
        "the burst whose raw intensity is under the speech gate is dropped");
}

void testSpeechSegmentCopiesSamples()
{
    const int rate = LiveCapture::kSampleRate;
    std::vector<qint16> input = sine(3 * rate, 9000.0, 180.0, rate);
    input.insert(input.end(), static_cast<std::size_t>(2 * rate), 0);
    LiveCapture take;
    take.start(60, 10);
    take.setPauseLimitMs(2000);
    const qint64 speechStart = rate / 2;
    const qint64 speechStop = 2 * rate;
    std::vector<float> closed;
    // Same order as capture: append a hop, classify it, then trim.
    for (qint64 at = 0; at + LiveCapture::kHop <= static_cast<qint64>(input.size()); at += LiveCapture::kHop) {
        take.append(input.data() + at, LiveCapture::kHop);
        const bool speech = at + LiveCapture::kHop > speechStart && at < speechStop;
        LiveCapture::Update update;
        if (speech)
            update = take.advanceSpeech(at, at + LiveCapture::kVadFrame, at * 1000 / rate);
        else if (take.phraseSpeaking())
            update = take.advanceSilence(2000);
        for (const LiveCapture::ClosedSegment& segment : update.closed)
            closed.insert(closed.end(), segment.samples.begin(), segment.samples.end());
        take.trim();
    }
    double peak = 0;
    for (float sample : closed)
        peak = std::max(peak, std::fabs(static_cast<double>(sample)));
    // First speech hop is the first 64-sample frame that reaches 0.5 s.
    // The segment is that start minus the 300 ms pad, through the last
    // speech frame plus the 300 ms pad.
    const qint64 firstHop = (speechStart / LiveCapture::kHop) * LiveCapture::kHop;
    const qint64 from = firstHop - LiveCapture::kPadSamples;
    const qint64 lastHop = ((speechStop - 1) / LiveCapture::kHop) * LiveCapture::kHop;
    const qint64 to = lastHop + LiveCapture::kVadFrame + LiveCapture::kPadSamples;
    bool same = static_cast<qint64>(closed.size()) == to - from && from >= 0;
    if (same) {
        for (qint64 index = 0; index < to - from; ++index) {
            if (closed[static_cast<std::size_t>(index)]
                != static_cast<float>(input[static_cast<std::size_t>(from + index)])) {
                same = false;
                break;
            }
        }
    }
    report("speech-segment samples=" + std::to_string(closed.size())
        + " peak=" + std::to_string(peak)
        + " from=" + std::to_string(from)
        + " to=" + std::to_string(to)
        + " unscaled=" + std::to_string(same ? 1 : 0));
    expect(!closed.empty(), "a pause closes the speech segment");
    expect(std::fabs(peak - 9000.0) <= 1.0, "silence segmentation copies the PCM peak unchanged");
    expect(same, "the closed segment is the unscaled speech span plus 300 ms pads");
}

void testWavRoundTrip()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary wav directory");
    const std::vector<qint16> tone = sine(LiveCapture::kSampleRate, 6000.0, 220.0, LiveCapture::kSampleRate);
    const QString path = dir.filePath(QStringLiteral("constant.wav"));
    expect(writePcm16MonoWav(path, asFloat(tone), LiveCapture::kSampleRate), "write 8 kHz constant wav");
    const WavLoadResult loaded = loadAnalysisWav(path);
    expect(loaded.ok, "reload 8 kHz constant wav");
    expect(loaded.samples.size() == tone.size(), "8 kHz reload keeps the length");
    bool same = loaded.ok && loaded.samples.size() == tone.size();
    if (same) {
        for (std::size_t index = 0; index < tone.size(); ++index) {
            if (loaded.samples[index] != static_cast<float>(tone[index])) {
                same = false;
                break;
            }
        }
    }
    expect(same, "8 kHz WAV round-trip does not scale samples");

    std::vector<float> dc(48000, 5000.f);
    const QString wide = dir.filePath(QStringLiteral("dc48.wav"));
    expect(writePcm16MonoWav(wide, dc, 48000), "write 48 kHz constant wav");
    const WavLoadResult resampled = loadAnalysisWav(wide);
    expect(resampled.ok, "reload 48 kHz constant wav");
    double minValue = 1e300;
    double maxValue = -1e300;
    for (float sample : resampled.samples) {
        minValue = std::min(minValue, static_cast<double>(sample));
        maxValue = std::max(maxValue, static_cast<double>(sample));
    }
    report("48 kHz DC 5000 resampled to 8 kHz: samples=" + std::to_string(resampled.samples.size())
        + " min=" + std::to_string(minValue) + " max=" + std::to_string(maxValue));
    expect(resampled.samples.size() == 8000, "one second at 48 kHz becomes one second at 8 kHz");
    expect(minValue == 5000.0 && maxValue == 5000.0, "linear resample of a constant level stays constant");
}

void testShippedDurationSettings()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary settings directory");
    const QString source = QStringLiteral(SRM_SOURCE_DIR) + QStringLiteral("/settings.ini");
    const QString dest = dir.filePath(QStringLiteral("settings.ini"));
    expect(QFile::copy(source, dest), "copy shipped settings.ini");
    Settings::setAppDataDirForTests(dir.path());
    const AppSettings loaded = Settings::loadSettings();
    report("shipped settings.ini via beginGroup(General):");
    report("  analysisWindowSec=" + std::to_string(loaded.analysisWindowSec));
    report("  updatesPerMinute=" + std::to_string(loaded.updatesPerMinute));
    report("  tempoChartWindowSec=" + std::to_string(loaded.tempoChartWindowSec));
    report("  autoCalibrate=" + std::to_string(loaded.autoCalibrate ? 1 : 0));
    report("  autoStopSilenceDuration=" + std::to_string(loaded.autoStopSilenceDuration));
    report("  keepRecordingFiles=" + std::to_string(loaded.keepRecordingFiles ? 1 : 0));
    expect(loaded.analysisWindowSec == 60, "shipped Standard analysis window is 60 s");
    expect(loaded.updatesPerMinute == 12, "shipped Standard updates are 12 per minute");
    expect(loaded.tempoChartWindowSec == 300, "shipped chart window is 300 s");
    expect(!loaded.autoCalibrate, "shipped file does not auto-detect speech");
    expect(loaded.autoStopSilenceDuration == 2000, "shipped pause is 2000 ms");
    expect(loaded.keepRecordingFiles, "shipped file keeps recording files");

    {
        QSettings stored(dest, QSettings::IniFormat);
        stored.beginGroup(QStringLiteral("General"));
        stored.setValue(QStringLiteral("maxRecordingTimeMs"), 99999);
        stored.setValue(QStringLiteral("minRecordingTimeMs"), 500);
        stored.endGroup();
        stored.sync();
    }
    Settings::saveSettings(loaded);
    QSettings stored(dest, QSettings::IniFormat);
    stored.beginGroup(QStringLiteral("General"));
    expect(!stored.contains(QStringLiteral("maxRecordingTimeMs")), "save removes maxRecordingTimeMs");
    expect(!stored.contains(QStringLiteral("minRecordingTimeMs")), "save removes minRecordingTimeMs");
    stored.endGroup();
    Settings::clearAppDataDirForTests();
}

void testExampleEnvelope(const QString& relativePath)
{
    const QString path = QStringLiteral(SRM_SOURCE_DIR) + QStringLiteral("/") + relativePath;
    const WavLoadResult loaded = loadAnalysisWav(path);
    expect(loaded.ok, "example wav loads");
    if (!loaded.ok || loaded.samples.size() < static_cast<std::size_t>(LiveCapture::kSampleRate))
        return;
    const int seconds = static_cast<int>(loaded.samples.size() / LiveCapture::kSampleRate);
    double first = 0;
    double last = 0;
    double minRms = 1e300;
    double maxRms = 0;
    int minAt = 0;
    int maxAt = 0;
    for (int second = 0; second < seconds; ++second) {
        const std::size_t begin = static_cast<std::size_t>(second) * LiveCapture::kSampleRate;
        const double rms = rmsOf(loaded.samples, begin, begin + LiveCapture::kSampleRate);
        if (second < 3)
            first += rms;
        if (second >= seconds - 3)
            last += rms;
        if (rms < minRms) {
            minRms = rms;
            minAt = second;
        }
        if (rms > maxRms) {
            maxRms = rms;
            maxAt = second;
        }
    }
    const int head = std::min(3, seconds);
    const int tail = std::min(3, seconds);
    report(relativePath.toStdString() + " seconds=" + std::to_string(seconds)
        + " meanRmsFirst3=" + std::to_string(first / head)
        + " meanRmsLast3=" + std::to_string(last / tail)
        + " minSecond=" + std::to_string(minAt) + ":" + std::to_string(minRms)
        + " maxSecond=" + std::to_string(maxAt) + ":" + std::to_string(maxRms));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testConstantToneIsNotFaded();
    testEqualAndDecayingBursts();
    testSpeechSegmentCopiesSamples();
    testWavRoundTrip();
    testShippedDurationSettings();
    testExampleEnvelope(QStringLiteral("data/tests/speech_rate_example_fast_185wpm.wav"));
    testExampleEnvelope(QStringLiteral("data/tests/speech_rate_example_slow_95wpm.wav"));
    if (failures != 0) {
        std::cerr << failures << " diagnostic check(s) failed\n";
        return 1;
    }
    std::cout << "signal-level diagnostic passed\n";
    return 0;
}
