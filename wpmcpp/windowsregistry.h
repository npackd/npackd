#ifndef WINDOWSREGISTRY_H
#define WINDOWSREGISTRY_H

#include "windows.h"

#include "qstring.h"

/**
 * Windows registry. Better than QSettings as the value do not land under
 * Wow6432Node on 64-bit Windows versions.
 */
class WindowsRegistry
{
private:
    /** current key or 0 */
    HKEY hkey;
public:
    WindowsRegistry();
    ~WindowsRegistry();

    /**
     * @param hk a key
     * @param path path under hk
     * @return error message or ""
     */
    QString open(HKEY hk, QString path);

    /**
     * Closes the current key.
     *
     * @return error message or ""
     */
    QString close();

    /**
     * @param err the error message will be stored here
     * @return list of sub-keys
     */
    QStringList list(QString* err);
};

#endif // WINDOWSREGISTRY_H
