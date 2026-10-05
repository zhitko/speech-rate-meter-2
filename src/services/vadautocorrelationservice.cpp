#include "vadautocorrelationservice.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

namespace {

std::pair<int, int> computeLagRange(double sampleRate,
    double minF0,
    double maxF0)
{
    if (sampleRate <= 0.0)
        sampleRate = 8000.0;
    if (minF0 <= 0.0)
        minF0 = 80.0;
    if (maxF0 <= 0.0)
        maxF0 = 200.0;
    if (minF0 > maxF0)
        std::swap(minF0, maxF0);

    int lagMin = static_cast<int>(std::round(sampleRate / maxF0));
    int lagMax = static_cast<int>(std::round(sampleRate / minF0));
    lagMin = std::max(1, lagMin);
    lagMax = std::max(lagMin, lagMax);
    return { lagMin, lagMax };
}

} // namespace

VADAutocorrelationService::VADAutocorrelationService(QObject* parent)
    : QObject(parent)
{
    auto lagRange = computeLagRange(m_sampleRate, m_minF0, m_maxF0);
    qDebug().nospace()
        << "VAD-ACF: Initialized"
        << " | FRAME=" << FRAME_SIZE << "samples"
        << " HOP=" << HOP_SIZE << "samples"
        << " K=" << K_FRAMES
        << " LAG[" << lagRange.first << ".." << lagRange.second << "]"
        << " F0[" << m_minF0 << ".." << m_maxF0 << " Hz]"
        << " | U-thresholds[H=" << QString::number(DEFAULT_VOICE_THRESHOLD_U_HIGH, 'f', 3)
        << " L=" << QString::number(DEFAULT_VOICE_THRESHOLD_U_LOW, 'f', 3) << "]"
        << " | energyGate=" << QString::number(DEFAULT_ENERGY_THRESHOLD, 'f', 6);
}

// ---------------------------------------------------------------------------
// processAudioSamples
// ---------------------------------------------------------------------------
std::vector<double> VADAutocorrelationService::processAudioSamples(
    const qint16* samples, qint64 numSamples)
{
    // 1. Accumulate incoming samples
    m_sampleBuf.reserve(m_sampleBuf.size() + static_cast<std::size_t>(numSamples));
    for (qint64 i = 0; i < numSamples; ++i) {
        m_sampleBuf.push_back(samples[i]);
    }

    // Precompute lag range (constant for this instance)
    auto lagRange = computeLagRange(m_sampleRate, m_minF0, m_maxF0);

    // 2. Extract C(n) frames
    while (m_sampleBuf.size() >= static_cast<std::size_t>(FRAME_SIZE)) {
        std::vector<double> frame(FRAME_SIZE);
        for (int i = 0; i < FRAME_SIZE; ++i) {
            frame[i] = static_cast<double>(m_sampleBuf[i]) / 32768.0;
        }
        applyHammingWindow(frame);

        double C_n = findMaxAutocorrelation(frame, lagRange.first, lagRange.second);
        m_C.push_back(C_n);

        m_sampleBuf.erase(m_sampleBuf.begin(), m_sampleBuf.begin() + HOP_SIZE);
    }

    // Grow U, H to match C (new slots initialised to 0.0)
    if (m_U.size() < m_C.size())
        m_U.resize(m_C.size(), 0.0);
    if (m_H.size() < m_C.size())
        m_H.resize(m_C.size(), 0.0);

    // 3. First smoothing pass: U(n) = mean(C[n-K+1..n+K-1]), H(n) = |C(n)-U(n)|
    //    All frames (including boundaries) are processed with clamped windows.
    //
    std::vector<double> newUValues;
    for (; m_valid_U < static_cast<int>(m_C.size()); ++m_valid_U) {
        int i = m_valid_U;
        // Clamp left edge at 0 for boundary frames (mirrors energy service skip behaviour
        // but produces a real value instead of leaving the slot zero).
        int lo = std::max(0, i - K_FRAMES + 1);
        int hi = std::min(static_cast<int>(m_C.size()) - 1, i + K_FRAMES - 1);

        double sumC = 0.0;
        for (int j = lo; j <= hi; ++j)
            sumC += m_C[j];
        m_U[i] = sumC / static_cast<double>(hi - lo + 1);
        m_H[i] = std::abs(m_C[i] - m_U[i]);

        // VAD decision on U(n) immediately after computing
        bool frameSpeech = isSpeech(m_U[i], m_currentVadState);
        m_vadDecisions.push_back(frameSpeech);

        bool stateChanged = (frameSpeech != m_currentVadState);
        if (stateChanged || i % 10 == 0) {
            QString stateStr = frameSpeech ? "SPEECH" : "SILENCE";
            QString oldStateStr = m_currentVadState ? "SPEECH" : "SILENCE";
            QString reason;
            if (stateChanged) {
                if (frameSpeech) {
                    reason = QString(" [TRIGGERED: U=%1 > H=%2]")
                                 .arg(QString::number(m_U[i], 'f', 4))
                                 .arg(QString::number(m_voiceThresholdUHigh, 'f', 4));
                } else {
                    reason = QString(" [DROPPED: U=%1 < L=%2]")
                                 .arg(QString::number(m_U[i], 'f', 4))
                                 .arg(QString::number(m_voiceThresholdULow, 'f', 4));
                }
            }
            qDebug().nospace()
                << "VAD-ACF[F" << i << "]: " << oldStateStr << " → " << stateStr
                << " | C=" << QString::number(m_C[i], 'f', 3)
                << " U=" << QString::number(m_U[i], 'f', 3)
                << " H=" << QString::number(m_H[i], 'f', 3)
                << reason;
        }

        m_currentVadState = frameSpeech;
        newUValues.push_back(m_U[i]);
    }

    return newUValues;
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------
void VADAutocorrelationService::reset()
{
    m_sampleBuf.clear();
    m_C.clear();
    m_U.clear();
    m_H.clear();
    m_vadDecisions.clear();
    m_currentVadState = false;
    m_valid_U = 0;
    qDebug() << "VAD-ACF: Reset — cleared all state and buffers";
}

// ---------------------------------------------------------------------------
// isSpeech — based on U(n)
// ---------------------------------------------------------------------------
bool VADAutocorrelationService::isSpeech(double U_n, bool currentState) const
{
    if (currentState) {
        return U_n > m_voiceThresholdULow;
    } else {
        return U_n > m_voiceThresholdUHigh;
    }
}

// ---------------------------------------------------------------------------
// Saved metrics
// ---------------------------------------------------------------------------
void VADAutocorrelationService::updateSavedMetrics()
{
    m_savedC = m_C;
    m_savedU = m_U;

    // Trim to the number of actually computed values
    if (static_cast<int>(m_savedU.size()) > m_valid_U)
        m_savedU.resize(static_cast<std::size_t>(m_valid_U));

    double minC = 0.0, maxC = 0.0, avgC = 0.0;
    if (!m_savedC.empty()) {
        minC = *std::min_element(m_savedC.begin(), m_savedC.end());
        maxC = *std::max_element(m_savedC.begin(), m_savedC.end());
        avgC = std::accumulate(m_savedC.begin(), m_savedC.end(), 0.0)
            / static_cast<double>(m_savedC.size());
    }
    int speechFrames = static_cast<int>(
        std::count(m_vadDecisions.begin(), m_vadDecisions.end(), true));

    qDebug().nospace()
        << "VAD-ACF: Saved"
        << " | C=" << m_savedC.size()
        << " U=" << m_savedU.size()
        << " | speech_frames=" << speechFrames
        << " | C[min=" << QString::number(minC, 'f', 3)
        << " max=" << QString::number(maxC, 'f', 3)
        << " avg=" << QString::number(avgC, 'f', 3) << "]";
}

void VADAutocorrelationService::cropSavedMetrics(int startFrame, int endFrame)
{
    auto crop = [](std::vector<double>& v, int s, int e) {
        if (s < 0)
            s = 0;
        int sz = static_cast<int>(v.size());
        if (e > sz)
            e = sz;
        if (s >= e) {
            v.clear();
            return;
        }
        v = std::vector<double>(v.begin() + s, v.begin() + e);
    };
    crop(m_savedC, startFrame, endFrame);
    crop(m_savedU, startFrame, endFrame);
}

// ---------------------------------------------------------------------------
// DSP helpers
// ---------------------------------------------------------------------------
void VADAutocorrelationService::applyHammingWindow(std::vector<double>& frame)
{
    int N = static_cast<int>(frame.size());
    for (int n = 0; n < N; ++n) {
        double w = 0.54 - 0.46 * std::cos(2.0 * M_PI * n / (N - 1.0));
        frame[n] *= w;
    }
}

double VADAutocorrelationService::calculateAutocorrelation(
    const std::vector<double>& frame, int lag)
{
    double sum = 0.0;
    int limit = static_cast<int>(frame.size()) - lag;
    for (int n = 0; n < limit; ++n) {
        sum += frame[n] * frame[n + lag];
    }
    return sum;
}

double VADAutocorrelationService::findMaxAutocorrelation(
    const std::vector<double>& frame, int lagMin, int lagMax)
{
    // R(0) — frame energy after Hamming windowing
    double r0 = 0.0;
    for (double v : frame)
        r0 += v * v;

    // Energy gate: forces C=0 for near-silent frames.
    // DEFAULT_ENERGY_THRESHOLD is set very low (0.0001) so quiet Windows
    // microphone input (AGC-reduced amplitude) is not silenced here.
    // Log R0 at trace level to aid per-platform threshold tuning.
    if (r0 < m_energyThreshold) {
        // qDebug().nospace()
        //     << "VAD-ACF: energy gate R0=" << QString::number(r0, 'f', 6)
        //     << " < threshold=" << QString::number(m_energyThreshold, 'f', 6)
        //     << " → C=0 (raise threshold to suppress noise, lower to pass quiet mic)";
        return 0.0;
    }

    double maxCorr = 0.0;
    for (int lag = lagMin; lag <= lagMax && lag < static_cast<int>(frame.size()); ++lag) {
        double normalized = calculateAutocorrelation(frame, lag) / r0;
        if (normalized > maxCorr)
            maxCorr = normalized;
    }
    return maxCorr;
}