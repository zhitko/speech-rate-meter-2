#ifndef LOGGER_H
#define LOGGER_H

#include <filesystem>
#include <QDebug>
#include <QString>
#include <sstream>
#include <string>

class Logger {
public:
    enum class Level { DEBUG,
        INFO,
        WARNING,
        CRITICAL };

    static void log(Level level, const std::string& file, int line,
        const std::string& function, const std::string& message)
    {
        std::filesystem::path filePath(file);
        const QString formatted = QStringLiteral("%1:%2 [%3] %4")
                                      .arg(QString::fromStdString(filePath.filename().string()))
                                      .arg(line)
                                      .arg(QString::fromStdString(function))
                                      .arg(QString::fromStdString(message));
        switch (level) {
        case Level::DEBUG:
            qDebug().noquote() << formatted;
            break;
        case Level::INFO:
            qInfo().noquote() << formatted;
            break;
        case Level::WARNING:
            qWarning().noquote() << formatted;
            break;
        case Level::CRITICAL:
            qCritical().noquote() << formatted;
            break;
        }
    }
};

// LogStream helper class for stream-style logging
class LogStream {
public:
    LogStream(Logger::Level level, const std::string& file, int line,
        const std::string& function)
        : m_level(level)
        , m_file(file)
        , m_line(line)
        , m_function(function)
    {
    }

    ~LogStream()
    {
        Logger::log(m_level, m_file, m_line, m_function, m_stream.str());
    }

    template <typename T>
    LogStream& operator<<(const T& value)
    {
        m_stream << value;
        return *this;
    }

private:
    Logger::Level m_level;
    std::string m_file;
    int m_line;
    std::string m_function;
    std::ostringstream m_stream;
};

// Macros to make it easier to use with stream-style syntax
#define LOG_DEBUG() \
    LogStream(Logger::Level::DEBUG, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO() \
    LogStream(Logger::Level::INFO, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING() \
    LogStream(Logger::Level::WARNING, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL() \
    LogStream(Logger::Level::CRITICAL, __FILE__, __LINE__, __FUNCTION__)

#endif // LOGGER_H
