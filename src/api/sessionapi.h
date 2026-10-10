#ifndef SESSIONAPI_H
#define SESSIONAPI_H

#include <QList>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <deque>
#include <memory>

class QThread;
class CaptureWorker;
class AnalysisThread;
class JobQueue;

/**
 * Process-wide recording session. Capture, pause cutting, and analysis run
 * off the UI thread. While recording, Home reads metrics of the most recent
 * analysis window from here; each pace drawn on the gauge shows the median of
 * the last few of its own readings. After Stop, speech rate, articulation, fillers, and
 * pauses become the duration-weighted mean of every kept phrase. Speech stays
 * the total. Home labels that state Mean values. After Stop, and after Open
 * File, a separate whole-recording summary counts vowel nuclei, phrasal
 * pauses, and vowel-duration statistics. That pass does not change the live
 * numbers.
 */
class SessionApi : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool sessionActive READ sessionActive NOTIFY sessionActiveChanged)
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(int phraseSeconds READ phraseSeconds NOTIFY phraseSecondsChanged)
    Q_PROPERTY(qreal audioLevel READ audioLevel NOTIFY audioLevelChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY hasResultChanged)
    Q_PROPERTY(bool showingMean READ showingMean NOTIFY showingMeanChanged)
    Q_PROPERTY(double speechRate READ speechRate NOTIFY metricsChanged)
    Q_PROPERTY(double articulationRate READ articulationRate NOTIFY metricsChanged)
    Q_PROPERTY(double gaugeArticulationRate READ gaugeArticulationRate NOTIFY metricsChanged)
    Q_PROPERTY(double phrasePauses READ phrasePauses NOTIFY metricsChanged)
    Q_PROPERTY(double speechDuration READ speechDuration NOTIFY metricsChanged)
    Q_PROPERTY(double fillerScore READ fillerScore NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap details READ details NOTIFY metricsChanged)
    Q_PROPERTY(bool hasRecordingSummary READ hasRecordingSummary NOTIFY recordingSummaryChanged)
    Q_PROPERTY(bool recordingSummaryPending READ recordingSummaryPending NOTIFY recordingSummaryChanged)
    Q_PROPERTY(QVariantMap recordingSummary READ recordingSummary NOTIFY recordingSummaryChanged)
    Q_PROPERTY(bool openFileAvailable READ openFileAvailable CONSTANT)
    Q_PROPERTY(bool openFileBusy READ openFileBusy NOTIFY openFileBusyChanged)
    Q_PROPERTY(QString openFileError READ openFileError NOTIFY openFileErrorChanged)
    Q_PROPERTY(bool clearingUserData READ clearingUserData NOTIFY clearingUserDataChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ openFileError NOTIFY openFileErrorChanged)
    Q_PROPERTY(QStringList listenFiles READ listenFiles NOTIFY listenFilesChanged)

public:
    enum Phase {
        IdleEmpty = 0,
        IdleReady = 1,
        Listening = 2,
        TooShort = 3,
        Measuring = 4,
        Dropped = 5,
        MicDenied = 6
    };
    Q_ENUM(Phase)

    explicit SessionApi(QObject* parent = nullptr);
    ~SessionApi() override;

    bool sessionActive() const { return m_sessionActive; }
    int phase() const { return m_phase == Measuring && m_noSpeech ? Listening : m_phase; }
    int phraseSeconds() const { return m_phraseSeconds; }
    qreal audioLevel() const { return m_audioLevel; }
    bool hasResult() const { return m_hasResult; }
    bool showingMean() const { return m_showingMean; }
    double speechRate() const { return m_speechRate; }
    double articulationRate() const { return m_articulationRate; }
    double gaugeArticulationRate() const { return m_gaugeArticulationRate; }
    double phrasePauses() const { return m_phrasePauses; }
    double speechDuration() const { return m_speechDuration; }
    double fillerScore() const { return m_fillerScore; }
    QVariantMap details() const { return m_details; }
    bool hasRecordingSummary() const { return m_hasRecordingSummary; }
    bool recordingSummaryPending() const { return m_recordingSummaryPending; }
    QVariantMap recordingSummary() const { return m_recordingSummary; }
    bool openFileAvailable() const;
    bool openFileBusy() const { return m_openFileBusy; }
    QString openFileError() const { return m_openFileError; }
    bool clearingUserData() const { return m_clearPending; }
    bool busy() const { return m_openFileBusy || m_clearPending; }
    QStringList listenFiles() const { return m_listenFiles; }

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void stopSession();
    Q_INVOKABLE void openWavFile(const QUrl& url);
    Q_INVOKABLE void reportMicrophoneDenied();
    Q_INVOKABLE void clearUserData();
    Q_INVOKABLE QUrl testsFolderUrl() const;
    Q_INVOKABLE QVariantList sessions() const;
    Q_INVOKABLE QVariantMap session(const QString& sessionId) const;

signals:
    void sessionActiveChanged();
    void phaseChanged();
    void phraseSecondsChanged();
    void audioLevelChanged();
    void hasResultChanged();
    void showingMeanChanged();
    void metricsChanged();
    void recordingSummaryChanged();
    void sessionsChanged();
    void openFileBusyChanged();
    void openFileErrorChanged();
    void openFileFinished(bool success, const QString& error);
    void clearingUserDataChanged();
    void userDataCleared();
    void busyChanged();
    void listenFilesChanged();

private slots:
    void applyPhase(int phase, int seconds, int epoch);
    void applyLevel(qreal level);
    void applyMetrics(const QVariantMap& metrics);
    void applyRunning(bool running);
    void applyDeviceFailed();
    void applyOpenFileFinished(bool success, const QString& error);
    void applyUserDataCleared();
    void applySessionEnded(quint64 generation);
    void applyRecordingSummary(quint64 generation, QVariantMap summary);
    void notifySessions();
    void noteListenFile(quint64 generation, const QString& path);

private:
    void beginCapture();
    void enqueueClearUserData();
    void resetResultState();
    void setOpenFileError(const QString& error);
    void setNoSpeech(bool noSpeech);
    void showMetrics(const QVariantMap& shown, double speechSeconds);
    void showSessionMean();
    void setShowingMean(bool showing);
    void storeRecordingSummary(const QVariantMap& summary);
    void clearRecordingSummary();
    void rememberShown(const QVariantMap& shown, double speechSeconds);
    void clearListenFiles();
    double medianOf(std::deque<double>& window, double rate, bool live);

    struct SessionAccumulator;
    std::unique_ptr<SessionAccumulator> m_accumulator;
    quint64 m_captureGeneration = 0;

    bool m_sessionActive = false;
    bool m_startPending = false;
    bool m_stopPending = false;
    bool m_openFileBusy = false;
    bool m_clearPending = false;
    bool m_clearEnqueued = false;
    int m_phase = IdleEmpty;
    int m_phraseSeconds = 0;
    int m_epoch = 0;
    qreal m_audioLevel = 0;
    bool m_hasResult = false;
    bool m_showingMean = false;
    bool m_noSpeech = false;
    double m_speechRate = 0;
    std::deque<double> m_speechRateWindow;
    double m_articulationRate = 0;
    double m_gaugeArticulationRate = 0;
    std::deque<double> m_articulationWindow;
    double m_phrasePauses = 0;
    double m_speechDuration = 0;
    double m_fillerScore = 0;
    QString m_openFileError;
    QVariantMap m_details;
    bool m_hasRecordingSummary = false;
    bool m_recordingSummaryPending = false;
    QVariantMap m_recordingSummary;
    QStringList m_listenFiles;
    quint64 m_listenGeneration = 0;
    QString m_shownSessionId;
    QString m_shownSessionStartedAt;
    QString m_lastShownKey;
    std::atomic<bool> m_alive { true };

    JobQueue* m_queue = nullptr;
    QThread* m_captureThread = nullptr;
    CaptureWorker* m_worker = nullptr;
    AnalysisThread* m_analysisThread = nullptr;
};

#endif
