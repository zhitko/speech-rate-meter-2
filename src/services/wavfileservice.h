#ifndef WAVFILESERVICE_H
#define WAVFILESERVICE_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * This header file defines the structures and functions for working with WAV
 * files. It includes definitions for the various chunks that make up a WAV file,
 * such as the WaveHeader, FormatChunk, DataChunk, CueChunk, LablChunk, and
 * LtxtChunk. The file also provides functions for opening, creating, saving,
 * closing, and cloning WAV files, as well as utility functions for handling
 * endianness and converting waveform data to vectors. This header is intended to
 * be used in conjunction with the WavFileApi class to provide audio processing
 * capabilities in the application.
 */
struct AudioFormat {
    int sampleRate;
    int channelCount;
    int bitsPerSample;
};

/**
 * CuePointType defines the type of cue point, which can be used to categorize
 * different types of markers in the audio data, such as pre-nucleus, nucleus,
 * and post-nucleus points. This categorization can be useful for analyzing
 * melodic contours and providing feedback on pitch accuracy in the Speech Rate Meter
 * application.
 */
enum class CuePointType { PRE_NUCLEUS,
    NUCLEUS,
    POST_NUCLEUS };

/**
 * CuePointData struct represents the data associated with a cue point in a WAV
 * file. It includes the unique ID of the cue point, its position in the audio
 * data, its length, an optional label, and its type (e.g., pre-nucleus,
 * nucleus, post-nucleus). This structure is used to store and manage cue point
 * information extracted from WAV files, which can be used for synchronization,
 * analysis, and feedback purposes in the Speech Rate Meter application.
 */
struct CuePointData {
    uint32_t id;
    uint32_t position;
    uint32_t length;
    std::string label;
    CuePointType type;
};

struct WaveFile;

/**
 * WavFileService class provides methods for opening WAV files, extracting audio
 * data, pitch, spectrum, cepstrum, and amplitude information. It also includes
 * methods for comparing patterns using dynamic programming. This class is
 * designed to be used in a QML context, allowing for easy integration with the
 * UI components of the application. The methods in this class enable the
 * application to analyze audio files and provide feedback to the user based on
 * the extracted features.
 */
class WavFileService {
public:
    /**
     * Constructs a WavFileService object with the specified root path. The root
     * path is used as the base directory for reading and writing WAV files, allowing
     * for organized file management within the application.
     *
     * @param rootPath - The root directory path for managing WAV files.
     */
    explicit WavFileService(const std::string& rootPath);

    /**
     * Writes a WAV file with the specified file name, audio data buffer, and
     * audio format. This method allows for saving audio data to a WAV file on
     * disk, which can be useful for exporting processed audio or for creating new
     * WAV files from generated audio data.
     *
     * @param fileName - The name of the WAV file to be created (including path).
     * @param buffer - A vector of characters containing the raw audio data to be
     * written to the WAV file.
     * @param format - An AudioFormat structure containing the sample rate, channel
     * count, and bits per sample for the audio data.
     * @return A string containing the full path of the created WAV file, or an
     * error message if the file could not be created.
     */
    std::string writeWaveFile(const std::string& fileName,
        const std::vector<char>& buffer,
        const AudioFormat& format);

    /**
     * Reads the audio data from the given WaveFile object and returns it as a
     * vector of doubles. This method allows for extracting the raw audio data from
     * a loaded WAV file, which can then be used for analysis, processing, or
     * feature extraction in the application.
     * @param waveFile - A pointer to the WaveFile object from which to extract audio
     * data.
     * @return A vector of doubles containing the audio data extracted from the
     * WaveFile.
     */
    static std::vector<double> readWaveData(WaveFile* waveFile);

    /**
     * Reads the cue points from the given WaveFile object and returns them as a
     * vector of CuePointData structures. This method allows for extracting the
     * cue point information from a loaded WAV file, which can be used for
     * synchronization, analysis, and feedback purposes in the Speech Rate Meter
     * application.
     *
     * @param waveFile - A pointer to the WaveFile object from which to extract
     * cue points.
     * @return A vector of CuePointData structures containing the cue points
     * extracted from the WaveFile.
     */
    static std::vector<CuePointData> readCuePoints(WaveFile* waveFile);

private:
    std::string m_rootPath;
};

#endif // WAVFILESERVICE_H
