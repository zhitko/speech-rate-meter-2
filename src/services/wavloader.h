#ifndef WAVLOADER_H
#define WAVLOADER_H

#include <QString>
#include <vector>

/**
 * Loads a WAV for whole-file analysis and for Listen after Open File.
 *
 * PCM (8/16/24/32-bit, including WAVE_FORMAT_EXTENSIBLE) and IEEE float
 * (32/64-bit) are accepted. Extra chunks such as LIST, bext, and fact are
 * skipped. Channels are mixed to mono and the result is resampled to 8000 Hz.
 * Sample values are on the signed 16-bit scale the rest of the analysis uses.
 * On failure ok is false, samples is empty, and error is a message for the UI.
 */
struct WavLoadResult {
    bool ok = false;
    QString error;
    std::vector<float> samples;
    int sampleRate = 0;
    int channelCount = 0;
    int bitsPerSample = 0;
};

WavLoadResult loadAnalysisWav(const QString& path);

/** Writes mono signed 16-bit little-endian PCM. samples are on that scale. */
bool writePcm16MonoWav(const QString& path, const std::vector<float>& samples, int sampleRate);

#endif
