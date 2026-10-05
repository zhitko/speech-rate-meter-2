#ifndef QMLLOGGER_H
#define QMLLOGGER_H

#include <QObject>
#include <QQmlEngine>
#include <QString>

/**
 * QmlLogger class provides logging functionality that can be accessed from QML.
 * It defines methods for logging messages at different levels (debug, info,
 * warning, critical) along with contextual information such as the source file,
 * line number, and function name. This allows for detailed logging of events
 * and errors in the application, which can be useful for debugging and
 * monitoring purposes. The logs can be output to the console or integrated with
 * a more sophisticated logging system as needed.
 */
class QmlLogger : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static QmlLogger* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return new QmlLogger();
    }

    explicit QmlLogger(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

public slots:
    /**
     * Logs a debug message with contextual information.
     *
     * @param message - The message to be logged.
     * @param file - The source file where the log is generated.
     * @param line - The line number in the source file.
     * @param function - The function name where the log is generated.
     */
    Q_INVOKABLE void debug(const QString& message, const QString& file, int line,
        const QString& function);

    /**
     * Logs an informational message with contextual information.
     *
     * @param message - The message to be logged.
     * @param file - The source file where the log is generated.
     * @param line - The line number in the source file.
     * @param function - The function name where the log is generated.
     */
    Q_INVOKABLE void info(const QString& message, const QString& file, int line,
        const QString& function);

    /**
     * Logs a warning message with contextual information.
     *
     * @param message - The message to be logged.
     * @param file - The source file where the log is generated.
     * @param line - The line number in the source file.
     * @param function - The function name where the log is generated.
     */
    Q_INVOKABLE void warning(const QString& message, const QString& file,
        int line, const QString& function);

    /**
     * Logs a critical error message with contextual information.
     *
     * @param message - The message to be logged.
     * @param file - The source file where the log is generated.
     * @param line - The line number in the source file.
     * @param function - The function name where the log is generated.
     */
    Q_INVOKABLE void critical(const QString& message, const QString& file,
        int line, const QString& function);
};

#endif // QMLLOGGER_H
