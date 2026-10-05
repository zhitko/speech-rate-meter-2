#include "qmllogger.h"
#include "helpers/logger.h"

void QmlLogger::debug(const QString& message, const QString& file, int line,
    const QString& function)
{
    QtLogStream stream(QtLogger::Level::DEBUG, file, line, function);
    stream << message;
}

void QmlLogger::info(const QString& message, const QString& file, int line,
    const QString& function)
{
    QtLogStream stream(QtLogger::Level::INFO, file, line, function);
    stream << message;
}

void QmlLogger::warning(const QString& message, const QString& file, int line,
    const QString& function)
{
    QtLogStream stream(QtLogger::Level::WARNING, file, line, function);
    stream << message;
}

void QmlLogger::critical(const QString& message, const QString& file, int line,
    const QString& function)
{
    QtLogStream stream(QtLogger::Level::CRITICAL, file, line, function);
    stream << message;
}
