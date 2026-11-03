#include "wavmerge.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <vector>

namespace {

constexpr quint32 kSampleRate = 8000;
constexpr int kCopyBytes = 256 * 1024;

struct Span {
    qint64 offset = 0;
    quint32 bytes = 0;
};

bool readU16(QFile& file, quint16& value)
{
    unsigned char bytes[2];
    if (file.read(reinterpret_cast<char*>(bytes), 2) != 2)
        return false;
    value = static_cast<quint16>(bytes[0] | (bytes[1] << 8));
    return true;
}

bool readU32(QFile& file, quint32& value)
{
    unsigned char bytes[4];
    if (file.read(reinterpret_cast<char*>(bytes), 4) != 4)
        return false;
    value = static_cast<quint32>(bytes[0])
        | (static_cast<quint32>(bytes[1]) << 8)
        | (static_cast<quint32>(bytes[2]) << 16)
        | (static_cast<quint32>(bytes[3]) << 24);
    return true;
}

void appendU16(QByteArray& bytes, quint16 value)
{
    const char stored[2] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff)
    };
    bytes.append(stored, 2);
}

void appendU32(QByteArray& bytes, quint32 value)
{
    const char stored[4] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff)
    };
    bytes.append(stored, 4);
}

QString notWav(const QString& path)
{
    return QStringLiteral("Not a valid 8000 Hz mono 16-bit PCM WAV: ") + path;
}

bool inspectChunk(const QString& path, std::vector<Span>& spans, QString& error)
{
    spans.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Could not read ") + path;
        return false;
    }
    const QByteArray riff = file.read(12);
    if (riff.size() != 12 || riff.first(4) != "RIFF" || riff.sliced(8, 4) != "WAVE") {
        error = notWav(path);
        return false;
    }

    bool sawFormat = false;
    bool sawData = false;
    const qint64 fileSize = file.size();
    while (file.pos() + 8 <= fileSize) {
        const QByteArray id = file.read(4);
        quint32 chunkSize = 0;
        if (id.size() != 4 || !readU32(file, chunkSize)) {
            error = notWav(path);
            return false;
        }
        const qint64 body = file.pos();
        const qint64 end = body + static_cast<qint64>(chunkSize);
        if (end > fileSize) {
            // A finished fmt+data pair is enough. A trailing size that runs
            // past EOF must not hide audio that was already found.
            if (sawFormat && sawData)
                break;
            error = notWav(path);
            return false;
        }

        if (id == "fmt ") {
            if (chunkSize < 16) {
                error = notWav(path);
                return false;
            }
            quint16 format = 0;
            quint16 channels = 0;
            quint32 rate = 0;
            quint32 byteRate = 0;
            quint16 align = 0;
            quint16 bits = 0;
            if (!readU16(file, format) || !readU16(file, channels) || !readU32(file, rate)
                || !readU32(file, byteRate) || !readU16(file, align) || !readU16(file, bits)) {
                error = notWav(path);
                return false;
            }
            if (format != 1 || channels != 1 || rate != kSampleRate || bits != 16 || align != 2) {
                error = notWav(path);
                return false;
            }
            sawFormat = true;
        } else if (id == "data") {
            if ((chunkSize % 2U) != 0) {
                error = notWav(path);
                return false;
            }
            Span span;
            span.offset = body;
            span.bytes = chunkSize;
            spans.push_back(span);
            sawData = true;
        }

        qint64 next = end;
        if ((chunkSize & 1U) != 0 && next < fileSize)
            ++next;
        if (!file.seek(next)) {
            error = notWav(path);
            return false;
        }
    }

    if (!sawFormat || !sawData) {
        error = notWav(path);
        return false;
    }
    return true;
}

QByteArray pcmHeader(quint32 dataBytes)
{
    const quint32 riffSize = 36 + dataBytes;
    QByteArray bytes;
    bytes.reserve(44);
    bytes.append("RIFF", 4);
    appendU32(bytes, riffSize);
    bytes.append("WAVE", 4);
    bytes.append("fmt ", 4);
    appendU32(bytes, 16);
    appendU16(bytes, 1);
    appendU16(bytes, 1);
    appendU32(bytes, kSampleRate);
    appendU32(bytes, kSampleRate * 2);
    appendU16(bytes, 2);
    appendU16(bytes, 16);
    bytes.append("data", 4);
    appendU32(bytes, dataBytes);
    return bytes;
}

} // namespace

WavMergeResult mergePcm16MonoWavs(const QStringList& chunkPaths, const QString& outputPath)
{
    WavMergeResult result;
    if (chunkPaths.isEmpty()) {
        result.ok = true;
        return result;
    }
    if (outputPath.isEmpty()) {
        result.error = QStringLiteral("No output path for the merged recording.");
        return result;
    }

    std::vector<std::vector<Span>> files;
    files.reserve(static_cast<std::size_t>(chunkPaths.size()));
    quint64 total = 0;
    for (const QString& path : chunkPaths) {
        std::vector<Span> spans;
        if (!inspectChunk(path, spans, result.error))
            return result;
        for (const Span& span : spans) {
            total += span.bytes;
            if (total > (0xffffffffull - 36ull)) {
                result.error = QStringLiteral("The recording is too large to store in one WAV file.");
                return result;
            }
        }
        files.push_back(std::move(spans));
    }

    QSaveFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly)) {
        result.error = QStringLiteral("The merged recording could not be written.");
        return result;
    }

    const auto fail = [&](const QString& message) {
        output.cancelWriting();
        result.ok = false;
        result.path.clear();
        result.error = message;
        return result;
    };

    const auto header = pcmHeader(static_cast<quint32>(total));
    if (output.write(header) != header.size())
        return fail(QStringLiteral("The merged recording could not be written."));

    QByteArray buffer(kCopyBytes, Qt::Uninitialized);
    for (int index = 0; index < chunkPaths.size(); ++index) {
        QFile input(chunkPaths.at(index));
        if (!input.open(QIODevice::ReadOnly))
            return fail(QStringLiteral("Could not read ") + chunkPaths.at(index));
        for (const Span& span : files[static_cast<std::size_t>(index)]) {
            if (span.bytes == 0)
                continue;
            if (!input.seek(span.offset))
                return fail(notWav(chunkPaths.at(index)));
            quint32 left = span.bytes;
            while (left > 0) {
                const qint64 ask = qMin(static_cast<quint32>(buffer.size()), left);
                const qint64 got = input.read(buffer.data(), ask);
                if (got != ask)
                    return fail(notWav(chunkPaths.at(index)));
                if (output.write(buffer.constData(), got) != got)
                    return fail(QStringLiteral("The merged recording could not be written."));
                left -= static_cast<quint32>(got);
            }
        }
    }

    if (!output.commit())
        return fail(QStringLiteral("The merged recording could not be written."));

    result.ok = true;
    result.path = QFileInfo(outputPath).absoluteFilePath();
    result.error.clear();
    return result;
}
