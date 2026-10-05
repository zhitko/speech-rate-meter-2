#ifndef API_LOGGER_H
#define API_LOGGER_H

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QString>

#include "../services/helpers/fileLogger.h"

/*
 * Custom logging utility that integrates with Qt's logging system and also
 * writes logs to a file using FileLogger. Provides stream-style logging with
 * automatic timestamp, filename, line number, and function name.
 *
 * Usage:
 *   LOG_DEBUG() << "This is a debug message with value:" << someValue;
 *   LOG_INFO() << "Informational message";
 *   LOG_WARNING() << "Warning message";
 *   LOG_CRITICAL() << "Critical error message";
 *
 * Logs will be printed to the console and also saved to a log file.
 */
class QtLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        CRITICAL
    };

    static QString formatPrefix(const QString& file,
        int line,
        const QString& function)
    {
        // Get current timestamp with milliseconds
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyy-MM-dd hh:mm:ss.zzz");

        // Extract filename from full path
        QFileInfo fileInfo(file);
        QString filename = fileInfo.fileName();

        // Format: "timestamp filename:line:function"
        return QString("%1 %2:%3 [%4]")
            .arg(timestamp)
            .arg(filename)
            .arg(line)
            .arg(function);
    }

    static QtMsgType levelToMsgType(Level level)
    {
        switch (level) {
        case Level::DEBUG:
            return QtDebugMsg;
        case Level::INFO:
            return QtInfoMsg;
        case Level::WARNING:
            return QtWarningMsg;
        case Level::CRITICAL:
            return QtCriticalMsg;
        }
        return QtDebugMsg;
    }
};

/*
 * Helper class to accumulate log messages and write them on destruction.
 * This allows for stream-style logging while ensuring that the full message is
 * logged with the appropriate prefix and written to both console and file.
 * Example usage:
 *   LOG_DEBUG() << "Value of x:" << x << "and y:" << y;
 * This will log a message like:
 *   "2024-06-01 12:34:56.789 MyFile.cpp:123 [MyFunction] Value of x: 10 and y:
 * 20"
 */
class QtLogStream {
public:
    QtLogStream(QtLogger::Level level,
        const QString& file,
        int line,
        const QString& function)
        : m_level(level)
        , m_file(file)
        , m_line(line)
        , m_function(function)
    {
    }

    ~QtLogStream()
    {
        QString prefix = QtLogger::formatPrefix(m_file, m_line, m_function);
        QString fullMessage = QString("%1 %2").arg(prefix).arg(m_message);

        // Write to console
        switch (m_level) {
        case QtLogger::Level::DEBUG:
            qDebug().noquote() << fullMessage;
            break;
        case QtLogger::Level::INFO:
            qInfo().noquote() << fullMessage;
            break;
        case QtLogger::Level::WARNING:
            qWarning().noquote() << fullMessage;
            break;
        case QtLogger::Level::CRITICAL:
            qCritical().noquote() << fullMessage;
            break;
        }

        // Write to file
        FileLogger::getInstance().writeLog(fullMessage.toStdString());
    }

    template <typename T>
    QtLogStream& operator<<(const T& value)
    {
        if (!m_message.isEmpty()) {
            m_message += " ";
        }

        // Convert to QString for accumulation
        QDebug debug(&m_message);
        debug.nospace() << value;

        return *this;
    }

private:
    QtLogger::Level m_level;
    QString m_file;
    int m_line;
    QString m_function;
    QString m_message;
};

// Macros to make it easier to use with stream-style syntax
#define LOG_DEBUG() \
    QtLogStream(QtLogger::Level::DEBUG, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO() \
    QtLogStream(QtLogger::Level::INFO, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING() \
    QtLogStream(QtLogger::Level::WARNING, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL() \
    QtLogStream(QtLogger::Level::CRITICAL, __FILE__, __LINE__, __FUNCTION__)

#endif // API_LOGGER_H
