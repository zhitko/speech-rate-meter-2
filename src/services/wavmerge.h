#ifndef WAVMERGE_H
#define WAVMERGE_H

#include <QString>
#include <QStringList>

/**
 * Joins PCM WAV files into one 16-bit mono WAV at 8000 Hz.
 *
 * The data payload of each file is copied in order. The samples are not
 * loaded. An empty list succeeds and writes nothing. One file is written
 * again with a single correct header. Every input must already be PCM,
 * one channel, 16-bit, 8000 Hz. A mismatch or a write error removes any
 * partial output and leaves the inputs in place.
 */
struct WavMergeResult {
    bool ok = false;
    QString path;
    QString error;
};

WavMergeResult mergePcm16MonoWavs(const QStringList& chunkPaths, const QString& outputPath);

#endif
