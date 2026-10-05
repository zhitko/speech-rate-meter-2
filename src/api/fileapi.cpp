#include "fileapi.h"
#include "helpers/logger.h"
#include "helpers/settings.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {
void insert(QVariantMap& map, const QStringList& path, bool isFile)
{
    if (path.isEmpty()) {
        return;
    }

    QString key = path.first();
    QStringList remainingPath = path.mid(1);

    if (remainingPath.isEmpty()) {
        if (isFile) {
            map.insert(key,
                QVariant()); // It's a file, represented by a null QVariant
        } else { // It's a directory
            if (!map.contains(key)) {
                map.insert(key, QVariantMap());
            }
        }
        return;
    }

    if (!map.contains(key) || map[key].typeId() != QMetaType::QVariantMap) {
        map[key] = QVariant(QVariantMap());
    }

    QVariantMap innerMap = map[key].toMap();
    insert(innerMap, remainingPath, isFile);
    map[key] = innerMap;
}
} // namespace

FileApi::FileApi(QObject* parent)
    : QObject(parent)
{
}

void FileApi::deleteFile(const QString& file)
{
    LOG_DEBUG() << "Start: deleteFile - file=" << file;
    QString basePath = Settings::getAppDataDir();
    QString searchPath = QDir(basePath).filePath(file);
    QFile f(searchPath);
    f.remove();
    LOG_DEBUG() << "Finish: deleteFile";
}

QString FileApi::getApplicationDirPath()
{
    return Settings::getAppDataDir();
}

QUrl FileApi::getUrlFromPath(const QString& path)
{
    return QUrl::fromLocalFile(path);
}

QString FileApi::getPathFromUrl(const QUrl& url)
{
    return url.toLocalFile();
}

QString FileApi::readTextResource(const QUrl& url)
{
    if (url.scheme() != QLatin1String("qrc")) {
        LOG_WARNING() << "Rejected non-resource text URL:" << url;
        return {};
    }

    const QString resourcePath = QDir::cleanPath(
        QLatin1Char(':') + url.path());
    const QString allowedPrefix =
        QStringLiteral(":/qt/qml/SpeechRateMeter2/");
    if (!resourcePath.startsWith(allowedPrefix)) {
        LOG_WARNING() << "Rejected text resource outside application module:"
                      << resourcePath;
        return {};
    }

    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARNING() << "Failed to open text resource:" << resourcePath;
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

bool FileApi::directoryExists(const QString& path)
{
    LOG_DEBUG() << "Start: directoryExists - path=" << path;
    QString basePath = Settings::getAppDataDir();
    QString searchPath = QDir(basePath).filePath(path);
    bool exists = QDir(searchPath).exists();
    LOG_DEBUG() << "Finish: directoryExists - exists=" << exists
                << ", path=" << searchPath;
    return exists;
}

QVariantMap FileApi::getFiles(const QString& path,
    const QStringList& nameFilters)
{
    LOG_DEBUG() << "Start: getFiles - path=" << path
                << ", nameFilters=" << nameFilters;
    QString basePath = Settings::getAppDataDir();
    QString searchPath = QDir(basePath).filePath(path);

    QVariantMap fileTree;
    QDirIterator it(searchPath, QDir::AllEntries | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();

        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isFile()) {
            bool match = false;
            if (nameFilters.isEmpty()) {
                match = true;
            } else {
                for (const QString& filter : nameFilters) {
                    QRegularExpression re(
                        QRegularExpression::wildcardToRegularExpression(filter));
                    if (re.match(fileInfo.fileName()).hasMatch()) {
                        match = true;
                        break;
                    }
                }
            }

            if (!match) {
                continue; // Skip files that don't match the filter
            }
        }

        QString fullPath = fileInfo.filePath();
        QString relativePath = fullPath;
        if (searchPath.endsWith("/")) {
            relativePath.remove(0, searchPath.length());
        } else {
            relativePath.remove(0, searchPath.length() + 1);
        }

        if (relativePath.isEmpty()) {
            continue;
        }

        QStringList pathParts = relativePath.split('/');
        insert(fileTree, pathParts, fileInfo.isFile());
    }
    LOG_DEBUG() << "Finish: getFiles - fileCount=" << fileTree.keys().count();
    return fileTree;
}

QVariantList FileApi::getFolders(const QString& path)
{
    LOG_DEBUG() << "Start: getFolders - path=" << path;
    QString basePath = Settings::getAppDataDir();
    QString searchPath = QDir(basePath).filePath(path);

    QDir dir(searchPath);
    QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QVariantList result;
    for (const QString& folder : folders) {
        result << folder;
    }

    LOG_DEBUG() << "Finish: getFolders - folderCount=" << result.count();
    return result;
}

QVariantList FileApi::getFilesList(const QString& path,
    const QStringList& nameFilters)
{
    LOG_DEBUG() << "Start: getFilesList - path=" << path
                << ", nameFilters=" << nameFilters;
    QString basePath = Settings::getAppDataDir();
    QString searchPath = QDir(basePath).filePath(path);

    QVariantList fileList;
    QDirIterator it(searchPath, QDir::AllEntries | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();

        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isFile()) {
            bool match = false;
            if (nameFilters.isEmpty()) {
                match = true;
            } else {
                for (const QString& filter : nameFilters) {
                    QRegularExpression re(
                        QRegularExpression::wildcardToRegularExpression(filter));
                    if (re.match(fileInfo.fileName()).hasMatch()) {
                        match = true;
                        break;
                    }
                }
            }

            if (!match) {
                continue; // Skip files that don't match the filter
            }

            QString fullPath = fileInfo.filePath();
            QString relativePath = fullPath;
            if (searchPath.endsWith("/")) {
                relativePath.remove(0, searchPath.length());
            } else {
                relativePath.remove(0, searchPath.length() + 1);
            }

            if (relativePath.isEmpty()) {
                continue;
            }

            QString directory = "";
            if (relativePath.contains('/')) {
                directory = relativePath.section('/', 0, -2);
            } else {
                directory = ""; // Or handled as empty/top level
            }

            // Format directory for display (capitalize, maybe?)
            // For now, keep as is or just take the immediate parent folder name if we
            // want to mimic "Categories" style? "Basic/Sentence 1.wav" -> directory
            // "Basic" "Advanced/Sentences/S1.wav" -> directory "Advanced/Sentences"

            // If the user wants "Grouped by parent subdirectories",
            // "Advanced/Sentences" is good.

            QVariantMap fileData;
            fileData["fileName"] = fileInfo.fileName();
            fileData["filePath"] = relativePath;
            fileData["directory"] = directory;

            fileList.append(fileData);
        }
    }

    // Sort logic? Maybe the UI will sort or the iterator order is filesystem
    // dependent. Let's sort by directory then filename to ensure consistent
    // grouping.
    std::sort(
        fileList.begin(), fileList.end(),
        [](const QVariant& a, const QVariant& b) -> bool {
            const QVariantMap& mapA = a.toMap();
            const QVariantMap& mapB = b.toMap();
            const QString& dirA = mapA["directory"].toString();
            const QString& dirB = mapB["directory"].toString();
            if (dirA != dirB) {
                return dirA < dirB;
            }
            return mapA["fileName"].toString() < mapB["fileName"].toString();
        });

    LOG_DEBUG() << "Finish: getFilesList - fileCount=" << fileList.count();
    return fileList;
}