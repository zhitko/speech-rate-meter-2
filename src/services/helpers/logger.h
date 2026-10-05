#ifndef LOGGER_H
#define LOGGER_H

#include "fileLogger.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
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
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch())
            % 1000;

        std::string levelStr;
        switch (level) {
        case Level::DEBUG:
            levelStr = "DEBUG";
            break;
        case Level::INFO:
            levelStr = "INFO";
            break;
        case Level::WARNING:
            levelStr = "WARNING";
            break;
        case Level::CRITICAL:
            levelStr = "CRITICAL";
            break;
        }

        // Extract filename from path
        std::filesystem::path filePath(file);
        std::string filename = filePath.filename().string();

        // Format the log message
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "."
            << std::setfill('0') << std::setw(3) << ms.count() << " " << filename
            << ":" << line << " [" << function << "] " << message;

        std::string formattedMessage = oss.str();

        // Write to console
        std::cout << formattedMessage << std::endl;

        // Write to file
        FileLogger::getInstance().writeLog(formattedMessage);
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
