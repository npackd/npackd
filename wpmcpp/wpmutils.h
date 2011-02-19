#ifndef WPMUTILS_H
#define WPMUTILS_H

#include <windows.h>

#include "qstring.h"
#include "qdir.h"

#include "job.h"
#include "version.h"

/**
 * Some utility methods.
 */
class WPMUtils
{
private:
    WPMUtils();
public:
    static const char* NPACKD_VERSION;

    /**
     * Converts the value returned by SHFileOperation to an error message.
     *
     * @param res value returned by SHFileOperation
     * @return error message or "", if OK
     */
    static QString getShellFileOperationErrorMessage(int res);

    /**
     * Moves a directory to a recycle bin.
     *
     * @param dir directory
     * @return error message or ""
     */
    static QString moveToRecycleBin(QString dir);

    /**
     * @return true if this program is running on a 64-bit Windows
     */
    static bool is64BitWindows();

    /**
     * Deletes a directory
     *
     * @param job progress for this task
     * @param aDir this directory will be deleted
     */
    static void removeDirectory(Job* job, QDir &aDir);

    /**
     * Uses the Shell's IShellLink and IPersistFile interfaces
     * to create and store a shortcut to the specified object.
     *
     * @return the result of calling the member functions of the interfaces.
     * @param lpszPathObj - address of a buffer containing the path of the object.
     * @param lpszPathLink - address of a buffer containing the path where the
     *      Shell link is to be stored.
     * @param lpszDesc - address of a buffer containing the description of the
     *      Shell link.
     * @param workingDir working directory
     */
    static HRESULT createLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink,
            LPCWSTR lpszDesc,
            LPCWSTR workingDir);

    /**
     * Finds the parent directory for a path.
     *
     * @param path a directory
     * @return parent directory without an ending \\
     */
    static QString parentDirectory(const QString& path);

    /**
     * @return directory like "C:\Program Files"
     */
    static QString getProgramFilesDir();

    /**
     * - lower case
     * - replaces / by \
     * - removes \ at the end
     * - replace multiple occurences of \
     *
     * @param path a file/directory path
     * @return normalized path
     */
    static QString normalizePath(const QString& path);

    /**
     * @param type a CSIDL constant like CSIDL_COMMON_PROGRAMS
     * @return directory like
     *     "C:\Documents and Settings\All Users\Start Menu\Programs"
     */
    static QString getShellDir(int type);

    /**
     * Formats a Windows error message.
     *
     * @param err see GetLastError()
     * @param errMsg the message will be stored her
     */
    static void formatMessage(DWORD err, QString* errMsg);

    /**
     * Checks whether a file is somewhere in a directory (at any level).
     *
     * @param file the file
     * @param dir the directory
     */
    static bool isUnder(const QString& file, const QString& dir);

    /**
     * @return directory where the packages will be installed. Typically
     *     c:\program files\Npackd
     */
    static QString getInstallationDirectory();

    /**
     * see getInstallationDirectory()
     */
    static void setInstallationDirectory(const QString& dir);

    /**
     * @return full paths to files locked because of running processes
     */
    static QStringList getProcessFiles();

    /**
     * Computes SHA1 hash for a file
     *
     * @param filename name of the file
     * @return SHA1 or "" in case of an error or SHA1 in lower case
     */
    static QString sha1(const QString& filename);

    /**
     * Scans all files in a directory and deletes all links (.lnk) to files
     * in another directory.
     *
     * @param dir links to items in this directory (or any subdirectory) will
     *     be deleted
     * @param d this directory will be scanned for .lnk files
     */
    static void deleteShortcuts(const QString& dir, QDir& d);

    /**
     * Creates a path for a non-existing directory based on the start value.
     *
     * @param start start path (e.g. C:\Program Files\Prog 1.0)
     * @return non-existing path based on start
     *     (e.g. C:\Program Files\Prog 1.0_2)
     */
    static QString findNonExistingDir(const QString& start);

    /**
     * Reads a value from the registry.
     *
     * @param hk open key
     * @param var name of the REG_SZ-Variable
     * @return value or "" if an error has occured
     */
    static QString regQueryValue(HKEY hk, const QString& var);

    /**
     * Deletes a tree of nodes completely.
     *
     * @param hkey open key
     * @param path path to the key
     */
    static void regDeleteTree(HKEY hkey, const QString path);

    /**
     * @return directory with the .exe
     */
    static QString getExeDir();

    /**
     * @return C:\Windows
     */
    static QString getWindowsDir();

    /**
     * @return GUIDs for installed products (MSI)
     */
    static QStringList findInstalledMSIProducts();

    /**
     * @return Names and GUIDs for installed products (MSI)
     */
    static QStringList findInstalledMSIProductNames();

    /**
     * @param path .DLL file path
     * @return version number or 0.0 it it cannot be determined
     */
    static Version getDLLVersion(const QString& path);
};

#endif // WPMUTILS_H
