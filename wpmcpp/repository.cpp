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
#include "msi.h"

Repository* Repository::def = 0;

Repository::Repository()
{
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

    QDomNodeList deps = e->elementsByTagName("dependency");
    for (int i = 0; i < deps.count(); i++) {
        QDomElement e = deps.at(i).toElement();
        Dependency* d = createDependency(&e);
        if (d)
            a->dependencies.append(d);
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

Dependency* Repository::createDependency(QDomElement* e)
{
    // qDebug() << "Repository::createDependency";

    QString package = e->attribute("package").trimmed();

    QString versions = e->attribute("versions").trimmed();

    bool minIncluded, maxIncluded;

    // qDebug() << "Repository::createDependency.1" << versions;

    if (versions.startsWith('['))
        minIncluded = true;
    else if (versions.startsWith('('))
        minIncluded = false;
    else
        return 0;
    versions.remove(0, 1);

    // qDebug() << "Repository::createDependency.1.1" << versions;

    if (versions.endsWith(']'))
        maxIncluded = true;
    else if (versions.endsWith(')'))
        maxIncluded = false;
    else
        return 0;
    versions.chop(1);

    // qDebug() << "Repository::createDependency.2";

    QStringList parts = versions.split(',');
    if (parts.count() != 2)
        return 0;

    Version min, max;
    if (!min.setVersion(parts.at(0).trimmed()) ||
            !max.setVersion(parts.at(1).trimmed()))
        return 0;

    Dependency* d = new Dependency();
    d->package = package;
    d->minIncluded = minIncluded;
    d->min = min;
    d->maxIncluded = maxIncluded;
    d->max = max;

    // qDebug() << d->toString();

    return d;
}

Package* Repository::findPackage(const QString& name)
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
    if (!this->findPackage("com.microsoft.Windows")) {
        Package* p = new Package("com.microsoft.Windows", "Windows");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        this->packages.append(p);
    }
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
        job->setHint("Detecting JRE");
        detectJRE();
        job->setProgress(0.75);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JDK");
        detectJDK();
        job->setProgress(0.8);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        detectDotNet();
        job->setProgress(0.9);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        detectMSIProducts();
        job->setProgress(1);
    }

    job->complete();
}

void Repository::detectJRE()
{
    if (!this->findPackage("com.oracle.JRE")) {
        Package* p = new Package("com.oracle.JRE", "JRE");
        p->url = "http://www.java.com/";
        p->description = "Java runtime";
        this->packages.append(p);
    }
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
                Version v;
                if (v.setVersion(v_) && v.getNParts() > 2) {
                    PackageVersion* pv =
                            this->findPackageVersion("com.oracle.JRE", v);
                    if (!pv) {
                        pv = new PackageVersion("com.oracle.JRE");
                        pv->version = v;
                        pv->external = true;
                        this->packageVersions.append(pv);
                    } else {
                        if (!pv->installed())
                            pv->external = true;
                    }
                }
            } else if (r == ERROR_NO_MORE_ITEMS) {
                break;
            }
            index++;
        }
        RegCloseKey(hk);
    }
}

void Repository::detectJDK()
{
    if (!this->findPackage("com.oracle.JDK")) {
        Package* p = new Package("com.oracle.JDK", "JDK");
        p->url = "http://www.oracle.com/technetwork/java/javase/overview/index.html";
        p->description = "Java development kit";
        this->packages.append(p);
    }
    HKEY hk;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"Software\\JavaSoft\\Java Development Kit",
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
                Version v;
                if (v.setVersion(v_) && v.getNParts() > 2) {
                    PackageVersion* pv =
                            this->findPackageVersion("com.oracle.JDK", v);
                    if (!pv) {
                        pv = new PackageVersion("com.oracle.JDK");
                        pv->version = v;
                        pv->external = true;
                        this->packageVersions.append(pv);
                    } else {
                        if (!pv->installed())
                            pv->external = true;
                    }
                }
            } else if (r == ERROR_NO_MORE_ITEMS) {
                break;
            }
            index++;
        }
        RegCloseKey(hk);
    }
}

void Repository::versionDetected(const QString &package, const Version &v)
{
    PackageVersion* pv = findPackageVersion(package, v);
    if (pv) {
        if (!pv->installed())
            pv->external = true;
    } else {
        pv = new PackageVersion(package);
        pv->version = v;
        pv->external = true;
        this->packageVersions.append(pv);
    }
}

void Repository::detectOneDotNet(HKEY hk2, const QString& keyName)
{
    QString packageName("com.microsoft.DotNetRedistributable");
    Version keyVersion;

    Version oneOne(1, 1);
    Version four(4, 0);
    Version two(2, 0);

    Version v;
    bool found = false;
    if (keyName.startsWith("v") && keyVersion.setVersion(
            keyName.right(keyName.length() - 1))) {
        if (keyVersion.compare(oneOne) < 0) {
            // not yet implemented
        } else if (keyVersion.compare(two) < 0) {
            v = keyVersion;
            found = true;
        } else if (keyVersion.compare(four) < 0) {
            QString value_ = WPMUtils::regQueryValue(hk2, "Version");
            if (v.setVersion(value_)) {
                found = true;
            }
        } else {
            HKEY hk3;
            if (RegOpenKeyExW(hk2, L"Full",
                    0, KEY_READ, &hk3) == ERROR_SUCCESS) {
                QString value_ = WPMUtils::regQueryValue(hk3, "Version");
                if (v.setVersion(value_)) {
                    found = true;
                }
                RegCloseKey(hk2);
            }
        }
    }

    if (found) {
        PackageVersion* pv = this->findPackageVersion(
                packageName, v);
        if (!pv) {
            pv = new PackageVersion(packageName);
            pv->version = v;
            pv->external = true;
            this->packageVersions.append(pv);
        } else {
            if (!pv->installed())
                pv->external = true;
        }
    }
}

void Repository::detectMSIProducts()
{
    QStringList guids = WPMUtils::findInstalledMSIProducts();

    // Detecting VisualC++ runtimes:
    // http://blogs.msdn.com/b/astebner/archive/2009/01/29/9384143.aspx
    if (guids.contains("{FF66E9F6-83E7-3A3E-AF14-8DE9A809A6A4}")) {
        this->versionDetected("com.microsoft.VisualCPPRedistributable",
                Version("9.0.21022.8"));
    }
    if (guids.contains("{9A25302D-30C0-39D9-BD6F-21E6EC160475}")) {
        this->versionDetected("com.microsoft.VisualCPPRedistributable",
                Version("9.0.30729.17"));
    }
    if (guids.contains("{1F1C2DFC-2D24-3E06-BCB8-725134ADF989}")) {
        this->versionDetected("com.microsoft.VisualCPPRedistributable",
                Version("9.0.30729.4148"));
    }
}

void Repository::detectDotNet()
{
    QString packageName("com.microsoft.DotNetRedistributable");

    // http://stackoverflow.com/questions/199080/how-to-detect-what-net-framework-versions-and-service-packs-are-installed
    if (!this->findPackage(packageName)) {
        Package* p = new Package("com.microsoft.DotNetRedistributable",
                ".NET redistributable runtime");
        p->url = "http://www.microsoft.com/downloads/details.aspx?FamilyID=0856eacb-4362-4b0d-8edd-aab15c5e04f5&amp;displaylang=en";
        p->description = ".NET runtime";
        this->packages.append(p);
    }
    HKEY hk;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\NET Framework Setup\\NDP",
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
                Version v;
                if (v_.startsWith("v") && v.setVersion(
                        v_.right(v_.length() - 1))) {
                    HKEY hk2;
                    if (RegOpenKeyExW(hk, (WCHAR*) v_.utf16(),
                            0, KEY_READ, &hk2) == ERROR_SUCCESS) {
                        detectOneDotNet(hk2, v_);
                        RegCloseKey(hk2);
                    }
                }
            } else if (r == ERROR_NO_MORE_ITEMS) {
                break;
            }
            index++;
        }
        RegCloseKey(hk);
    }
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

void Repository::load(Job* job)
{
    qDeleteAll(this->packages);
    this->packages.clear();
    qDeleteAll(this->packageVersions);
    this->packageVersions.clear();

    QList<QUrl*> urls = getRepositoryURLs();
    if (urls.count() > 0) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Repository %1 of %2").arg(i + 1).
                         arg(urls.count()));
            Job* s = job->newSubJob(0.9 / urls.count());
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
        job->setProgress(0.9);
    }

    // qDebug() << "Repository::load.3";

    job->setHint("Scanning for installed package versions");
    addUnknownExistingPackages();

    qDeleteAll(urls);
    urls.clear();

    job->setHint("Detecting software");
    Job* sub = job->newSubJob(0.1);
    recognize(sub);
    delete sub;

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


