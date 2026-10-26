#include "temposeries.h"

#include "helpers/settings.h"
#include "livecadence.h"

#include <QString>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <deque>

namespace {

constexpr int kSampleRate = LiveCapture::kSampleRate;
constexpr qint64 kMinWindowSamples = LiveCapture::kMinWindowSamples;

QString clockOf(double totalSeconds)
{
    const int seconds = std::max(0, static_cast<int>(std::floor(totalSeconds + 1e-6)));
    const int minutes = seconds / 60;
    const int remain = seconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(remain, 2, 10, QLatin1Char('0'));
}

QVariantMap tempoPoint(double seconds, double speechRate, double articulationRate)
{
    QVariantMap point;
    point.insert(QStringLiteral("seconds"), seconds);
    point.insert(QStringLiteral("clock"), clockOf(seconds));
    point.insert(QStringLiteral("speechRate"), speechRate);
    point.insert(QStringLiteral("articulationRate"), articulationRate);
    return point;
}

// Odd count is the middle after sorting. Even count is the mean of the two
// central values. Same rule as the gauge.
double medianValue(const std::deque<double>& window)
{
    if (window.empty())
        return 0;
    std::vector<double> samples(window.begin(), window.end());
    std::sort(samples.begin(), samples.end());
    const std::size_t mid = samples.size() / 2;
    if (samples.size() % 2 == 1)
        return samples[mid];
    return (samples[mid - 1] + samples[mid]) / 2.0;
}

// keepAll: median of every reading pushed so far (the chart span).
// Otherwise the gauge median of the last `count` readings.
// Speech rate and articulation keep separate windows.
double rollingMedian(std::deque<double>& window, double rate, int count, bool keepAll)
{
    window.push_back(rate);
    if (!keepAll) {
        const int keep = std::clamp(count, 1, 30);
        while (static_cast<int>(window.size()) > keep)
            window.pop_front();
    }
    return medianValue(window);
}

} // namespace

QVariantList tempoSeries(const std::vector<float>& samples,
    const speechrate::Config& cfg,
    int windowSec,
    int chartWindowSec,
    int gaugeAverageCount,
    bool useAllReadings,
    int updatesPerMinute)
{
    QVariantList points;
    const qint64 total = static_cast<qint64>(samples.size());
    if (total < kMinWindowSamples)
        return points;

    // Same wait as LiveCapture::publish: max(1, 60000 / updates) ms.
    const int intervalMs = liveUpdateIntervalMs(updatesPerMinute);
    qint64 step = static_cast<qint64>(intervalMs) * kSampleRate / 1000;
    if (step < 1)
        step = 1;
    const qint64 windowSamples = static_cast<qint64>(clampAnalysisWindowSec(windowSec)) * kSampleRate;
    const int historySec = std::clamp(chartWindowSec, 30, 1800);
    // Only the last chart window is drawn, so only those points are measured.
    // A 5–30 minute file at 60 updates per minute stays on this span.
    const qint64 regionStart = std::max<qint64>(
        0, total - static_cast<qint64>(historySec) * kSampleRate);

    std::deque<double> speechWindow;
    std::deque<double> articulationWindow;

    const auto push = [&](qint64 end) {
        if (end < kMinWindowSamples || end < regionStart || end > total)
            return;
        const qint64 start = (windowSamples > 0 && end > windowSamples) ? end - windowSamples : 0;
        // Shorter than the analysis window at the start of the file, but
        // never below the live gate (1.5 s of audio).
        if (end - start < kMinWindowSamples)
            return;
        const speechrate::Measurement measured = speechrate::measure(
            std::vector<float>(samples.begin() + static_cast<std::ptrdiff_t>(start),
                samples.begin() + static_cast<std::ptrdiff_t>(end)),
            cfg);
        if (!measured.metrics.valid)
            return;
        points.append(tempoPoint(static_cast<double>(end) / kSampleRate,
            rollingMedian(speechWindow, measured.metrics.speechRate, gaugeAverageCount, useAllReadings),
            rollingMedian(articulationWindow, measured.metrics.articulationRate, gaugeAverageCount, useAllReadings)));
    };

    // Grid of window ends: step, 2*step, ... and always the file end.
    // The first end is the first mark that holds kMinWindowSamples.
    qint64 end = step;
    if (end < kMinWindowSamples)
        end = ((kMinWindowSamples + step - 1) / step) * step;
    if (end < regionStart) {
        const qint64 jumps = (regionStart - end + step - 1) / step;
        end += jumps * step;
    }
    while (end < total) {
        push(end);
        if (total - end < step)
            break;
        end += step;
    }
    push(total);
    return points;
}
