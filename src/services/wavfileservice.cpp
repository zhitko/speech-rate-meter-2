#include "wavfileservice.h"
#include "helpers/logger.h"
#include "helpers/wavFile.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

WavFileService::WavFileService(const std::string& rootPath)
    : m_rootPath(rootPath)
{
    LOG_DEBUG() << "Start: WavFileService constructor - rootPath=" << rootPath;
    LOG_DEBUG() << "Finish: WavFileService constructor";
}

std::string WavFileService::writeWaveFile(const std::string& fileName,
    const std::vector<char>& buffer,
    const AudioFormat& format)
{
    LOG_DEBUG() << "Start: writeWaveFile - fileName=" << fileName
                << ", buffer.size=" << buffer.size()
                << ", channels=" << format.channelCount
                << ", sampleRate=" << format.sampleRate
                << ", bitsPerSample=" << format.bitsPerSample;
    fs::path appDir(m_rootPath);
    fs::path recordsDir = appDir / "data" / "records";

    if (!fs::exists(recordsDir)) {
        fs::create_directories(recordsDir);
    }

    fs::path relativePath = fs::path("data") / "records" / (fileName + ".wav");
    fs::path absoluteFilePath = appDir / relativePath;

    WaveHeader* waveHeader = makeWaveHeader(buffer.size());
    if (!waveHeader) {
        LOG_CRITICAL() << "Failed to create wave header";
        return "";
    }

    FormatChunk* formatChunk = makeFormatChunk(
        format.channelCount, format.sampleRate, format.bitsPerSample);
    if (!formatChunk) {
        LOG_CRITICAL() << "Failed to create format chunk";
        // Note: waveHeader cleanup is handled by makeWaveFile or caller
        return "";
    }

    // Cast constness away because makeDataChunk expects char* but likely doesn't
    // modify it (it copies).
    DataChunk* dataChunk = makeDataChunk(buffer.size(), const_cast<char*>(buffer.data()));
    if (!dataChunk) {
        LOG_CRITICAL() << "Failed to create data chunk";
        return "";
    }

    WaveFile* waveFile = makeWaveFile(waveHeader, formatChunk, dataChunk, nullptr,
        nullptr, 0, nullptr, 0);
    if (!waveFile) {
        LOG_CRITICAL() << "Failed to create wave file structure";
        return "";
    }

    saveWaveFile(waveFile, absoluteFilePath.string());
    waveCloseFile(waveFile);

    LOG_DEBUG() << "Finish: writeWaveFile - filePath=" << relativePath.string();
    return relativePath.string();
}

std::vector<double> WavFileService::readWaveData(WaveFile* waveFile)
{
    LOG_DEBUG() << "Start: readWaveData";
    std::vector<double> samples;
    if (!waveFile || !waveFile->dataChunk) {
        LOG_WARNING() << "WaveFile or DataChunk is null";
        return samples;
    }

    void* data = waveFile->dataChunk->waveformData;
    uint32_t dataSize = littleEndianBytesToUInt32(waveFile->dataChunk->chunkDataSize);
    uint16_t bitDepth = littleEndianBytesToUInt16(
        waveFile->formatChunk->significantBitsPerSample);

    samples = waveformDataToVector(data, dataSize, bitDepth);

    LOG_DEBUG() << "Finish: readWaveData - samples.size=" << samples.size();
    return samples;
}

std::vector<CuePointData> WavFileService::readCuePoints(WaveFile* waveFile)
{
    LOG_DEBUG() << "Start: readCuePoints";
    std::vector<CuePointData> cuePoints;
    if (!waveFile || !waveFile->cueChunk) {
        LOG_WARNING() << "WaveFile or CueChunk is null";
        return cuePoints;
    }

    uint32_t cuePointsCount = littleEndianBytesToUInt32(waveFile->cueChunk->cuePointsCount);
    LOG_DEBUG() << "Found " << cuePointsCount << " cue points";

    for (uint32_t i = 0; i < cuePointsCount; ++i) {
        CuePointData cuePoint;
        CuePoint* cp = &waveFile->cueChunk->cuePoints[i];

        cuePoint.id = littleEndianBytesToUInt32(cp->cuePointID);
        cuePoint.position = littleEndianBytesToUInt32(cp->playOrderPosition);

        // Initialize optional fields
        cuePoint.length = 0;

        // Find associated label
        std::string label = "";
        bool ltxtFound = false;
        if (waveFile->listCount > 0) {
            for (uint32_t j = 0; j < waveFile->listCount; ++j) {
                ListChunk* listChunk = &waveFile->listChunks[j];
                if (label.empty()) {
                    for (uint32_t k = 0; k < listChunk->lablCount; ++k) {
                        LablChunk* lablChunk = &listChunk->lablChunks[k];
                        if (littleEndianBytesToUInt32(lablChunk->cuePointID) == cuePoint.id) {
                            if (lablChunk->text) {
                                label = std::string(lablChunk->text);
                            }
                            break;
                        }
                    }
                }

                if (!ltxtFound) {
                    for (uint32_t k = 0; k < listChunk->ltxtCount; ++k) {
                        LtxtChunk* ltxtChunk = &listChunk->ltxtChunks[k];
                        if (littleEndianBytesToUInt32(ltxtChunk->cuePointID) == cuePoint.id) {
                            cuePoint.length = littleEndianBytesToUInt32(ltxtChunk->sampleLength);
                            ltxtFound = true;
                            break;
                        }
                    }
                }

                if (!label.empty() && ltxtFound) {
                    break;
                }
            }
        }
        cuePoint.label = label;

        if (!label.empty()) {
            char firstChar = std::tolower(label[0]);
            if (firstChar == 'p') {
                cuePoint.type = CuePointType::PRE_NUCLEUS;
            } else if (firstChar == 'n') {
                cuePoint.type = CuePointType::NUCLEUS;
            } else if (firstChar == 't') {
                cuePoint.type = CuePointType::POST_NUCLEUS;
            } else {
                cuePoint.type = CuePointType::NUCLEUS; // Default
            }
        } else {
            cuePoint.type = CuePointType::NUCLEUS; // Default
        }

        LOG_DEBUG() << "Cue Point " << i << ": ID=" << cuePoint.id
                    << " Label=" << cuePoint.label
                    << " Position=" << cuePoint.position
                    << " Length=" << cuePoint.length
                    << " Type=" << static_cast<int>(cuePoint.type);

        cuePoints.push_back(cuePoint);
    }

    std::sort(cuePoints.begin(), cuePoints.end(),
        [](const CuePointData& a, const CuePointData& b) {
            return a.position < b.position;
        });

    LOG_DEBUG() << "Finish: readCuePoints - cuePoints.size=" << cuePoints.size();
    return cuePoints;
}