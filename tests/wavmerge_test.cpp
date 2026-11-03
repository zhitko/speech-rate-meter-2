#include "src/services/wavloader.h"
#include "src/services/wavmerge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void appendU16(QByteArray& bytes, quint16 value)
{
    const char stored[2] = { char(value & 0xff), char((value >> 8) & 0xff) };
    bytes.append(stored, 2);
}

void appendU32(QByteArray& bytes, quint32 value)
{
    const char stored[4] = {
        char(value & 0xff),
        char((value >> 8) & 0xff),
        char((value >> 16) & 0xff),
        char((value >> 24) & 0xff)
    };
    bytes.append(stored, 4);
}

QByteArray pcm16(const std::vector<qint16>& samples)
{
    QByteArray data;
    data.reserve(static_cast<qsizetype>(samples.size() * 2));
    for (qint16 sample : samples)
        appendU16(data, static_cast<quint16>(sample));
    return data;
}

QString writeBytes(const QDir& dir, const QString& name, const QByteArray& bytes)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly), "open fixture");
    expect(file.write(bytes) == bytes.size(), "write fixture");
    file.close();
    return path;
}

bool sameSamples(const std::vector<float>& actual, const std::vector<qint16>& expected)
{
    if (actual.size() != expected.size())
        return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (actual[index] != static_cast<float>(expected[index]))
            return false;
    }
    return true;
}

void expectHeader(const QString& path, quint32 dataBytes)
{
    QFile file(path);
    expect(file.open(QIODevice::ReadOnly), "open merged header");
    const QByteArray bytes = file.read(44);
    expect(bytes.size() == 44, "merged header is 44 bytes");
    if (bytes.size() != 44)
        return;
    expect(bytes.first(4) == "RIFF" && bytes.sliced(8, 4) == "WAVE", "merged file is RIFF WAVE");
    const auto u32 = [](const QByteArray& data, int offset) {
        const auto* raw = reinterpret_cast<const unsigned char*>(data.constData() + offset);
        return quint32(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
    };
    expect(u32(bytes, 4) == 36 + dataBytes, "merged RIFF size covers one header and the PCM");
    expect(bytes.sliced(12, 4) == "fmt ", "merged format chunk");
    expect(u32(bytes, 16) == 16, "merged format chunk size");
    expect(bytes.sliced(20, 2) == QByteArray("\x01\x00", 2), "merged file is PCM");
    expect(bytes.sliced(22, 2) == QByteArray("\x01\x00", 2), "merged file is mono");
    expect(bytes.sliced(24, 4) == QByteArray("\x40\x1f\x00\x00", 4), "merged file is 8000 Hz");
    expect(bytes.sliced(34, 2) == QByteArray("\x10\x00", 2), "merged file is 16-bit");
    expect(bytes.sliced(36, 4) == "data", "merged data chunk");
    expect(u32(bytes, 40) == dataBytes, "merged data size is the sum of the chunks");
}

void testConcatenate()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const std::vector<qint16> first { 1, -2, 300, 32767, -32768 };
    const std::vector<qint16> second { 4, -5 };
    const std::vector<qint16> third { 9 };

    QByteArray listed;
    listed.append("fmt ", 4);
    appendU32(listed, 16);
    appendU16(listed, 1);
    appendU16(listed, 1);
    appendU32(listed, 8000);
    appendU32(listed, 16000);
    appendU16(listed, 2);
    appendU16(listed, 16);
    listed.append("LIST", 4);
    appendU32(listed, 4);
    listed.append("INFO", 4);
    listed.append("data", 4);
    const QByteArray thirdPcm = pcm16(third);
    appendU32(listed, static_cast<quint32>(thirdPcm.size()));
    listed.append(thirdPcm);
    QByteArray wrapped;
    wrapped.append("RIFF", 4);
    appendU32(wrapped, static_cast<quint32>(4 + listed.size()));
    wrapped.append("WAVE", 4);
    wrapped.append(listed);

    const QString firstPath = dir.filePath(QStringLiteral("a.wav"));
    const QString secondPath = dir.filePath(QStringLiteral("b.wav"));
    std::vector<float> firstFloat(first.begin(), first.end());
    std::vector<float> secondFloat(second.begin(), second.end());
    expect(writePcm16MonoWav(firstPath, firstFloat, 8000), "write first chunk");
    expect(writePcm16MonoWav(secondPath, secondFloat, 8000), "write second chunk");
    const QString thirdPath = writeBytes(QDir(dir.path()), QStringLiteral("c.wav"), wrapped);

    const QString out = dir.filePath(QStringLiteral("merged.wav"));
    const WavMergeResult merged = mergePcm16MonoWavs({ firstPath, secondPath, thirdPath }, out);
    expect(merged.ok, "three chunks merge");
    expect(QFileInfo::exists(firstPath) && QFileInfo::exists(secondPath) && QFileInfo::exists(thirdPath),
        "merge leaves the chunk files");

    std::vector<qint16> expected = first;
    expected.insert(expected.end(), second.begin(), second.end());
    expected.insert(expected.end(), third.begin(), third.end());
    const quint32 dataBytes = static_cast<quint32>(expected.size() * 2);
    expectHeader(merged.path, dataBytes);
    const WavLoadResult loaded = loadAnalysisWav(merged.path);
    expect(loaded.ok, "merged wav loads");
    expect(sameSamples(loaded.samples, expected), "merged samples are the chunks in order");
}

void testEdges()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const QString out = dir.filePath(QStringLiteral("out.wav"));

    const WavMergeResult none = mergePcm16MonoWavs({}, out);
    expect(none.ok && none.path.isEmpty(), "zero chunks succeed without a file");
    expect(!QFileInfo::exists(out), "zero chunks do not create a file");

    const std::vector<qint16> only { 11, -22, 33 };
    const QString onePath = dir.filePath(QStringLiteral("one.wav"));
    expect(writePcm16MonoWav(onePath, std::vector<float>(only.begin(), only.end()), 8000), "write one chunk");
    const WavMergeResult one = mergePcm16MonoWavs({ onePath }, out);
    expect(one.ok, "one chunk merges");
    expectHeader(one.path, static_cast<quint32>(only.size() * 2));
    const WavLoadResult loaded = loadAnalysisWav(one.path);
    expect(loaded.ok && sameSamples(loaded.samples, only), "one chunk keeps its samples");
    expect(QFileInfo::exists(onePath), "one chunk is left in place");

    const QString otherRate = dir.filePath(QStringLiteral("fast.wav"));
    expect(writePcm16MonoWav(otherRate, { 1.f }, 16000), "write 16000 Hz chunk");
    const QString badOut = dir.filePath(QStringLiteral("bad.wav"));
    const WavMergeResult mismatch = mergePcm16MonoWavs({ onePath, otherRate }, badOut);
    expect(!mismatch.ok && mismatch.path.isEmpty(), "a different sample rate is rejected");
    expect(!QFileInfo::exists(badOut), "a rejected merge writes no file");
    expect(QFileInfo::exists(onePath) && QFileInfo::exists(otherRate), "a rejected merge leaves the chunks");

    const QString missingDir = dir.filePath(QStringLiteral("missing/out.wav"));
    const WavMergeResult writeError = mergePcm16MonoWavs({ onePath }, missingDir);
    expect(!writeError.ok, "a missing directory is a write error");
    expect(!QFileInfo::exists(missingDir), "a write error leaves no output");
    expect(QFileInfo::exists(onePath), "a write error leaves the chunk");

    const QString garbage = writeBytes(QDir(dir.path()), QStringLiteral("garbage.wav"), QByteArray("RIFF"));
    const QString garbageOut = dir.filePath(QStringLiteral("garbage-out.wav"));
    const WavMergeResult broken = mergePcm16MonoWavs({ garbage }, garbageOut);
    expect(!broken.ok && !QFileInfo::exists(garbageOut), "a truncated header is rejected");
}

void testStreamedLength()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    constexpr int count = 200000;
    std::vector<float> first(static_cast<std::size_t>(count));
    std::vector<float> second(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        first[static_cast<std::size_t>(index)] = static_cast<float>((index * 3) % 1000 - 500);
        second[static_cast<std::size_t>(index)] = static_cast<float>(((index + 7) * 5) % 1000 - 500);
    }
    const QString firstPath = dir.filePath(QStringLiteral("long-a.wav"));
    const QString secondPath = dir.filePath(QStringLiteral("long-b.wav"));
    expect(writePcm16MonoWav(firstPath, first, 8000), "write long first chunk");
    expect(writePcm16MonoWav(secondPath, second, 8000), "write long second chunk");
    const QString out = dir.filePath(QStringLiteral("long.wav"));
    const WavMergeResult merged = mergePcm16MonoWavs({ firstPath, secondPath }, out);
    expect(merged.ok, "long chunks merge");
    expectHeader(merged.path, static_cast<quint32>(count * 2 * 2));

    const WavLoadResult loaded = loadAnalysisWav(merged.path);
    expect(loaded.ok, "long merge loads");
    expect(static_cast<int>(loaded.samples.size()) == count * 2, "long merge sample count");
    bool match = loaded.ok && static_cast<int>(loaded.samples.size()) == count * 2;
    if (match) {
        match = loaded.samples.front() == first.front()
            && loaded.samples[static_cast<std::size_t>(count - 1)] == first.back()
            && loaded.samples[static_cast<std::size_t>(count)] == second.front()
            && loaded.samples.back() == second.back();
        for (int index = 0; match && index < count; index += 1000) {
            if (loaded.samples[static_cast<std::size_t>(index)] != first[static_cast<std::size_t>(index)])
                match = false;
            if (loaded.samples[static_cast<std::size_t>(count + index)] != second[static_cast<std::size_t>(index)])
                match = false;
        }
    }
    expect(match, "long merge samples equal the concatenation");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testConcatenate();
    testEdges();
    testStreamedLength();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "wav merge tests passed\n";
    return 0;
}
