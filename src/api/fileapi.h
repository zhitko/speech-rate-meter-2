#ifndef FILEAPI_H
#define FILEAPI_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>

/**
 * FileApi class provides methods for file system operations such as listing
 * files and folders, deleting files, and retrieving application directory
 * paths. This class is designed to be used in a QML context, allowing for easy
 * integration with the UI components of the application. The methods in this
 * class enable the application to interact with the file system in a
 * platform-independent way, making it easier to manage user data and
 * application resources.
 */
class FileApi : public QObject {
    Q_OBJECT
public:
    explicit FileApi(QObject* parent = nullptr);

    /**
     * Retrieves a list of files in the specified directory that match the given
     * name filters. The method returns a QVariantMap where the keys are file
     * names and the values are file paths. This allows the UI to display a list
     * of files along with their corresponding paths for user interaction.
     *
     * @param path - The directory path to search for files.
     * @param nameFilters - A list of name filters (e.g., "*.txt", "*.wav") to
     *                      specify which files to include in the results.
     * @return A QVariantMap containing file names as keys and file paths as
     * values.
     */
    Q_INVOKABLE QVariantMap getFiles(const QString& path,
        const QStringList& nameFilters);

    /**
     * Retrieves a list of folders in the specified directory. The method returns
     * a QVariantList containing the names of the folders found in the given path.
     * This allows the UI to display a list of folders for user navigation and
     * selection.
     *
     * @param path - The directory path to search for folders.
     * @return A QVariantList containing the names of the folders in the specified
     * path.
     */
    Q_INVOKABLE QVariantList getFolders(const QString& path);

    /**
     * Retrieves a list of files in the specified directory that match the given
     * name filters. The method returns a QVariantList containing the file paths
     * of the matching files. This allows the UI to display a list of file paths
     * for user interaction.
     *
     * @param path - The directory path to search for files.
     * @param nameFilters - A list of name filters (e.g., "*.txt", "*.wav") to
     *                      specify which files to include in the results.
     * @return A QVariantList containing the file paths of the matching files.
     */
    Q_INVOKABLE QVariantList getFilesList(const QString& path,
        const QStringList& nameFilters);

    /**
     * Deletes the specified file from the file system. The method takes a file
     * path as input and attempts to remove the file. If the file is successfully
     * deleted, the method returns true; otherwise, it returns false. This allows
     * the UI to provide functionality for users to manage their files directly
     * from the application.
     * @param file - The file path of the file to be deleted.
     * @return true if the file was successfully deleted, false otherwise.
     */
    Q_INVOKABLE void deleteFile(const QString& file);

    /**
     * Retrieves the directory path of the application's executable or main
     * directory. This method is useful for accessing application resources or
     * saving user data in a location relative to the application. The returned
     * path is platform-dependent and may vary based on the operating system and
     * deployment method.
     *
     * @return The directory path of the application's executable or main
     * directory.
     */
    Q_INVOKABLE QString getApplicationDirPath();

    /**
     * Converts a local file path into a properly formatted URL.
     * This is especially important for Windows to avoid UNC path timeouts
     * in native file dialogs when paths are manually prefixed with file://.
     *
     * @param path - The local filesystem path.
     * @return A QUrl representing the path.
     */
    Q_INVOKABLE QUrl getUrlFromPath(const QString& path);

    /**
     * Converts a URL back into a local filesystem path.
     * Handles percent-encoding and platform-specific path separators.
     *
     * @param url - The URL to convert.
     * @return A local filesystem path string.
     */
    Q_INVOKABLE QString getPathFromUrl(const QUrl& url);

    /**
     * Reads a text file embedded in this application's Qt resource module.
     * Local filesystem URLs and resources outside the module are rejected.
     */
    Q_INVOKABLE QString readTextResource(const QUrl& url);

    /**
     * Checks if the specified directory exists in the file system. The method
     * takes a directory path as input and returns true if the directory exists,
     * or false if it does not. This allows the UI to validate user input for
     * directory paths and provide feedback on whether the specified location is
     * valid.
     *
     * @param path - The directory path to check for existence.
     * @return true if the directory exists, false otherwise.
     */
    Q_INVOKABLE bool directoryExists(const QString& path);
};

#endif // FILEAPI_H
