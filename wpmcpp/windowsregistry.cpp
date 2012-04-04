#include "windowsregistry.h"

#include <windows.h>
#include <aclapi.h>

#include "qstring.h"

#include "wpmutils.h"

WindowsRegistry::WindowsRegistry()
{
    this->hkey = 0;
    this->useWow6432Node = false;
    this->samDesired = KEY_ALL_ACCESS;
}

WindowsRegistry::WindowsRegistry(const WindowsRegistry& wr)
{
    this->useWow6432Node = wr.useWow6432Node;

    if (wr.hkey == 0)
        this->hkey = 0;
    else {
        const REGSAM KEY_WOW64_64KEY = 0x0100;
        bool w64bit = WPMUtils::is64BitWindows() && !useWow6432Node;
        LONG r = RegOpenKeyEx(wr.hkey,
                L"",
                0, wr.samDesired | (w64bit ? KEY_WOW64_64KEY : 0),
                &this->hkey);
        if (r != ERROR_SUCCESS) {
            this->hkey = 0;
        }
    }
}

WindowsRegistry::WindowsRegistry(HKEY hk, bool useWow6432Node,
        REGSAM samDesired)
{
    this->hkey = hk;
    this->useWow6432Node = useWow6432Node;
    this->samDesired = samDesired;
}

WindowsRegistry::~WindowsRegistry()
{
    close();
}

QString WindowsRegistry::get(QString name, QString* err) const
{
    err->clear();

    if (this->hkey == 0) {
        err->append("No key is open");
        return "";
    }

    QString value_;
    DWORD valueSize;
    LONG r = RegQueryValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, 0, 0,
                &valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, err);
    } else {
        char* value = new char[valueSize];
        r = RegQueryValueEx(this->hkey,
                    (WCHAR*) name.utf16(), 0, 0, (BYTE*) value,
                    &valueSize);
        if (r != ERROR_SUCCESS) {
            WPMUtils::formatMessage(r, err);
        } else {
            value_.setUtf16((ushort*) value, valueSize / 2 - 1);
        }
        delete[] value;
    }

    return value_;
}

DWORD WindowsRegistry::getDWORD(QString name, QString* err)
{
    err->clear();

    if (this->hkey == 0) {
        err->append("No key is open");
        return 0;
    }

    DWORD value = 0;
    DWORD valueSize = sizeof(value);
    DWORD type;
    LONG r = RegQueryValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, &type, (BYTE*) &value,
                &valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, err);
    } else if (type != REG_DWORD) {
        *err = "Wrong registry value type (DWORD expected)";
    }
    return value;
}

QString WindowsRegistry::setDWORD(QString name, DWORD value)
{
    QString err;

    if (this->hkey == 0) {
        return "No key is open";
    }

    DWORD valueSize = sizeof(value);
    LONG r = RegSetValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, REG_DWORD, (BYTE*) &value,
                valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, &err);
    }
    return err;
}

QString WindowsRegistry::set(QString name, QString value)
{
    QString err;

    if (this->hkey == 0) {
        return "No key is open";
    }

    DWORD valueSize = (value.length() + 1) * 2;
    LONG r = RegSetValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, REG_SZ, (BYTE*) value.utf16(),
                valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, &err);
    }
    return err;
}

QString WindowsRegistry::setExpand(QString name, QString value)
{
    QString err;

    if (this->hkey == 0) {
        return "No key is open";
    }

    DWORD valueSize = (value.length() + 1) * 2;
    LONG r = RegSetValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, REG_EXPAND_SZ, (BYTE*) value.utf16(),
                valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, &err);
    }
    return err;
}

QStringList WindowsRegistry::list(QString* err)
{
    err->clear();

    QStringList res;
    if (this->hkey == 0) {
        err->append("No key is open");
        return res;
    }

    WCHAR name[255];
    int index = 0;
    while (true) {
        DWORD nameSize = sizeof(name) / sizeof(name[0]);
        LONG r = RegEnumKeyEx(this->hkey, index, name, &nameSize,
                0, 0, 0, 0);
        if (r == ERROR_SUCCESS) {
            QString v_;
            v_.setUtf16((ushort*) name, nameSize);
            res.append(v_);
        } else if (r == ERROR_NO_MORE_ITEMS) {
            break;
        } else {
            WPMUtils::formatMessage(r, err);
            break;
        }
        index++;
    }

    return res;
}

QString WindowsRegistry::open(const WindowsRegistry& wr, QString subkey,
        REGSAM samDesired)
{
    return open(wr.hkey, subkey, wr.useWow6432Node, samDesired);
}

QString WindowsRegistry::open(HKEY hk, QString path, bool useWow6432Node,
        REGSAM samDesired)
{
    close();

    this->useWow6432Node = useWow6432Node;
    this->samDesired = samDesired;

    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows() && !useWow6432Node;
    LONG r = RegOpenKeyEx(hk,
            (WCHAR*) path.utf16(),
            0, samDesired | (w64bit ? KEY_WOW64_64KEY : 0),
            &this->hkey);
    if (r == ERROR_SUCCESS) {
        return "";
    } else {
        QString s;
        WPMUtils::formatMessage(r, &s);
        return s;
    }
}

WindowsRegistry WindowsRegistry::createSubKey(QString name, QString* err,
        REGSAM samDesired)
{
    err->clear();

    if (this->hkey == 0) {
        err->append("No key is open");
        return WindowsRegistry();
    }

    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows() && !useWow6432Node;
    HKEY hk;
    LONG r = RegCreateKeyEx(this->hkey,
            (WCHAR*) name.utf16(),
            0, 0, 0, samDesired | (w64bit ? KEY_WOW64_64KEY : 0), 0,
            &hk, 0);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, err);
        hk = 0;
    }

    return WindowsRegistry(hk, this->useWow6432Node, samDesired);
}

QString WindowsRegistry::close()
{
    if (this->hkey != 0) {
        LONG r = RegCloseKey(this->hkey);
        this->hkey = 0;
        if (r == ERROR_SUCCESS) {
            return "";
        } else {
            QString s;
            WPMUtils::formatMessage(r, &s);
            return s;
        }
    } else {
        return "";
    }
}
