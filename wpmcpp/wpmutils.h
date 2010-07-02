#ifndef WPMUTILS_H
#define WPMUTILS_H

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
};

#endif // WPMUTILS_H
