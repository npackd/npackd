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

    /**
     * Creates an object from an existing HKEY
     *
     * @param hk existing HKEY or 0
     */
    WindowsRegistry(HKEY hk);

    /**
     * Creates a copy
     *
     * @param wr other registry object
     */
    WindowsRegistry(const WindowsRegistry& wr);

    ~WindowsRegistry();

    /**
     * Reads a REG_SZ value.
     *
     * @param name name of the variable
     * @param err error message will be stored here
     * @return the value
     */
    QString get(QString name, QString* err);

    /**
     * Reads a DWORD value.
     *
     * @param name name of the variable
     * @param err error message will be stored here
     * @return the value
     */
    DWORD getDWORD(QString name, QString* err);

    /**
     * Writes a DWORD value.
     *
     * @param name name of the variable
     * @return error message
     * @param value the value
     */
    QString setDWORD(QString name, DWORD value);

    /**
     * Writes a DWORD value.
     *
     * @param name name of the variable
     * @param value the value
     * @return error message
     */
    QString set(QString name, QString value);

    /**
     * Opens a key. The previously open key (if any) will be closed.
     * @param hk a key
     * @param path path under hk
     * @return error message or ""
     */
    QString open(HKEY hk, QString path);

    /**
     * Opens or creates a sub-key.
     *
     * @param name name of the sub-key
     * @param err error message or ""
     * @return created key (uninitialized, if an error occured)
     */
    WindowsRegistry createSubKey(QString name, QString* err);

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
