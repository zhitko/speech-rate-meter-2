pragma Singleton

import QtQuick 6.8
import by.intoncore.logger 1.0

QtObject {
    id: logger

    function parseStackTrace() {
        try {
            // Create error to get stack trace
            var err = new Error();
            var stack = err.stack;

            // Split stack into lines
            var lines = stack.split('\n');

            // Find the first line that is NOT from Logger.qml
            for (var i = 0; i < lines.length; i++) {
                var line = lines[i];

                // Skip lines that reference Logger.qml itself
                if (line.indexOf('Logger.qml') !== -1) {
                    continue;
                }

                // Match pattern: at function (file:line:column) or file:line
                // Try to extract file, line, and function name
                var fileMatch = line.match(/([^\/]+\.qml):(\d+)/);
                var funcMatch = line.match(/at\s+([^\s(]+)/);

                if (fileMatch) {
                    var filename = fileMatch[1];
                    var lineNumber = parseInt(fileMatch[2]);
                    var functionName = funcMatch ? funcMatch[1] : "<anonymous>";

                    return {
                        file: filename,
                        line: lineNumber,
                        func: functionName
                    };
                }
            }
        } catch (e) {
            // Fallback if stack trace parsing fails
            return {
                file: "Unknown",
                line: 0,
                func: "<unknown>"
            };
        }

        // Fallback
        return {
            file: "Unknown",
            line: 0,
            func: "<unknown>"
        };
    }

    function debug(message) {
        var info = logger.parseStackTrace();
        QmlLogger.debug(message, info.file, info.line, info.func);
    }

    function info(message) {
        var info = logger.parseStackTrace();
        QmlLogger.info(message, info.file, info.line, info.func);
    }

    function warning(message) {
        var info = logger.parseStackTrace();
        QmlLogger.warning(message, info.file, info.line, info.func);
    }

    function critical(message) {
        var info = logger.parseStackTrace();
        QmlLogger.critical(message, info.file, info.line, info.func);
    }
}
