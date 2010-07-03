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
     * @return directory like "C:\Program Files"
     */
    static QString getProgramFilesDir();

    /**
     * @return directory like "C:\Users\t\Start Menu\Programs"
     */
    static QString getProgramShortcutsDir();

    /**
     * Formats a Windows error message.
     *
     * @param err see GetLastError()
     * @param errMsg the message will be stored her
     */
    static void formatMessage(DWORD err, QString* errMsg);
};

#endif // WPMUTILS_H
