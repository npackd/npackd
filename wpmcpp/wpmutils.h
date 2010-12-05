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
     * Deletes a directory. If something cannot be deleted, it waits and
     * tries to delete the directory again.
     *
     * @param job progress for this task
     * @param aDir this directory will be deleted
     */
    static void removeDirectory2(Job* job, QDir &aDir);

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
     * Checks whether a file is somewhere in a directory (at any level). The
     * directory must exist.
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
     * @return SHA1 or "" in case of an error
     */
    static QString sha1(const QString& filename);

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
     * @return GUIDs for installed products (MSI)
     */
    static QStringList findInstalledMSIProducts();

    /**
     * @param path .DLL file path
     * @return version number or 0.0 it it cannot be determined
     */
    static Version getDLLVersion(const QString& path);
};

#endif // WPMUTILS_H
