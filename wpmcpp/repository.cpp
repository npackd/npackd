#include <windows.h>
#include <shlobj.h>

#include "qhttp.h"
#include "qtemporaryfile.h"
#include "downloader.h"
#include "qsettings.h"
#include "qdom.h"
#include "qdebug.h"

#include "repository.h"
#include "downloader.h"
#include "packageversionfile.h"
#include "wpmutils.h"
#include "version.h"

Repository* Repository::def = 0;

Repository::Repository()
{
    this->addWindowsPackage();
}

Repository::~Repository()
{
    qDeleteAll(this->packages);
    qDeleteAll(this->packageVersions);
}

PackageVersion* Repository::findNewestPackageVersion(const QString &name)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == name) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }
    return r;
}

PackageVersion* Repository::findNewestInstalledPackageVersion(QString &name)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == name && p->installed()) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }
    return r;
}

PackageVersion* Repository::createPackageVersion(QDomElement* e)
{
    // qDebug() << "Repository::createPackageVersion.1" << e->attribute("package");

    PackageVersion* a = new PackageVersion(
            e->attribute("package"));
    QString url = e->elementsByTagName("url").at(0).
                  firstChild().nodeValue();
    a->download.setUrl(url);
    QString name = e->attribute("name", "1.0");
    a->version.setVersion(name);

    QDomNodeList sha1 = e->elementsByTagName("sha1");
    if (sha1.count() > 0)
        a->sha1 = sha1.at(0).firstChild().nodeValue();

    QString type = e->attribute("type", "zip");
    if (type == "one-file")
        a->type = 1;
    else
        a->type = 0;

    QDomNodeList ifiles = e->elementsByTagName("important-file");
    for (int i = 0; i < ifiles.count(); i++) {
        QDomElement e = ifiles.at(i).toElement();
        QString p = e.attribute("path", "");
        if (p.isEmpty())
            p = e.attribute("name", "");
        a->importantFiles.append(p);

        QString title = e.attribute("title", p);
        a->importantFilesTitles.append(title);
    }

    QDomNodeList files = e->elementsByTagName("file");
    for (int i = 0; i < files.count(); i++) {
        QDomElement e = files.at(i).toElement();
        a->files.append(createPackageVersionFile(&e));
    }

    // qDebug() << "Repository::createPackageVersion.2";
    return a;
}

Package* Repository::createPackage(QDomElement* e)
{
    QString name = e->attribute("name");
    Package* a = new Package(name, name);
    QDomNodeList nl = e->elementsByTagName("title");
    if (nl.count() != 0)
        a->title = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("url");
    if (nl.count() != 0)
        a->url = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("description");
    if (nl.count() != 0)
        a->description = nl.at(0).firstChild().nodeValue();

    return a;
}

PackageVersionFile* Repository::createPackageVersionFile(QDomElement* e)
{
    QString path = e->attribute("path");
    QString content = e->firstChild().nodeValue();
    PackageVersionFile* a = new PackageVersionFile(path, content);

    return a;
}

Package* Repository::findPackage(QString& name)
{
    for (int i = 0; i < this->packages.count(); i++) {
        if (this->packages.at(i)->name == name)
            return this->packages.at(i);
    }
    return 0;
}

int Repository::countUpdates()
{
    int r = 0;
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->installed()) {
            PackageVersion* newest = findNewestPackageVersion(p->package);
            if (newest->version.compare(p->version) > 0 && !newest->installed())
                r++;
        }
    }
    return r;
}

void Repository::recognize(Job* job)
{
    job->setProgress(0);
    job->setCancellable(true);

    job->setHint("Detecting Windows");
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    Version v;
    v.setVersion(osvi.dwMajorVersion, osvi.dwMinorVersion,
            osvi.dwBuildNumber);
    PackageVersion* pv = this->findPackageVersion(
            "com.microsoft.Windows", v);
    if (!pv) {
        pv = new PackageVersion("com.microsoft.Windows");
        pv->version = v;
        this->packageVersions.append(pv);
    }
    pv->external = true;
    job->setProgress(0.5);

    if (!job->isCancelled()) {
        job->setHint("Detecting Java");
        HKEY hk;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                L"Software\\JavaSoft\\Java Runtime Environment",
                0, KEY_READ, &hk) == ERROR_SUCCESS) {
            WCHAR name[255];
            int index = 0;
            while (true) {
                DWORD nameSize = sizeof(name) / sizeof(name[0]);
                LONG r = RegEnumKeyEx(hk, index, name, &nameSize,
                        0, 0, 0, 0);
                if (r == ERROR_SUCCESS) {
                    QString v_;
                    v_.setUtf16((ushort*) name, nameSize);
                    v_ = v_.replace('_', '.');
                    if (v.setVersion(v_) && v.getNParts() > 2) {
                        pv = this->findPackageVersion("com.oracle.JRE",
                                v);
                        if (!pv) {
                            pv = new PackageVersion("com.oracle.JRE");
                            pv->version = v;
                            this->packageVersions.append(pv);
                        }
                        pv->external = true;
                    }
                } else if (r == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                index++;
            }
            RegCloseKey(hk);
        }
        job->setProgress(0.75);
    }

    // for .NET see
    // h*ttp://stackoverflow.com/questions/199080/how-to-detect-what-net-framework-versions-and-service-packs-are-installed
    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        job->setProgress(1);
    }

    job->complete();
}

QDir Repository::getDirectory()
{
    QString pf = WPMUtils::getProgramFilesDir();
    QDir d(pf + "\\WPM");
    return d;
}

PackageVersion* Repository::findPackageVersion(const QString& package,
        const Version& version)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == package && p->version.compare(version) == 0) {
            r = p;
            break;
        }
    }
    return r;
}

void Repository::addUnknownExistingPackages()
{
    QDir aDir = getDirectory();
    if (aDir.exists()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::Dirs);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            QFileInfo entryInfo = entries[idx];
            QString fn = entryInfo.fileName();
            QStringList sl = fn.split('-');
            if (sl.count() == 2) {
                QString package = sl.at(0);
                if (Package::isValidName(package)) {
                    QString version_ = sl.at(1);
                    Version version;
                    if (version.setVersion(version_)) {
                        if (this->findPackage(package) == 0) {
                            QString title = package +
                                    " (unknown in current repositories)";
                            Package* p = new Package(package,
                                    title);
                            this->packages.append(p);
                        }
                        if (this->findPackageVersion(package, version) == 0) {
                            PackageVersion* pv = new PackageVersion(package);
                            pv->version = version;
                            this->packageVersions.append(pv);
                        }
                    }
                }
            }
        }
    }
}

void Repository::addWindowsPackage()
{
    Package* p = new Package("com.microsoft.Windows", "Windows");
    p->url = "http://www.microsoft.com/windows/";
    p->description = "Operating system";
    this->packages.append(p);
}

void Repository::load(Job* job)
{
    qDeleteAll(this->packages);
    this->packages.clear();
    qDeleteAll(this->packageVersions);
    this->packageVersions.clear();

    addWindowsPackage();

    QList<QUrl*> urls = getRepositoryURLs();
    if (urls.count() > 0) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Repository %1 of %2").arg(i + 1).
                         arg(urls.count()));
            Job* s = job->newSubJob(1 / urls.count());
            loadOne(urls.at(i), s);
            if (!s->getErrorMessage().isEmpty()) {
                job->setErrorMessage(QString(
                        "Error loading the repository %1: %2").arg(
                        urls.at(i)->toString()).arg(
                        s->getErrorMessage()));
                delete s;
                break;
            }
            delete s;

            if (job->isCancelled())
                break;
        }
    } else {
        job->setErrorMessage("No repositories defined");
    }

    // qDebug() << "Repository::load.3";

    job->setHint("Scanning for installed package versions");
    addUnknownExistingPackages();

    qDeleteAll(urls);
    urls.clear();

    job->complete();
}

void Repository::loadOne(QUrl* url, Job* job) {
    job->setHint("Downloading");
    Job* djob = job->newSubJob(0.90);
    QTemporaryFile* f = Downloader::download(djob, *url);
    // qDebug() << "Repository::loadOne.1";
    if (f) {
        job->setHint("Parsing the content");
        // qDebug() << "Repository::loadOne.2";
        QDomDocument doc;
        int errorLine;
        int errorColumn;
        QString errMsg;
        if (doc.setContent(f, &errMsg, &errorLine, &errorColumn)) {
            QDomElement root = doc.documentElement();
            for (QDomNode n = root.firstChild(); !n.isNull();
                    n = n.nextSibling()) {
                if (n.isElement()) {
                    QDomElement e = n.toElement();
                    if (e.nodeName() == "version") {
                        this->packageVersions.append(
                                createPackageVersion(&e));
                    } else if (e.nodeName() == "package") {
                        this->packages.append(
                                createPackage(&e));
                    }
                }
            }
            job->setProgress(1);
        } else {
            job->setErrorMessage(QString("XML parsing failed: %1").
                                 arg(errMsg));
        }
        delete f;
    } else {
        job->setErrorMessage(QString("Download failed: %2").
                arg(djob->getErrorMessage()));
    }
    delete djob;

    job->complete();
}


QList<QUrl*> Repository::getRepositoryURLs()
{
    QList<QUrl*> r;
    QSettings s("WPM", "Windows Package Manager");

    int size = s.beginReadArray("repositories");
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        QString v = s.value("repository").toString();
        r.append(new QUrl(v));
    }
    s.endArray();

    if (size == 0) {
        QString v = s.value("repository", "").toString();
        if (v != "") {
            r.append(new QUrl(v));
            setRepositoryURLs(r);
        }
    }
    
    return r;
}

void Repository::setRepositoryURLs(QList<QUrl*>& urls)
{
    QSettings s("WPM", "Windows Package Manager");
    s.beginWriteArray("repositories", urls.count());
    for (int i = 0; i < urls.count(); ++i) {
        s.setArrayIndex(i);
        s.setValue("repository", urls.at(i)->toString());
    }
    s.endArray();
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}


