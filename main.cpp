#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QtQml>

// APIS
#include "audioapi.h"
#include "fileapi.h"
#include "sessionapi.h"
#include "settingsapi.h"

// Logging
#include "qmllogger.h"
#include "src/services/helpers/fileLogger.h"

#ifdef Q_OS_ANDROID
/**
 * Extracts bundled assets from the APK to the writable app data directory.
 * Qt for Android does NOT automatically extract assets/ to the filesystem —
 * they are only accessible via the assets:/ URI scheme. Since our C++ code
 * (and the C-level WAV library) uses real filesystem paths, we must copy
 * them out on first launch.
 *
 * Only files that don't already exist in the destination are copied, so
 * repeated startup is fast.
 */
static void extractAndroidAssets()
{
    const QString srcPrefix = QStringLiteral("assets:/");
    const QString dstDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // The asset paths to extract (relative to assets:/)
    const QStringList assetRoots = {
        QStringLiteral("settings.ini"),
        QStringLiteral("data")
    };

    for (const QString& root : assetRoots) {
        const QString srcPath = srcPrefix + root;
        const QString dstPath = dstDir + QLatin1Char('/') + root;

        QFileInfo dstInfo(dstPath);
        if (dstInfo.exists()) {
            // Already extracted — skip
            continue;
        }

        // Recursively copy assets
        // Use a stack of (source, destination) pairs
        struct Entry { QString src; QString dst; };
        QList<Entry> stack = { { srcPath, dstPath } };

        while (!stack.isEmpty()) {
            Entry e = stack.takeLast();
            QFileInfo srcInfo(e.src);

            // Qt's asset file engine doesn't support QFileInfo::isDir() reliably,
            // so we try to list the directory contents first.
            QDir dir(e.src);
            QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            if (!entries.isEmpty()) {
                // It's a directory — create destination and enqueue children
                QDir().mkpath(e.dst);
                for (const QString& entry : entries) {
                    stack.append({
                        e.src + QLatin1Char('/') + entry,
                        e.dst + QLatin1Char('/') + entry
                    });
                }
            } else {
                // It's a file — copy it
                if (QFile::exists(e.src)) {
                    QDir().mkpath(QFileInfo(e.dst).absolutePath());
                    QFile::copy(e.src, e.dst);
                }
            }
        }
    }
}
#endif // Q_OS_ANDROID

static void appendDebugLog(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    const QString line = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss:zzz "))
        + message;
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    QFile file(QStringLiteral("logs.txt"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << line << '\n';
    }
    fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char* argv[])
{
    // Initialize file logger (clears the log file)
    FileLogger::getInstance().initialize();
    qInstallMessageHandler(appendDebugLog);

#ifdef Q_OS_ANDROID
    // Extract bundled assets (settings.ini, data/patterns, …) from APK
    // to the writable filesystem so fopen() and QFile paths work.
    extractAndroidAssets();
#endif

    QGuiApplication app(argc, argv);
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.0.0"
#endif
    app.setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(
        ":/qt/qml/SpeechRateMeter2/res/icons/speechratemeter.png")));

    // Explicitly load fonts from resources to ensure they are available on all platforms (especially Windows)
    // These paths match the structure defined in qt_add_qml_module RESOURCES
    const QStringList fontPaths = {
        QString::fromLatin1(":/qt/qml/SpeechRateMeter2/res/fonts/fa-brands-400.ttf"),
        QString::fromLatin1(":/qt/qml/SpeechRateMeter2/res/fonts/fa-regular-400.ttf"),
        QString::fromLatin1(":/qt/qml/SpeechRateMeter2/res/fonts/fa-solid-900.ttf")
    };

    for (const QString& path : fontPaths) {
        if (QFontDatabase::addApplicationFont(path) == -1) {
            // Fallback if prefixing is different (e.g. no /qt/qml prefix)
            QString fallbackPath = path;
            fallbackPath.replace(QLatin1String(":/qt/qml/"), QLatin1String(":/"));
            if (QFontDatabase::addApplicationFont(fallbackPath) == -1) {
                // Second fallback for direct res path
                QString secondFallback = QLatin1String(":/res/fonts/") + path.section(QLatin1Char('/'), -1);
                QFontDatabase::addApplicationFont(secondFallback);
            }
        }
    }

    qmlRegisterType<AudioApi>("by.intoncore.audio", 1, 0, "AudioApi");
    qmlRegisterType<FileApi>("by.intoncore.file", 1, 0, "FileApi");
    qmlRegisterType<SettingsApi>("by.intoncore.settings", 1, 0, "SettingsApi");
    qmlRegisterType<SessionApi>("by.intoncore.session", 1, 0, "SessionApi");
    qmlRegisterSingletonType<QmlLogger>("by.intoncore.logger", 1, 0, "QmlLogger", &QmlLogger::create);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule(QLatin1String("SpeechRateMeter2"), QLatin1String("Main"));

    return app.exec();
}
