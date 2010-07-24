#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "qdebug.h"
#include "qdir.h"
#include "qstring.h"
#include "qfile.h"

#include "wpmutils.h"

WPMUtils::WPMUtils()
{
}

QString WPMUtils::parentDirectory(const QString& path)
{
    QString p = path;
    p = p.replace('/', '\\');
    int index = p.lastIndexOf('\\');
    return p.left(index);
}

QString WPMUtils::getProgramFilesDir()
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, CSIDL_PROGRAM_FILES, NULL, 0, dir);
    return  QString::fromUtf16(reinterpret_cast<ushort*>(dir));
}

bool WPMUtils::isUnder(QString &file, QString &dir)
{
    QString f = file.replace('/', '\\').toLower();
    QString d = dir.replace('/', '\\').toLower();

    return f.startsWith(d);
}

void WPMUtils::formatMessage(DWORD err, QString* errMsg)
{
    HLOCAL pBuffer;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   0, err, 0, (LPTSTR)&pBuffer, 0, 0);
    if (n == 0)
        errMsg->append(QString("Error %1").arg(err));
    else {
        errMsg->setUtf16((ushort*) pBuffer, n);
        LocalFree(pBuffer);
    }
}


QString WPMUtils::getProgramShortcutsDir()
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, CSIDL_PROGRAMS, NULL, 0, dir);
    return  QString::fromUtf16(reinterpret_cast<ushort*>(dir));
}

bool WPMUtils::removeDirectory(QDir &aDir, QString *errMsg)
{
    bool ok = true;
    if (aDir.exists()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::Dirs | QDir::Files);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            QFileInfo entryInfo = entries[idx];
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir()) {
                QDir dd(path);
                ok = removeDirectory(dd, errMsg);
                if (!ok)
                    qDebug() << "PackageVersion::removeDirectory.3" << *errMsg;
            } else {
                QFile file(path);
                ok = file.remove();
                if (!ok) {
                    ok = false;
                    errMsg->clear();
                    errMsg->append("Cannot delete the file: ").append(path);
                    qDebug() << "PackageVersion::removeDirectory.1" << *errMsg;
                }
            }
            if (!ok)
                break;
        }
        if (ok && !aDir.rmdir(aDir.absolutePath())) {
            qDebug() << "PackageVersion::removeDirectory.2";
            ok = false;
            errMsg->clear();
            errMsg->append("Cannot delete the directory: ").append(
                    aDir.absolutePath());
        }
    }
    qDebug() << "PackageVersion::removeDirectory: " << aDir << " " << ok <<
            *errMsg;
    return ok;
}
