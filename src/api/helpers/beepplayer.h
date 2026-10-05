#ifndef BEEPPLAYER_H
#define BEEPPLAYER_H

#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QByteArray>
#include <cmath>

class BeepPlayer : public QObject
{
    Q_OBJECT
public:
    explicit BeepPlayer(QObject *parent = nullptr) : QObject(parent) {}

    // Plays a single beep, or a beep + second beep if freq2 > 0
    void playBeep(double freq1, int dur1, double amp,
                  double freq2 = 0.0, int dur2 = 0)
    {
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);

        QByteArray pcm = generateTone(freq1, dur1, amp, format);

        // append second beep (with a short gap) if requested
        if (freq2 > 0.0 && dur2 > 0) {
            pcm += generateSilence(30, format);           // 30ms gap
            pcm += generateTone(freq2, dur2, amp, format);
        }

        m_buffer.reset(new QBuffer(this));
        m_data = pcm;
        m_buffer->setBuffer(&m_data);
        m_buffer->open(QIODevice::ReadOnly);

        m_sink.reset(new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this));
        m_sink->start(m_buffer.get());

        // Notify when playback finishes
        disconnect(m_sink.get(), &QAudioSink::stateChanged, nullptr, nullptr);
        connect(m_sink.get(), &QAudioSink::stateChanged, this, [this](QAudio::State state) {
            if (state == QAudio::IdleState || state == QAudio::StoppedState) {
                emit finished();
            }
        });
    }

signals:
    void finished();

private:
    QByteArray generateTone(double freq, int durationMs, double amp,
                             const QAudioFormat &format)
    {
        const int sampleRate = format.sampleRate();
        const int sampleCount = sampleRate * durationMs / 1000;
        QByteArray data;
        data.resize(sampleCount * sizeof(qint16));
        auto *samples = reinterpret_cast<qint16*>(data.data());

        const double amplitude = qBound(0.0, amp, 1.0) * 32767.0;
        for (int i = 0; i < sampleCount; ++i) {
            double t = double(i) / sampleRate;
            samples[i] = static_cast<qint16>(amplitude * std::sin(2.0 * M_PI * freq * t));
        }
        return data;
    }

    QByteArray generateSilence(int durationMs, const QAudioFormat &format)
    {
        const int sampleCount = format.sampleRate() * durationMs / 1000;
        QByteArray data(sampleCount * sizeof(qint16), 0);
        return data;
    }

    QScopedPointer<QAudioSink> m_sink;
    QScopedPointer<QBuffer> m_buffer;
    QByteArray m_data; // must outlive playback
};

#endif // BEEPPLAYER_H
