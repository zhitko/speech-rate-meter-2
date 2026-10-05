#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include <fstream>
#include <memory>
#include <mutex>
#include <string>

/**
 * FileLogger is a singleton class that provides thread-safe logging to a file.
 * It allows for writing log messages to a file and ensures that the log file
 * is properly initialized and managed. The logger can be used throughout the
 * application to record important events, errors, or debugging information.
 */
class FileLogger {
public:
    // Get the singleton instance
    static FileLogger& getInstance()
    {
        static FileLogger instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    /**
     * Writes a log message to the log file. This method is thread-safe and
     * ensures that log messages from different threads do not interleave.
     *
     * @param message - The log message to be written to the file.
     */
    void writeLog(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logFile.is_open()) {
            m_logFile << message << std::endl;
            m_logFile.flush(); // Ensure immediate write to disk
        }
    }

    /**
     * Initializes the log file by clearing its contents. This method can be
     * called at the start of the application to ensure that the log file is
     * empty before new log messages are written.
     */
    void initialize()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Close existing file if open
        if (m_logFile.is_open()) {
            m_logFile.close();
        }

        // Open file in truncate mode to clear it
        m_logFile.open("application.log", std::ios::out | std::ios::trunc);
    }

    ~FileLogger()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

private:
    FileLogger()
    {
        // Open log file in truncate mode (clears existing content)
        m_logFile.open("application.log", std::ios::out | std::ios::trunc);
    }

    std::ofstream m_logFile;
    std::mutex m_mutex;
};

#endif // FILE_LOGGER_H
