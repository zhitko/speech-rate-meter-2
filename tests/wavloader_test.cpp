#include "src/services/wavloader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>
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

void expectMsg(bool condition, const QString& message)
{
    const QByteArray utf8 = message.toUtf8();
    expect(condition, utf8.constData());
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

void appendChunk(QByteArray& bytes, const char* id, const QByteArray& payload)
{
    bytes.append(id, 4);
    appendU32(bytes, static_cast<quint32>(payload.size()));
    bytes.append(payload);
    if (payload.size() & 1)
        bytes.append('\0');
}

QByteArray formatChunk(quint16 format, quint16 channels, quint32 rate, quint16 bits, const QByteArray& extra = {})
{
    QByteArray body;
    const quint16 blockAlign = static_cast<quint16>(channels * (bits / 8));
    appendU16(body, format);
    appendU16(body, channels);
    appendU32(body, rate);
    appendU32(body, rate * blockAlign);
    appendU16(body, blockAlign);
    appendU16(body, bits);
    body.append(extra);
    return body;
}

QByteArray pcm16(const std::vector<qint16>& samples)
{
    QByteArray data;
    data.reserve(static_cast<qsizetype>(samples.size() * 2));
    for (qint16 sample : samples)
        appendU16(data, static_cast<quint16>(sample));
    return data;
}

QByteArray wrapWave(const QByteArray& body)
{
    QByteArray file;
    file.append("RIFF", 4);
    appendU32(file, static_cast<quint32>(4 + body.size()));
    file.append("WAVE", 4);
    file.append(body);
    return file;
}

QString writeFile(const QDir& dir, const QString& name, const QByteArray& bytes)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly), "open fixture");
    expect(file.write(bytes) == bytes.size(), "write fixture");
    file.close();
    return path;
}

void expectCount(const WavLoadResult& loaded, int count, const char* message)
{
    expect(loaded.ok, message);
    expect(static_cast<int>(loaded.samples.size()) == count, message);
}

void expectNear(float actual, float expected, float tolerance, const char* message)
{
    expect(std::fabs(actual - expected) <= tolerance, message);
}

void testExactPcm16()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const std::vector<qint16> samples = { 0, 1000, -1000, 32767, -32768 };
    QByteArray body;
    appendChunk(body, "fmt ", formatChunk(1, 1, 8000, 16));
    appendChunk(body, "data", pcm16(samples));
    const QString path = writeFile(QDir(dir.path()), QStringLiteral("exact.wav"), wrapWave(body));

    const WavLoadResult loaded = loadAnalysisWav(path);
    expectCount(loaded, static_cast<int>(samples.size()), "8 kHz mono s16 count");
    for (std::size_t index = 0; index < samples.size() && index < loaded.samples.size(); ++index)
        expect(loaded.samples[index] == static_cast<float>(samples[index]), "8 kHz mono s16 value");
    expect(loaded.error.isEmpty(), "8 kHz mono s16 has no error");
}

void testRatesAndStereo()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");

    const auto constant = [](int count, qint16 value) {
        return std::vector<qint16>(static_cast<std::size_t>(count), value);
    };
    const int rates[] = { 16000, 22050, 44100, 48000 };
    for (int rate : rates) {
        QByteArray body;
        appendChunk(body, "fmt ", formatChunk(1, 1, static_cast<quint32>(rate), 16));
        appendChunk(body, "data", pcm16(constant(rate, 10000)));
        const QString path = writeFile(QDir(dir.path()),
            QStringLiteral("rate-%1.wav").arg(rate),
            wrapWave(body));
        const WavLoadResult loaded = loadAnalysisWav(path);
        expectCount(loaded, 8000, "resampled length is one second");
        expect(loaded.sampleRate == rate, "source rate is kept on the result");
        if (!loaded.samples.empty()) {
            expectNear(loaded.samples.front(), 10000.f, 0.f, "constant survives resample");
            expectNear(loaded.samples.back(), 10000.f, 0.f, "constant survives resample tail");
        }
    }

    std::vector<qint16> stereo = { 1000, 3000, -2000, -4000, 0, 500 };
    QByteArray body;
    appendChunk(body, "fmt ", formatChunk(1, 2, 8000, 16));
    appendChunk(body, "data", pcm16(stereo));
    const WavLoadResult loaded = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("stereo.wav"), wrapWave(body)));
    expectCount(loaded, 3, "stereo downmix length");
    expect(loaded.channelCount == 2, "stereo channel count");
    if (loaded.samples.size() == 3) {
        expectNear(loaded.samples[0], 2000.f, 0.f, "stereo downmix first");
        expectNear(loaded.samples[1], -3000.f, 0.f, "stereo downmix second");
        expectNear(loaded.samples[2], 250.f, 0.f, "stereo downmix third");
    }
}

void testHeldResample()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    std::vector<qint16> samples;
    samples.reserve(48000);
    for (int index = 0; index < 8000; ++index) {
        const auto value = static_cast<qint16>(index * 3 - 100);
        for (int repeat = 0; repeat < 6; ++repeat)
            samples.push_back(value);
    }
    QByteArray body;
    appendChunk(body, "fmt ", formatChunk(1, 1, 48000, 16));
    appendChunk(body, "data", pcm16(samples));
    const WavLoadResult loaded = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("held.wav"), wrapWave(body)));
    expectCount(loaded, 8000, "48 kHz held pattern length");
    if (loaded.samples.size() == 8000) {
        expectNear(loaded.samples[0], -100.f, 0.f, "held pattern start");
        expectNear(loaded.samples[100], 200.f, 0.f, "held pattern middle");
        expectNear(loaded.samples[7999], 23897.f, 0.f, "held pattern end");
    }
}

void testWideSamples()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");

    QByteArray pcm24;
    const std::int32_t wide = 2560078;
    const unsigned char packed[3] = {
        static_cast<unsigned char>(wide & 0xff),
        static_cast<unsigned char>((wide >> 8) & 0xff),
        static_cast<unsigned char>((wide >> 16) & 0xff)
    };
    pcm24.append(reinterpret_cast<const char*>(packed), 3);
    QByteArray body24;
    appendChunk(body24, "fmt ", formatChunk(1, 1, 8000, 24));
    appendChunk(body24, "data", pcm24);
    const WavLoadResult loaded24 = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("wide24.wav"), wrapWave(body24)));
    expectCount(loaded24, 1, "24-bit sample count");
    if (!loaded24.samples.empty())
        expectNear(loaded24.samples[0], 10000.f, 1.f, "24-bit scales into int16");

    QByteArray pcm32;
    appendU32(pcm32, 655360000u);
    QByteArray body32;
    appendChunk(body32, "fmt ", formatChunk(1, 1, 8000, 32));
    appendChunk(body32, "data", pcm32);
    const WavLoadResult loaded32 = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("wide32.wav"), wrapWave(body32)));
    expectCount(loaded32, 1, "32-bit PCM sample count");
    if (!loaded32.samples.empty())
        expectNear(loaded32.samples[0], 10000.f, 1.f, "32-bit PCM scales into int16");

    QByteArray pcmFloat;
    const float unit = 10000.f / 32767.f;
    pcmFloat.append(reinterpret_cast<const char*>(&unit), sizeof(unit));
    QByteArray bodyFloat;
    appendChunk(bodyFloat, "fmt ", formatChunk(3, 1, 8000, 32));
    appendChunk(bodyFloat, "data", pcmFloat);
    const WavLoadResult loadedFloat = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("float.wav"), wrapWave(bodyFloat)));
    expectCount(loadedFloat, 1, "float sample count");
    if (!loadedFloat.samples.empty())
        expectNear(loadedFloat.samples[0], 10000.f, 1.f, "float scales into int16");
    expect(loadedFloat.bitsPerSample == 32, "float bit depth");
}

void testExtraChunksAndOrder()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const std::vector<qint16> samples = { 111, -222, 333, -444 };

    QByteArray list;
    list.append("INFO", 4);
    QByteArray name;
    name.append("test");
    name.append('\0');
    appendChunk(list, "INAM", name);

    QByteArray bext("bext!");
    QByteArray fact;
    appendU32(fact, static_cast<quint32>(samples.size()));

    QByteArray body;
    appendChunk(body, "bext", bext);
    appendChunk(body, "LIST", list);
    appendChunk(body, "fmt ", formatChunk(1, 1, 8000, 16));
    appendChunk(body, "fact", fact);
    appendChunk(body, "data", pcm16(samples));
    const WavLoadResult listed = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("chunks.wav"), wrapWave(body)));
    expectCount(listed, static_cast<int>(samples.size()), "LIST and bext before data");
    if (listed.samples.size() == samples.size())
        expect(listed.samples[1] == -222.f, "sample after extra chunks");

    QByteArray reversed;
    appendChunk(reversed, "data", pcm16(samples));
    appendChunk(reversed, "fmt ", formatChunk(1, 1, 8000, 16));
    const WavLoadResult swapped = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("data-first.wav"), wrapWave(reversed)));
    expectCount(swapped, static_cast<int>(samples.size()), "data chunk before fmt");
    if (swapped.samples.size() == samples.size())
        expect(swapped.samples[0] == 111.f, "data-first first sample");

    QByteArray extensible = formatChunk(0xFFFE, 1, 8000, 16, {});
    appendU16(extensible, 22);
    appendU16(extensible, 16);
    appendU32(extensible, 0x4);
    const unsigned char pcmGuid[16] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
    };
    extensible.append(reinterpret_cast<const char*>(pcmGuid), 16);
    QByteArray extBody;
    appendChunk(extBody, "JUNK", QByteArray("pad"));
    appendChunk(extBody, "fmt ", extensible);
    appendChunk(extBody, "data", pcm16(samples));
    const WavLoadResult ext = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("extensible.wav"), wrapWave(extBody)));
    expectCount(ext, static_cast<int>(samples.size()), "extensible PCM");
    if (ext.samples.size() == samples.size())
        expect(ext.samples[2] == 333.f, "extensible sample");
}

void testCyrillicPath()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const std::vector<qint16> samples = { 42, -7 };
    QByteArray body;
    appendChunk(body, "fmt ", formatChunk(1, 1, 8000, 16));
    appendChunk(body, "data", pcm16(samples));
    const QString name = QString::fromUtf8("\xd1\x82\xd0\xb5\xd1\x81\xd1\x82-\xd1\x80\xd0\xb5\xd1\x87\xd1\x8c.wav");
    const WavLoadResult loaded = loadAnalysisWav(writeFile(QDir(dir.path()), name, wrapWave(body)));
    expectCount(loaded, 2, "Cyrillic path loads");
    if (loaded.samples.size() == 2)
        expect(loaded.samples[0] == 42.f, "Cyrillic path sample");
}

void testFailures()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");

    const WavLoadResult missing = loadAnalysisWav(dir.filePath(QStringLiteral("missing.wav")));
    expect(!missing.ok && missing.samples.empty(), "missing file fails");
    expect(missing.error.contains(QStringLiteral("cannot be opened")), "missing file error");

    const WavLoadResult junk = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("notes.txt"), "hello"));
    expect(!junk.ok && junk.samples.empty(), "text file fails");
    expect(junk.error.contains(QStringLiteral("not a valid WAV")), "text file error");

    QByteArray truncated;
    truncated.append("RIFF", 4);
    appendU32(truncated, 1000);
    truncated.append("WAVE", 4);
    appendChunk(truncated, "fmt ", formatChunk(1, 1, 8000, 16));
    truncated.append("data", 4);
    appendU32(truncated, 50);
    truncated.append("xx", 2);
    QByteArray withJunk = wrapWave(QByteArray());
    {
        QByteArray body;
        appendChunk(body, "fmt ", formatChunk(1, 1, 8000, 16));
        appendChunk(body, "data", pcm16({ 7, 8 }));
        withJunk = wrapWave(body);
        withJunk.append("junk", 4);
        appendU32(withJunk, 100000);
    }
    const WavLoadResult trailing = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("trailing.wav"), withJunk));
    expectCount(trailing, 2, "trailing overrun chunk is ignored");
    if (trailing.samples.size() == 2)
        expect(trailing.samples[0] == 7.f, "sample before trailing junk");

    const WavLoadResult cut = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("cut.wav"), truncated));
    expect(!cut.ok && cut.samples.empty(), "truncated file fails");
    expect(cut.error.contains(QStringLiteral("not a valid WAV")), "truncated file error");

    QByteArray adpcmBody;
    appendChunk(adpcmBody, "fmt ", formatChunk(0x11, 1, 8000, 4));
    appendChunk(adpcmBody, "data", QByteArray(16, '\x11'));
    const WavLoadResult adpcm = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("adpcm.wav"), wrapWave(adpcmBody)));
    expect(!adpcm.ok && adpcm.samples.empty(), "ADPCM fails");
    expect(adpcm.error.contains(QStringLiteral("not supported")), "ADPCM error");

    QByteArray emptyBody;
    appendChunk(emptyBody, "fmt ", formatChunk(1, 1, 8000, 16));
    appendChunk(emptyBody, "data", QByteArray());
    const WavLoadResult empty = loadAnalysisWav(writeFile(QDir(dir.path()), QStringLiteral("empty.wav"), wrapWave(emptyBody)));
    expect(!empty.ok && empty.samples.empty(), "empty data fails");
    expect(empty.error.contains(QStringLiteral("no audio samples")), "empty data error");
}

void testPlaybackRoundTrip()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temp dir");
    const std::vector<float> samples = { 1.f, -2.f, 32767.f, -32768.f, 10000.4f };
    const QString path = dir.filePath(QStringLiteral("playback.wav"));
    expect(writePcm16MonoWav(path, samples, 8000), "write playback wav");

    QFile file(path);
    expect(file.open(QIODevice::ReadOnly), "read playback wav");
    const QByteArray bytes = file.readAll();
    expect(bytes.size() >= 44, "playback header size");
    expect(bytes.first(4) == "RIFF" && bytes.sliced(8, 4) == "WAVE", "playback is RIFF WAVE");
    expect(bytes.sliced(20, 2) == QByteArray("\x01\x00", 2), "playback is PCM");
    expect(bytes.sliced(22, 2) == QByteArray("\x01\x00", 2), "playback is mono");
    expect(bytes.sliced(24, 4) == QByteArray("\x40\x1f\x00\x00", 4), "playback is 8000 Hz");
    expect(bytes.sliced(34, 2) == QByteArray("\x10\x00", 2), "playback is 16-bit");

    const WavLoadResult loaded = loadAnalysisWav(path);
    expectCount(loaded, static_cast<int>(samples.size()), "playback round trip count");
    const float expected[] = { 1.f, -2.f, 32767.f, -32768.f, 10000.f };
    for (int index = 0; index < 5 && index < static_cast<int>(loaded.samples.size()); ++index)
        expect(loaded.samples[static_cast<std::size_t>(index)] == expected[index], "playback round trip value");
}

QString findBundledTestsDir()
{
    QStringList roots;
#ifdef SRM_SOURCE_DIR
    roots << QString::fromUtf8(SRM_SOURCE_DIR);
#endif
    roots << QDir::currentPath();
    roots << QCoreApplication::applicationDirPath();

    for (const QString& root : roots) {
        QDir dir(root);
        for (int depth = 0; depth < 7; ++depth) {
            const QString candidate = dir.filePath(QStringLiteral("data/tests"));
            if (QDir(candidate).exists())
                return QDir(candidate).absolutePath();
            if (!dir.cdUp())
                break;
        }
    }
    return {};
}

void testBundledRecordings()
{
    const QString dirPath = findBundledTestsDir();
    QDir dir(dirPath);
    const QStringList names = dirPath.isEmpty()
        ? QStringList()
        : dir.entryList({ QStringLiteral("*.wav"), QStringLiteral("*.WAV") }, QDir::Files, QDir::Name);
    if (names.isEmpty()) {
        std::cout << "data/tests absent, skipped\n";
        return;
    }

    QTemporaryDir playback;
    expect(playback.isValid(), "bundled playback temp dir");
    if (!playback.isValid())
        return;

    int loadedCount = 0;
    for (const QString& name : names) {
        const WavLoadResult loaded = loadAnalysisWav(dir.filePath(name));
        expectMsg(loaded.ok, name + QStringLiteral(" loads"));
        expectMsg(!loaded.samples.empty(), name + QStringLiteral(" has samples"));
        expectMsg(loaded.sampleRate > 0 && loaded.channelCount > 0 && loaded.bitsPerSample > 0,
                  name + QStringLiteral(" reports format"));
        if (!loaded.ok || loaded.samples.empty())
            continue;

        const QString out = playback.filePath(QStringLiteral("playback-%1.wav").arg(loadedCount));
        expectMsg(writePcm16MonoWav(out, loaded.samples, 8000), name + QStringLiteral(" writes 8 kHz playback"));
        const WavLoadResult again = loadAnalysisWav(out);
        expectMsg(again.ok && again.samples.size() == loaded.samples.size() && again.sampleRate == 8000,
                  name + QStringLiteral(" playback reloads at 8 kHz"));
        ++loadedCount;
    }

    expect(loadedCount == names.size(), "every bundled wav loaded");
    std::cout << "loaded " << loadedCount << " bundled test files from " << dirPath.toUtf8().constData() << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testExactPcm16();
    testRatesAndStereo();
    testHeldResample();
    testWideSamples();
    testExtraChunksAndOrder();
    testCyrillicPath();
    testFailures();
    testPlaybackRoundTrip();
    testBundledRecordings();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "wav loader tests passed\n";
    return EXIT_SUCCESS;
}
