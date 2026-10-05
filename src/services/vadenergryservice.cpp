#include "vadenergryservice.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Helper function for percentile calculation
static double percentileValue(std::vector<double> values, double percentile)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    int idx = static_cast<int>(std::ceil(percentile * values.size())) - 1;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(values.size())) idx = static_cast<int>(values.size()) - 1;
    return values[idx];
}

VADEnergyService::VADEnergyService(QObject* parent)
    : QObject(parent)
{
}

std::vector<double> VADEnergyService::processAudioSamples(const qint16* samples, qint64 numSamples)
{
    std::vector<double> newVValues;

    // Add samples to buffer
    for (qint64 i = 0; i < numSamples; ++i) {
        m_sampleBuf.push_back(samples[i]);
    }

    // Calculate A (Amplitude) from 128-sample frames with 64-sample hop
    while (static_cast<int>(m_sampleBuf.size()) >= FRAME_SIZE) {
        long long sumAmp = 0;
        for (int i = 0; i < FRAME_SIZE; ++i) {
            sumAmp += std::abs(static_cast<int>(m_sampleBuf[i]));
        }
        m_A.push_back(static_cast<double>(sumAmp) / FRAME_SIZE);
        m_sampleBuf.erase(m_sampleBuf.begin(), m_sampleBuf.begin() + HOP_SIZE);
    }

    // Ensure vectors are large enough
    if (m_U.size() < m_A.size()) m_U.resize(m_A.size(), 0.0);
    if (m_H.size() < m_A.size()) m_H.resize(m_A.size(), 0.0);
    if (m_V.size() < m_A.size()) m_V.resize(m_A.size(), 0.0);

    // Calculate U(n) and H(n) incrementally
    for (; m_valid_U + K_FRAMES - 1 < static_cast<int>(m_A.size()); ++m_valid_U) {
        int i = m_valid_U;
        if (i - K_FRAMES + 1 < 0) continue;

        double sum_A = 0.0;
        for (int j = i - K_FRAMES + 1; j < i + K_FRAMES; ++j) {
            sum_A += m_A[j];
        }
        m_U[i] = sum_A / (2 * K_FRAMES - 1);
        m_H[i] = std::abs(m_A[i] - m_U[i]);
    }

    // Calculate V(n) incrementally
    for (; m_valid_V + K_FRAMES - 1 < m_valid_U; ++m_valid_V) {
        int i = m_valid_V;
        if (i - K_FRAMES + 1 < 0) continue;

        double sum_H = 0.0;
        for (int j = i - K_FRAMES + 1; j < i + K_FRAMES; ++j) {
            sum_H += m_H[j];
        }
        m_V[i] = sum_H / (2 * K_FRAMES - 1);
        newVValues.push_back(m_V[i]);
    }

    return newVValues;
}

bool VADEnergyService::addCalibrationFrame(double V_n)
{
    if (m_calibrationCounter < CALIBRATION_FRAMES) {
        m_calibrationFrames.push_back(V_n);
        m_calibrationCounter++;
        if (m_calibrationCounter == CALIBRATION_FRAMES) {
            // Use 95th percentile × 3 as threshold — robust against noise spikes
            m_Pe = percentileValue(m_calibrationFrames, 0.95) * 3.0;
            emit calibrationFinished(m_Pe);
            return true;
        }
    }
    return false;
}

void VADEnergyService::reset()
{
    m_sampleBuf.clear();
    m_A.clear();
    m_U.clear();
    m_H.clear();
    m_V.clear();
    m_valid_U = 0;
    m_valid_V = 0;
    resetCalibration();
}

void VADEnergyService::resetCalibration()
{
    m_calibrationCounter = 0;
    m_calibrationFrames.clear();
}

void VADEnergyService::prepareForCalibration()
{
    m_sampleBuf.clear();
    m_A.clear();
    m_U.clear();
    m_H.clear();
    m_V.clear();
    m_valid_U = 0;
    m_valid_V = 0;
    resetCalibration();
}

void VADEnergyService::updateSavedMetrics()
{
    m_savedA = m_A;
    m_savedU = m_U;
    m_savedV = m_V;
}

void VADEnergyService::cropSavedMetrics(int startFrame, int endFrame)
{
    if (startFrame >= static_cast<int>(m_savedA.size())) {
        return;
    }

    int count = endFrame - startFrame;
    if (startFrame + count > static_cast<int>(m_savedA.size())) {
        count = m_savedA.size() - startFrame;
    }

    if (count > 0) {
        m_savedA = std::vector<double>(m_savedA.begin() + startFrame, 
                                       m_savedA.begin() + startFrame + count);
        
        if (startFrame < static_cast<int>(m_savedU.size())) {
            int u_count = std::min<int>(count, m_savedU.size() - startFrame);
            m_savedU = std::vector<double>(m_savedU.begin() + startFrame, 
                                          m_savedU.begin() + startFrame + u_count);
        }
        
        if (startFrame < static_cast<int>(m_savedV.size())) {
            int v_count = std::min<int>(count, m_savedV.size() - startFrame);
            m_savedV = std::vector<double>(m_savedV.begin() + startFrame, 
                                          m_savedV.begin() + startFrame + v_count);
        }
    }
}

double VADEnergyService::calculateThresholdFromValues(const std::vector<double>& values)
{
    if (values.empty()) return 50000.0;
    std::vector<double> valuesCopy = values;
    double percentile95 = percentileValue(valuesCopy, 0.95);
    return percentile95 * 3.0;
}

std::vector<double> VADEnergyService::getAndClearRecentVValues()
{
    std::vector<double> result = m_V;
    m_V.clear();
    m_savedV.clear();
    return result;
}
