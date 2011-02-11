#include "windowsregistry.h"

#include <windows.h>

#include "qstring.h"

#include "wpmutils.h"

WindowsRegistry::WindowsRegistry()
{
    this->hkey = 0;
}

WindowsRegistry::~WindowsRegistry()
{
    close();
}

QStringList WindowsRegistry::list(QString* err) {
    err->clear();

    QStringList res;

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

QString WindowsRegistry::close()
{
    if (this->hkey != 0) {
        LONG r = RegCloseKey(this->hkey);
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
