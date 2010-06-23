#include <windows.h>
#include <shlobj.h>

#include "repository.h"
#include "qhttp.h"
#include "qtemporaryfile.h"
#include "downloader.h"

Repository* Repository::def = 0;

Repository::Repository()
{
    // TODO: remove later
    PackageVersion* a = new PackageVersion(QString("net.sourceforge.NotepadPlusPlus"));
    a->download.setUrl("http://sourceforge.net/projects/notepad-plus/files/notepad%2B%2B%20releases%20binary/npp%205.6.8%20bin/npp.5.6.8.bin.zip/download");

    this->packageVersions.append(a);
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}

QString Repository::getProgramFilesDir()
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, CSIDL_PROGRAM_FILES, NULL, 0, dir);
    return  QString::fromUtf16 (reinterpret_cast<ushort*>(dir));
}

QTemporaryFile* Repository::download(const QUrl &url)
{
    QTemporaryFile* file = new QTemporaryFile();
    file->open(); // TODO: handle return value

    Downloader d;
    d.download(url, file);
    file->close();

    return file;
}
