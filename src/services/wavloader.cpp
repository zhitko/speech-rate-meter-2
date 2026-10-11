#include "wavloader.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {

constexpr int kAnalysisRate = 8000;
constexpr qint64 kMaxFileBytes = 256LL * 1024 * 1024;
constexpr int kMaxChannels = 64;
constexpr int kMaxSampleRate = 384000;

QString cannotOpen()
{
    return QCoreApplication::translate("SessionApi", "The selected file cannot be opened.");
}

QString notWav()
{
    return QCoreApplication::translate("SessionApi", "The selected file is not a valid WAV file.");
}

QString noSamples()
{
    return QCoreApplication::translate("SessionApi", "The selected file has no audio samples.");
}

QString tooLarge()
{
    return QCoreApplication::translate("SessionApi", "The selected file is too large to open.");
}

QString unsupported(int format, int rate, int channels, int bits)
{
    return QCoreApplication::translate("SessionApi", "This WAV encoding is not supported.")
        + QStringLiteral(" (%1 Hz, %2 ch, %3-bit, format %4).")
              .arg(rate)
              .arg(channels)
              .arg(bits)
              .arg(format);
}

WavLoadResult fail(const QString& error)
{
    WavLoadResult result;
    result.error = error;
    return result;
}

bool readU16(const QByteArray& bytes, quint64 at, quint16& value)
{
    if (at + 2 > static_cast<quint64>(bytes.size()))
        return false;
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.constData() + at);
    value = quint16(p[0]) | (quint16(p[1]) << 8);
    return true;
}

bool readU32(const QByteArray& bytes, quint64 at, quint32& value)
{
    if (at + 4 > static_cast<quint64>(bytes.size()))
        return false;
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.constData() + at);
    value = quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
    return true;
}

bool guidEquals(const QByteArray& bytes, quint64 at, const unsigned char expected[16])
{
    if (at + 16 > static_cast<quint64>(bytes.size()))
        return false;
    return std::memcmp(bytes.constData() + at, expected, 16) == 0;
}

enum class SampleKind { Pcm, Float };

double decodeSample(const unsigned char* sample, SampleKind kind, int bits)
{
    if (kind == SampleKind::Float) {
        double value = 0;
        if (bits == 32) {
            float stored = 0;
            std::memcpy(&stored, sample, sizeof(float));
            value = static_cast<double>(stored);
        } else if (bits == 64) {
            std::memcpy(&value, sample, sizeof(double));
        } else {
            return 0;
        }
        if (!std::isfinite(value))
            return 0;
        return value * 32767.0;
    }

    if (bits == 8)
        return (static_cast<int>(sample[0]) - 128) * 256.0;
    if (bits == 16) {
        const auto value = static_cast<qint16>(sample[0] | (sample[1] << 8));
        return static_cast<double>(value);
    }
    if (bits == 24) {
        std::int32_t value = std::int32_t(sample[0])
            | (std::int32_t(sample[1]) << 8)
            | (std::int32_t(sample[2]) << 16);
        if (value & 0x00800000)
            value |= ~0x00FFFFFF;
        return static_cast<double>(value) * (32767.0 / 8388607.0);
    }
    if (bits == 32) {
        const std::int32_t value = std::int32_t(quint32(sample[0]) | (quint32(sample[1]) << 8)
            | (quint32(sample[2]) << 16) | (quint32(sample[3]) << 24));
        return static_cast<double>(value) * (32767.0 / 2147483647.0);
    }
    return 0;
}

float clampSample(double value)
{
    if (!std::isfinite(value))
        return 0;
    if (value >= 32767.0)
        return 32767.f;
    if (value <= -32768.0)
        return -32768.f;
    return static_cast<float>(std::lround(value));
}

std::vector<float> resampleMono(const std::vector<double>& mono, int sourceRate)
{
    std::vector<float> output;
    if (mono.empty() || sourceRate <= 0)
        return output;

    if (sourceRate == kAnalysisRate) {
        output.reserve(mono.size());
        for (double sample : mono)
            output.push_back(clampSample(sample));
        return output;
    }

    const double outCountD = static_cast<double>(mono.size()) * static_cast<double>(kAnalysisRate)
        / static_cast<double>(sourceRate);
    if (!(outCountD > 0) || outCountD > static_cast<double>(std::numeric_limits<int>::max()))
        return output;
    int outCount = static_cast<int>(std::llround(outCountD));
    if (outCount < 1)
        outCount = 1;
    output.resize(static_cast<std::size_t>(outCount));
    if (mono.size() == 1) {
        const float held = clampSample(mono.front());
        for (float& sample : output)
            sample = held;
        return output;
    }

    const double step = static_cast<double>(sourceRate) / static_cast<double>(kAnalysisRate);
    const int last = static_cast<int>(mono.size()) - 1;
    for (int index = 0; index < outCount; ++index) {
        const double position = index * step;
        int left = static_cast<int>(position);
        if (left >= last) {
            output[static_cast<std::size_t>(index)] = clampSample(mono.back());
            continue;
        }
        if (left < 0)
            left = 0;
        const double fraction = position - left;
        const double mixed = mono[static_cast<std::size_t>(left)] * (1.0 - fraction)
            + mono[static_cast<std::size_t>(left + 1)] * fraction;
        output[static_cast<std::size_t>(index)] = clampSample(mixed);
    }
    return output;
}

} // namespace

WavLoadResult loadAnalysisWav(const QString& path)
{
    const QFileInfo info(path);
    if (path.isEmpty() || !info.exists() || !info.isFile() || !info.isReadable())
        return fail(cannotOpen());
    if (info.size() <= 0)
        return fail(notWav());
    if (info.size() > kMaxFileBytes)
        return fail(tooLarge());

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(cannotOpen());
    const QByteArray bytes = file.readAll();
    if (bytes.size() < 12)
        return fail(notWav());
    if (bytes.first(4) != "RIFF" || bytes.sliced(8, 4) != "WAVE")
        return fail(notWav());

    bool sawFormat = false;
    bool sawData = false;
    int audioFormat = 0;
    int channels = 0;
    int sampleRate = 0;
    int bits = 0;
    int blockAlign = 0;
    SampleKind kind = SampleKind::Pcm;
    QByteArray data;

    quint64 offset = 12;
    const quint64 fileSize = static_cast<quint64>(bytes.size());
    while (offset + 8 <= fileSize) {
        const QByteArray id = bytes.sliced(static_cast<qsizetype>(offset), 4);
        quint32 chunkSize = 0;
        if (!readU32(bytes, offset + 4, chunkSize))
            return fail(notWav());
        const quint64 body = offset + 8;
        const quint64 end = body + chunkSize;
        if (end > fileSize) {
            // A finished fmt+data pair is enough. Trailing bytes that claim a
            // chunk past EOF must not hide audio that was already read.
            if (sawFormat && sawData)
                break;
            return fail(notWav());
        }

        if (id == "fmt ") {
            if (chunkSize < 16)
                return fail(notWav());
            quint16 formatTag = 0;
            quint16 channelCount = 0;
            quint32 rate = 0;
            quint16 align = 0;
            quint16 depth = 0;
            if (!readU16(bytes, body, formatTag)
                || !readU16(bytes, body + 2, channelCount)
                || !readU32(bytes, body + 4, rate)
                || !readU16(bytes, body + 12, align)
                || !readU16(bytes, body + 14, depth)) {
                return fail(notWav());
            }
            audioFormat = formatTag;
            channels = channelCount;
            sampleRate = static_cast<int>(rate);
            blockAlign = align;
            bits = depth;
            kind = SampleKind::Pcm;
            if (formatTag == 0xFFFE) {
                if (chunkSize < 40)
                    return fail(notWav());
                static const unsigned char pcmGuid[16] = {
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
                };
                static const unsigned char floatGuid[16] = {
                    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
                };
                if (guidEquals(bytes, body + 24, pcmGuid))
                    kind = SampleKind::Pcm;
                else if (guidEquals(bytes, body + 24, floatGuid))
                    kind = SampleKind::Float;
                else
                    return fail(unsupported(audioFormat, sampleRate, channels, bits));
            } else if (formatTag == 3) {
                kind = SampleKind::Float;
            } else if (formatTag != 1) {
                return fail(unsupported(audioFormat, sampleRate, channels, bits));
            }
            sawFormat = true;
        } else if (id == "data") {
            if (!data.isEmpty())
                data.append(bytes.sliced(static_cast<qsizetype>(body), static_cast<qsizetype>(chunkSize)));
            else
                data = bytes.sliced(static_cast<qsizetype>(body), static_cast<qsizetype>(chunkSize));
            sawData = true;
        }

        offset = end;
        if ((chunkSize & 1U) && offset < fileSize)
            ++offset;
    }

    if (!sawFormat || !sawData)
        return fail(notWav());
    if (channels < 1 || channels > kMaxChannels || sampleRate < 1 || sampleRate > kMaxSampleRate || blockAlign < 1)
        return fail(notWav());

    const bool supportedPcm = kind == SampleKind::Pcm && (bits == 8 || bits == 16 || bits == 24 || bits == 32);
    const bool supportedFloat = kind == SampleKind::Float && (bits == 32 || bits == 64);
    if (!supportedPcm && !supportedFloat)
        return fail(unsupported(audioFormat, sampleRate, channels, bits));

    const int sampleBytes = bits / 8;
    if (sampleBytes < 1 || blockAlign < channels * sampleBytes)
        return fail(notWav());
    if (data.isEmpty())
        return fail(noSamples());

    const int frames = data.size() / blockAlign;
    if (frames <= 0)
        return fail(noSamples());

    const double outCountD = static_cast<double>(frames) * static_cast<double>(kAnalysisRate)
        / static_cast<double>(sampleRate);
    if (outCountD > 200000000.0)
        return fail(tooLarge());

    std::vector<double> mono;
    mono.reserve(static_cast<std::size_t>(frames));
    const auto* raw = reinterpret_cast<const unsigned char*>(data.constData());
    for (int frame = 0; frame < frames; ++frame) {
        const unsigned char* framePtr = raw + frame * blockAlign;
        double sum = 0;
        for (int channel = 0; channel < channels; ++channel)
            sum += decodeSample(framePtr + channel * sampleBytes, kind, bits);
        mono.push_back(sum / static_cast<double>(channels));
    }

    WavLoadResult result;
    result.samples = resampleMono(mono, sampleRate);
    if (result.samples.empty())
        return fail(noSamples());
    result.ok = true;
    result.sampleRate = sampleRate;
    result.channelCount = channels;
    result.bitsPerSample = bits;
    return result;
}

bool writePcm16MonoWav(const QString& path, const std::vector<float>& samples, int sampleRate)
{
    if (path.isEmpty() || sampleRate <= 0 || samples.size() > 200000000)
        return false;

    const auto dataBytes = static_cast<quint32>(samples.size() * 2);
    const quint32 riffSize = 36 + dataBytes;
    QByteArray bytes;
    bytes.reserve(static_cast<qsizetype>(riffSize) + 8);

    const auto append = [&bytes](const char* text, int size) {
        bytes.append(text, size);
    };
    const auto appendU16 = [&bytes](quint16 value) {
        const char stored[2] = { char(value & 0xff), char((value >> 8) & 0xff) };
        bytes.append(stored, 2);
    };
    const auto appendU32 = [&bytes](quint32 value) {
        const char stored[4] = {
            char(value & 0xff),
            char((value >> 8) & 0xff),
            char((value >> 16) & 0xff),
            char((value >> 24) & 0xff)
        };
        bytes.append(stored, 4);
    };

    append("RIFF", 4);
    appendU32(riffSize);
    append("WAVE", 4);
    append("fmt ", 4);
    appendU32(16);
    appendU16(1);
    appendU16(1);
    appendU32(static_cast<quint32>(sampleRate));
    appendU32(static_cast<quint32>(sampleRate) * 2);
    appendU16(2);
    appendU16(16);
    append("data", 4);
    appendU32(dataBytes);
    for (float sample : samples) {
        double value = static_cast<double>(sample);
        if (!std::isfinite(value))
            value = 0;
        long rounded = std::lround(std::clamp(value, -32768.0, 32767.0));
        appendU16(static_cast<quint16>(static_cast<qint16>(rounded)));
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(bytes) == bytes.size();
}
