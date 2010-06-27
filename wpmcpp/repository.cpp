#include <windows.h>
#include <shlobj.h>

#include "repository.h"
#include "qhttp.h"
#include "qtemporaryfile.h"
#include "downloader.h"
#include "qsettings.h"
#include "qdom.h"

#include "downloader.h"

Repository* Repository::def = 0;

Repository::Repository()
{
}

void Repository::load()
{
    this->packageVersions.clear();
    QUrl* url = getRepositoryURL();
    if (url) {
        QString errMsg;
        QTemporaryFile* f = download(*url, &errMsg); // TODO: error handling
        QDomDocument doc;
        QString errorMsg;
        int errorLine;
        int errorColumn;
        // TODO: error handling
        if (doc.setContent(f, &errorMsg, &errorLine, &errorColumn)) {
            QDomElement root = doc.documentElement();
            for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
                if (n.isElement()) {
                    QDomElement e = n.toElement();
                    if (e.nodeName() == "version") {
                        PackageVersion* a = new PackageVersion(e.attribute("package"));
                        //a->download.setUrl("http://sourceforge.net/projects/notepad-plus/files/notepad%2B%2B%20releases%20binary/npp%205.6.8%20bin/npp.5.6.8.bin.zip/download");
                        QString url = e.elementsByTagName("url").at(0).firstChild().nodeValue();
                        a->download.setUrl(url);
                        QString name = e.attribute("name", "1.0");
                        a->setVersion(name);
                        this->packageVersions.append(a);
                    }
                }
            }
        }
        delete f;
    }
}

QUrl* Repository::getRepositoryURL()
{
    QSettings s("WPM", "Windows Package Manager");
    QString v = s.value("repository", "").toString();
    if (v != "")
        return new QUrl(v);
    else
        return 0;
}

void Repository::setRepositoryURL(QUrl& url)
{
    QSettings s("WPM", "Windows Package Manager");
    s.setValue("repository", url.toString());
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

QTemporaryFile* Repository::download(const QUrl &url, QString* errMsg)
{
    QTemporaryFile* file = new QTemporaryFile();
    file->open(); // TODO: handle return value

    Downloader d;
    bool r = d.download(url, file, errMsg);
    file->close();

    if (!r) {
        delete file;
        file = 0;
    }

    return file;
}
