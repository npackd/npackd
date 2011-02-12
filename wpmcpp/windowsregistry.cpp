#include "windowsregistry.h"

#include <windows.h>

#include "qstring.h"

#include "wpmutils.h"

WindowsRegistry::WindowsRegistry()
{
    this->hkey = 0;
}

WindowsRegistry::WindowsRegistry(const WindowsRegistry& wr)
{
    if (wr.hkey == 0)
        this->hkey = 0;
    else {
        const REGSAM KEY_WOW64_64KEY = 0x0100;
        bool w64bit = WPMUtils::is64BitWindows();
        LONG r = RegOpenKeyEx(wr.hkey,
                L"",
                0, KEY_ALL_ACCESS | (w64bit ? KEY_WOW64_64KEY : 0),
                &this->hkey);
        if (r != ERROR_SUCCESS) {
            this->hkey = 0;
        }
    }
}

WindowsRegistry::WindowsRegistry(HKEY hk)
{
    this->hkey = hk;
}

WindowsRegistry::~WindowsRegistry()
{
    close();
}

QString WindowsRegistry::get(QString name, QString* err)
{
    err->clear();

    if (this->hkey == 0) {
        err->append("No key is open");
        return "";
    }

    QString value_;
    char value[255];
    DWORD valueSize = sizeof(value);
    LONG r = RegQueryValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, 0, (BYTE*) value,
                &valueSize);
    if (r == ERROR_SUCCESS) {
        value_.setUtf16((ushort*) value, valueSize / 2 - 1);
    } else {
        WPMUtils::formatMessage(r, err);
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
    LONG r = RegQueryValueEx(this->hkey,
                (WCHAR*) name.utf16(), 0, 0, (BYTE*) &value,
                &valueSize);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, err);
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

QString WindowsRegistry::open(HKEY hk, QString path)
{
    close();

    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows();
    LONG r = RegOpenKeyEx(hk,
            (WCHAR*) path.utf16(),
            0, KEY_ALL_ACCESS | (w64bit ? KEY_WOW64_64KEY : 0),
            &this->hkey);
    if (r == ERROR_SUCCESS) {
        return "";
    } else {
        QString s;
        WPMUtils::formatMessage(r, &s);
        return s;
    }
}

WindowsRegistry WindowsRegistry::createSubKey(QString name, QString* err)
{
    err->clear();

    if (this->hkey == 0) {
        err->append("No key is open");
        return 0;
    }

    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows();
    HKEY hk;
    LONG r = RegCreateKeyEx(this->hkey,
            (WCHAR*) name.utf16(),
            0, 0, 0, KEY_ALL_ACCESS | (w64bit ? KEY_WOW64_64KEY : 0), 0,
            &hk, 0);
    if (r != ERROR_SUCCESS) {
        WPMUtils::formatMessage(r, err);
        hk = 0;
    }

    return WindowsRegistry(hk);
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
