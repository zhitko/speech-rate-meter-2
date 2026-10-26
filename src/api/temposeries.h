#ifndef TEMPOSERIES_H
#define TEMPOSERIES_H

#include "speechrateanalysis.h"

#include <QVariantList>

#include <vector>

// Tempo chart for an opened file. Same cadence as the microphone:
// one point every liveUpdateIntervalMs(updatesPerMinute), measured on the
// last analysis-window seconds ending at that time. The point at the end
// of the file is kept. Only the last chart-window seconds are returned.
QVariantList tempoSeries(const std::vector<float>& samples,
    const speechrate::Config& cfg,
    int windowSec,
    int chartWindowSec,
    int gaugeAverageCount,
    bool useAllReadings,
    int updatesPerMinute);

#endif
