#ifndef WAVFILE_H
#define WAVFILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
struct WaveHeader {
    char chunkID[4]; // Must be "RIFF" (0x52494646)
    char dataSize[4]; // Byte count for the rest of the file (i.e. file length - 8
                      // bytes)
    char riffType[4]; // Must be "WAVE" (0x57415645)
};

/**
 * FormatChunk contains information about the audio format, such as the number of
 * channels, sample rate, and bit depth. This chunk is essential for correctly
 * interpreting the audio data in the DataChunk.
 */
struct FormatChunk {
    char chunkID[4]; // String: must be "fmt " (0x666D7420).
    char chunkDataSize[4]; // Unsigned 4-byte little endian int: Byte count for
                           // the remainder of the chunk: 16 + extraFormatbytes.
    char compressionCode[2]; // Unsigned 2-byte little endian int
    char numberOfChannels[2]; // Unsigned 2-byte little endian int
    char sampleRate[4]; // Unsigned 4-byte little endian int
    char averageBytesPerSecond
        [4]; // Unsigned 4-byte little endian int: This value indicates how many
             // bytes of wave data must be streamed to a D/A converter per second
             // in order to play the wave file. This information is useful when
             // determining if data can be streamed from the source fast enough to
             // keep up with playback. = SampleRate * BlockAlign.
    char blockAlign[2]; // Unsigned 2-byte little endian int: The number of bytes
                        // per sample slice. This value is not affected by the
                        // number of channels and can be calculated with the
                        // formula: blockAlign = significantBitsPerSample / 8 *
                        // numberOfChannels
    char significantBitsPerSample[2]; // Unsigned 2-byte little endian int
};

/**
 * DataChunk contains the actual audio data of the WAV file. The chunkDataSize
 * field indicates the size of the waveform data in bytes, and the waveformData
 * pointer points to the raw audio data. The isWaveformDataOwned flag indicates
 * whether the DataChunk is responsible for managing the memory of the
 * waveformData.
 */
struct DataChunk {
    char chunkID[4]; // String: must be "data"
    char chunkDataSize[4]; // Unsigned 4-byte little endian int
    char* waveformData;
    int isWaveformDataOwned; // Is wave form data owned by this structure
};

/**
 * CuePoint represents a specific point in the audio data, defined by its
 * position and associated metadata. Cue points can be used for various purposes,
 * such as marking important events in the audio or providing synchronization
 * points for analysis.
 */
struct CuePoint {
    char cuePointID[4]; // a unique ID for the Cue Point.
    char playOrderPosition
        [4]; // Unsigned 4-byte little endian int: If a Playlist chunk is present
             // in the Wave file, this the sample number at which this cue point
             // will occur during playback of the entire play list as defined by
             // the play list's order.  **Otherwise set to same as sample
             // offset??***  Set to 0 when there is no playlist.
    char
        dataChunkID[4]; // Unsigned 4-byte little endian int: The ID of the chunk
                        // containing the sample data that corresponds to this cue
                        // point.  If there is no playlist, this should be 'data'.
    char chunkStart[4]; // Unsigned 4-byte little endian int: The byte offset into
                        // the Wave List Chunk of the chunk containing the sample
                        // that corresponds to this cue point. This is the same
                        // chunk described by the Data Chunk ID value. If no Wave
                        // List Chunk exists in the Wave file, this value is 0.
    char blockStart[4]; // Unsigned 4-byte little endian int: The byte offset into
                        // the "data" or "slnt" Chunk to the start of the block
                        // containing the sample. The start of a block is defined
                        // as the first byte in uncompressed PCM wave data or the
                        // last byte in compressed wave data where decompression
                        // can begin to find the value of the corresponding sample
                        // value.
    char frameOffset[4]; // Unsigned 4-byte little endian int: The offset into the
                         // block (specified by Block Start) for the sample that
                         // corresponds to the cue point.
};

/**
 * CueChunk contains an array of CuePoint structures, along with metadata about
 * the number of cue points. This chunk is used to store and manage cue points
 * within a WAV file, allowing for efficient access and manipulation of these
 * important markers in the audio data.
 */
struct CueChunk {
    char chunkID[4]; // String: Must be "cue " (0x63756520).
    char
        chunkDataSize[4]; // Unsigned 4-byte little endian int: Byte count for the
                          // remainder of the chunk: 4 (size of cuePointsCount) +
                          // (24 (size of CuePoint struct) * number of CuePoints).
    char cuePointsCount[4]; // Unsigned 4-byte little endian int: Length of
                            // cuePoints[].
    CuePoint* cuePoints;
};

/**
 * LablChunk represents a labeled cue point in the audio data. It contains a
 * unique ID for the cue point and a text label associated with that cue point.
 * This chunk can be used to provide descriptive information about specific points
 * in the audio, such as marking the start of a verse or chorus in a song.
 */
struct LablChunk {
    char chunkID[4]; // String: Must be "labl" (0x6C61626C)
    char chunkDataSize[4];
    char cuePointID[4];
    char* text;
};

/**
 * LtxtChunk represents a labeled text cue point in the audio data. It contains
 * metadata about the cue point, such as its unique ID, sample length, and
 * localization information (country, language, dialect, code page). Additionally,
 * it includes a text field that can be used to provide descriptive information
 * about the cue point. This chunk is useful for adding detailed annotations to
 * specific points in the audio.
 */
struct LtxtChunk {
    char chunkID[4]; // String: Must be "ltxt" (0x6C747874)
    char chunkDataSize[4];
    char cuePointID[4];
    char sampleLength[4];
    char purposeID[4];
    char country[2];
    char language[2];
    char dialect[2];
    char codePage[2];
    char* text;
};

/**
 * ListChunk is a container for multiple LtxtChunk and LablChunk structures. It
 * includes metadata about the number of each type of chunk it contains, as well
 * as pointers to arrays of these chunks. This structure allows for efficient
 * organization and access to labeled cue points and their associated text within
 * a WAV file.
 */
struct ListChunk {
    char chunkID[4]; // String: Must be "list" (0x6C696E74).
    char chunkDataSize[4]; // Unsigned 4-byte little endian int: Byte count for
                           // the remainder of the chunk: 4 (size of typeID) + (24
                           // (size of ListItem struct) * number of ListItems).
    char typeID[4]; // "adtl" (0x6164746C)
    uint32_t ltxtCount;
    LtxtChunk* ltxtChunks;
    uint32_t lablCount;
    LablChunk* lablChunks;
};

/**
 * WaveFile is the main structure representing a WAV file. It contains pointers to
 * the various chunks that make up the WAV file, such as the WaveHeader,
 * FormatChunk, DataChunk, CueChunk, and ListChunk. The structure also includes
 * metadata about the file path and file handle, allowing for efficient management
 * of the WAV file's data and resources.
 */
struct ChunkLocation {
    long startOffset; // in bytes
    long size; // in bytes
};

/**
 * WaveFile is the main structure representing a WAV file. It contains pointers to
 * the various chunks that make up the WAV file, such as the WaveHeader,
 * FormatChunk, DataChunk, CueChunk, and ListChunk. The structure also includes
 * metadata about the file path and file handle, allowing for efficient management
 * of the WAV file's data and resources.
 */
struct WaveFile {
    char* filePath;
    FILE* file;
    WaveHeader* waveHeader;
    FormatChunk* formatChunk;
    DataChunk* dataChunk;
    CueChunk* cueChunk;
    uint32_t listCount;
    ListChunk* listChunks;
};

/**
 * Initializes a new WaveFile structure with default values. This function is
 * intended to be used as a starting point for creating or loading a WAV file,
 * allowing for the proper allocation and initialization of the necessary
 * structures and resources.
 *
 * @return A pointer to an initialized WaveFile structure.
 */
WaveFile* initWaveFile();

WaveFile* waveOpenHFile(int);

/**
 * Opens a WAV file from the specified file path and returns a pointer to a
 * WaveFile object containing the audio data and metadata. This function reads
 * the contents of the WAV file, parses the various chunks, and populates the
 * WaveFile structure accordingly.
 *
 * @param filePath - The file path of the WAV file to be opened.
 * @return A pointer to a WaveFile object containing the loaded audio data and
 * metadata.
 */
WaveFile* waveOpenFile(const std::string&);

/**
 * Creates a new WaveFile structure from the provided chunk data. This function
 * allows for the construction of a WaveFile object using pre-existing chunk
 * data, which can be useful for creating new WAV files or modifying existing
 * ones.
 *
 * @param waveHeader - A pointer to a WaveHeader structure containing the header
 * information for the WAV file.
 * @param formatChunk - A pointer to a FormatChunk structure containing the audio
 * format information for the WAV file.
 * @param dataChunk - A pointer to a DataChunk structure containing the audio
 * data for the WAV file.
 * @param cueChunk - A pointer to a CueChunk structure containing the cue points
 * for the WAV file.
 * @param ltxtChunks - An array of LtxtChunk structures containing labeled text
 * cue points for the WAV file.
 * @param ltxtChunksCount - The number of LtxtChunk structures in the ltxtChunks
 * array.
 * @param lablChunks - An array of LablChunk structures containing labeled cue
 * points for the WAV file.
 * @param lablChunksCount - The number of LablChunk structures in the lablChunks
 * array.
 * @return A pointer to a WaveFile object constructed from the provided chunk
 * data.
 */
WaveFile* makeWaveFile(WaveHeader* waveHeader, FormatChunk* formatChunk,
    DataChunk* dataChunk, CueChunk* cueChunk,
    LtxtChunk* ltxtChunks, uint32_t ltxtChunksCount,
    LablChunk* lablChunks, uint32_t lablChunksCount);

/**
 * Creates a new WaveFile structure from raw audio data and associated metadata.
 * This function allows for the construction of a WaveFile object using raw
 * waveform data, which can be useful for creating new WAV files from audio data
 * generated or processed within the application.
 *
 * @param waveformData - A pointer to the raw audio data to be included in the
 * WAV file.
 * @param chunkDataSize - The size of the waveform data in bytes.
 * @param numberOfChannels - The number of audio channels (e.g., 1 for mono, 2
 * for stereo).
 * @param sampleRate - The sample rate of the audio data (e.g., 44100 Hz).
 * @param significantBitsPerSample - The bit depth of the audio data (e.g., 16
 * bits).
 * @param pointsCount - The number of cue points to be included in the WAV file.
 * @param pointsOffset - An array of byte offsets for each cue point within the
 * audio data.
 * @param pointsLength - An array of byte lengths for each cue point within the
 * audio data.
 * @param pointsLabels - An array of text labels for each cue point.
 * @return A pointer to a WaveFile object constructed from the provided raw
 * audio data and metadata.
 */
WaveFile* makeWaveFileFromRawData(char* waveformData, uint32_t chunkDataSize,
    uint16_t numberOfChannels,
    uint32_t sampleRate,
    uint16_t significantBitsPerSample,
    uint32_t pointsCount, uint32_t* pointsOffset,
    uint32_t* pointsLenght, char** pointsLabels);

/**
 * Creates a new WaveFile structure with the specified parameters. This function
 * allows for the construction of a WaveFile object using basic audio parameters,
 * which can be useful for creating new WAV files from scratch or for testing
 * purposes.
 *
 * @param uint32 - A placeholder parameter (not used in the function).
 * @return A pointer to a WaveFile object constructed with the specified
 * parameters.
 */
WaveHeader* makeWaveHeader(uint32_t uint32);

/**
 * Creates a new FormatChunk structure with the specified audio format parameters.
 * This function allows for the construction of a FormatChunk object using basic
 * audio format information, which can be useful for creating new WAV files or
 * modifying existing ones.
 *
 * @param numberOfChannels - The number of audio channels (e.g., 1 for mono, 2
 * for stereo).
 * @param sampleRate - The sample rate of the audio data (e.g., 44100 Hz).
 * @param significantBitsPerSample - The bit depth of the audio data (e.g., 16
 * bits).
 * @return A pointer to a FormatChunk object constructed with the specified audio
 * format parameters.
 */
FormatChunk* makeFormatChunk(uint16_t numberOfChannels, uint32_t sampleRate,
    uint16_t significantBitsPerSample);

/**
 * Creates a new DataChunk structure with the specified audio data and metadata.
 * This function allows for the construction of a DataChunk object using raw
 * audio data, which can be useful for creating new WAV files from audio data
 * generated or processed within the application.
 *
 * @param chunkDataSize - The size of the waveform data in bytes.
 * @param waveformData - A pointer to the raw audio data to be included in the
 * WAV file.
 * @return A pointer to a DataChunk object constructed with the specified audio
 * data and metadata.
 */
DataChunk* makeDataChunk(uint32_t chunkDataSize, char* waveformData);

/**
 * Creates a new CueChunk structure with the specified cue points. This function
 * allows for the construction of a CueChunk object using an array of cue points,
 * which can be useful for adding markers to specific points in the audio data.
 *
 * @param cuePointsCount - The number of cue points to be included in the CueChunk.
 * @param cuePoints - An array of CuePoint structures representing the cue points
 * to be included in the CueChunk.
 * @return A pointer to a CueChunk object constructed with the specified cue
 * points.
 */
CueChunk* makeCueChunk(uint32_t cuePointsCount, CuePoint* cuePoints);

/**
 * Creates a new CuePoint structure with the specified parameters. This function
 * allows for the construction of a CuePoint object using basic cue point
 * information, which can be useful for adding markers to specific points in the
 * audio data.
 *
 * @param cuePointID - A unique ID for the Cue Point.
 * @param playOrderPosition - The sample number at which this cue point will occur
 * during playback of the entire play list (if a Playlist chunk is present).
 * @param chunkStart - The byte offset into the Wave List Chunk of the chunk
 * containing the sample that corresponds to this cue point.
 * @param blockStart - The byte offset into the "data" or "slnt" Chunk to the
 * start of the block containing the sample that corresponds to this cue point.
 * @param frameOffset - The offset into the block (specified by Block Start) for
 * the sample that corresponds to the cue point.
 * @return A pointer to a CuePoint object constructed with the specified
 * parameters.
 */
CuePoint* makeCuePoint(uint32_t cuePointID, uint32_t playOrderPosition,
    uint32_t chunkStart, uint32_t blockStart,
    uint32_t frameOffset);

/**
 * Creates a new ListChunk structure with the specified LtxtChunk and LablChunk
 * data. This function allows for the construction of a ListChunk object using
 * arrays of LtxtChunk and LablChunk structures, which can be useful for adding
 * labeled cue points and their associated text to a WAV file.
 *
 * @param ltxtCount - The number of LtxtChunk structures to be included in the
 * ListChunk.
 * @param ltxtChunks - An array of LtxtChunk structures representing the labeled
 * text cue points to be included in the ListChunk.
 * @param lablCount - The number of LablChunk structures to be included in the
 * ListChunk.
 * @param lablChunks - An array of LablChunk structures representing the labeled
 * cue points to be included in the ListChunk.
 * @return A pointer to a ListChunk object constructed with the specified
 * LtxtChunk and LablChunk data.
 */
ListChunk* makeListChunk(uint32_t ltxtCount, LtxtChunk* ltxtChunks,
    uint32_t lablCount, LablChunk* lablChunks);

/**
 * Creates a new LtxtChunk structure with the specified parameters. This function
 * allows for the construction of an LtxtChunk object using basic labeled text
 * cue point information, which can be useful for adding detailed annotations to
 * specific points in the audio data.
 *
 * @param cuePointID - A unique ID for the Cue Point.
 * @param sampleLength - The length of the sample associated with the cue point in bytes.
 * @param country - The country code for localization (e.g., 0 for unspecified).
 * @param language - The language code for localization (e.g., 0 for unspecified).
 * @param dialect - The dialect code for localization (e.g., 0 for unspecified).
 * @param codePage - The code page for localization (e.g., 0 for unspecified).
 * @param text - A pointer to a string containing the text label associated with
 * the cue point.
 * @return A pointer to an LtxtChunk object constructed with the specified
 * parameters.
 */
LtxtChunk* makeLtxtChunk(uint32_t cuePointID, uint32_t sampleLength,
    uint16_t country, uint16_t language, uint16_t dialect,
    uint16_t codePage, char* text);

/**
 * Creates a new LablChunk structure with the specified parameters. This function
 * allows for the construction of a LablChunk object using basic labeled cue point
 * information, which can be useful for adding descriptive labels to specific points
 * in the audio data.
 *
 * @param cuePointID - A unique ID for the Cue Point.
 * @param text - A pointer to a string containing the text label associated with
 * the cue point.
 * @return A pointer to a LablChunk object constructed with the specified parameters.
 */
LablChunk* makeLablChunk(uint32_t cuePointID, char* text);

/**
 * Saves the given WaveFile object to a WAV file at the specified file path. This
 * function writes the contents of the WaveFile structure, including the various
 * chunks and subchunks, to the specified file path.
 *
 * @param waveFile - A pointer to the WaveFile object to be saved.
 * @param filePath - The path where the WAV file will be saved.
 */
void saveWaveFile(WaveFile* waveFile, const std::string& filePath);

/**
 * Closes the given WaveFile object and releases any associated resources. This
 * function ensures that all memory allocated for the WaveFile structure and its
 * components is properly freed, preventing memory leaks and ensuring efficient
 * resource management.
 *
 * @param waveFile - A pointer to the WaveFile object to be closed.
 */
void waveCloseFile(WaveFile*);

/**
 * Creates a deep copy of the given WaveFile object and returns a pointer to the
 * cloned object. This function allows for the creation of a new WaveFile object
 * that is identical to the original, including all of its chunks and associated
 * data, while ensuring that the memory for the cloned object is managed
 * independently from the original.
 *
 * @param waveFile - A pointer to the WaveFile object to be cloned.
 * @return A pointer to a new WaveFile object that is a deep copy of the original.
 */
WaveFile* waveCloneFile(WaveFile*);

// All data in a Wave file must be little endian.
// These are functions to convert 2- and 4-byte unsigned ints to and from little
// endian, if needed
enum HostEndiannessType { EndiannessUndefined = 0,
    LittleEndian,
    BigEndian };

static enum HostEndiannessType HostEndianness = EndiannessUndefined;

/**
 * Determines the endianness of the host system and returns it as a HostEndiannessType
 * enum value. This function checks the byte order of the system to determine whether
 * it is little endian or big endian, which is important for correctly interpreting
 * and manipulating data in WAV files, as all data in a WAV file must be in little
 * endian format.
 *
 * @return A HostEndiannessType enum value indicating the endianness of the host system.
 */
enum HostEndiannessType getHostEndianness();

/**
 * Converts a 4-byte array of little endian bytes to a uint32_t value. This function
 * is used to interpret data from WAV files, which are stored in little endian format,
 * and convert it to a standard uint32_t value that can be used in the application.
 *
 * @param littleEndianBytes - A 4-byte array containing the little endian bytes to be converted.
 * @return A uint32_t value representing the converted little endian bytes.
 */
uint32_t littleEndianBytesToUInt32(char littleEndianBytes[4]);

/**
 * Converts a uint32_t value to a 4-byte array of little endian bytes. This function
 * is used to prepare data for writing to WAV files, which require data to be in
 * little endian format, by converting standard uint32_t values to the appropriate
 * byte order.
 *
 * @param uInt32Value - The uint32_t value to be converted to little endian bytes.
 * @param out_LittleEndianBytes - A 4-byte array that will be filled with the little endian bytes representing the input value.
 */
void uint32ToLittleEndianBytes(uint32_t uInt32Value,
    char out_LittleEndianBytes[4]);

/**
 * Converts a 2-byte array of little endian bytes to a uint16_t value. This function
 * is used to interpret data from WAV files, which are stored in little endian format,
 * and convert it to a standard uint16_t value that can be used in the application.
 *
 * @param littleEndianBytes - A 2-byte array containing the little endian bytes to be converted.
 * @return A uint16_t value representing the converted little endian bytes.
 */
uint16_t littleEndianBytesToUInt16(char littleEndianBytes[2]);

/**
 * Converts a uint16_t value to a 2-byte array of little endian bytes. This function
 * is used to prepare data for writing to WAV files, which require data to be in
 * little endian format, by converting standard uint16_t values to the appropriate
 * byte order.
 *
 * @param uInt16Value - The uint16_t value to be converted to little endian bytes.
 * @param out_LittleEndianBytes - A 2-byte array that will be filled with the little endian bytes representing the input value.
 */
void uint16ToLittleEndianBytes(uint16_t uInt16Value,
    char out_LittleEndianBytes[2]);

/**
 * Converts raw waveform data from a WAV file into a vector of double values. This function
 * takes into account the byte size and bit depth of the audio data to correctly interpret
 * the raw bytes and produce a vector of normalized double values representing the audio
 * samples.
 *
 * @param data - A pointer to the raw audio data to be converted.
 * @param byteSize - The size of the raw audio data in bytes.
 * @param bitDepth - The bit depth of the audio data (e.g., 16 bits).
 * @return A std::vector<double> containing the converted and normalized audio samples.
 */
std::vector<double> waveformDataToVector(void* data, uint32_t byteSize,
    uint16_t bitDepth);

#endif // WAVFILE_H
