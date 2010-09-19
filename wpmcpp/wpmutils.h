#ifndef WPMUTILS_H
#define WPMUTILS_H

#include <windows.h>

#include "qstring.h"
#include "qdir.h"

/**
 * Some utility methods.
 */
class WPMUtils
{
private:
    WPMUtils();
public:
    /**
     * Deletes a directory
     *
     * @param aDir this directory will be deleted
     * @param errMsg error message will be stored here
     * @return true if no errors occured
     */
    static bool removeDirectory(QDir &aDir, QString* errMsg);

    /**
     * Deletes a directory. If something cannot be deleted, it waits and
     * tries to delete the directory again.
     *
     * @param aDir this directory will be deleted
     * @param errMsg error message will be stored here
     * @return true if no errors occured
     */
    static bool removeDirectory2(QDir &aDir, QString* errMsg);

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
     * @return GUIDs for installed products (MSI)
     */
    static QStringList findInstalledMSIProducts();
};

#endif // WPMUTILS_H
