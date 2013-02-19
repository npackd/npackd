#include <windows.h>
#include <shlobj.h>

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
#include "windowsregistry.h"
#include "xmlutils.h"
#include "wpmutils.h"
#include "installedpackages.h"

Repository Repository::def;
QMutex Repository::mutex;

Repository::Repository(): QObject()
{
    addWellKnownPackages();
}

bool packageVersionLessThan2(const PackageVersion* a, const PackageVersion* b) {
    int r = a->package.compare(b->package);
    if (r == 0) {
        r = a->version.compare(b->version);
    }

    return r < 0;
}

QList<PackageVersion*> Repository::getPackageVersions(const QString& package)
        const
{
    QList<PackageVersion*> ret = this->package2versions.values(package);

    qSort(ret.begin(), ret.end(), packageVersionLessThan2);

    return ret;
}

QList<PackageVersion*> Repository::getInstalledPackageVersions(
        const QString& package) const
{
    QList<PackageVersion*> ret = getPackageVersions(package);

    for (int i = 0; i < ret.count(); ) {
        PackageVersion* pv = ret.at(i);
        if (!pv->installed())
            ret.removeAt(i);
        else
            i++;
    }

    return ret;
}

QList<PackageVersion*> Repository::getInstalled()
{
    QList<PackageVersion*> ret;

    for (int i = 0; i < packageVersions.count(); i++) {
        PackageVersion* pv = packageVersions.at(i);
        if (pv->installed()) {
            ret.append(pv);
        }
    }

    return ret;
}

Repository::~Repository()
{
    qDeleteAll(this->packages);
    qDeleteAll(this->packageVersions);
    qDeleteAll(this->licenses);
}

void Repository::clearPackagesInNestedDirectories() {
    QList<PackageVersion*> pvs = this->getInstalled();
    qSort(pvs.begin(), pvs.end(), packageVersionLessThan2);

    for (int j = 0; j < pvs.count(); j++) {
        PackageVersion* pv = pvs.at(j);
        if (pv->installed() && !WPMUtils::pathEquals(pv->getPath(),
                WPMUtils::getWindowsDir())) {
            for (int i = j + 1; i < pvs.count(); i++) {
                PackageVersion* pv2 = pvs.at(i);
                if (pv2->installed() && !WPMUtils::pathEquals(pv2->getPath(),
                        WPMUtils::getWindowsDir())) {
                    if (WPMUtils::isUnder(pv2->getPath(), pv->getPath()) ||
                            WPMUtils::pathEquals(pv2->getPath(), pv->getPath())) {
                        pv2->setPath("");
                    }
                }
            }
        }
    }
}

PackageVersion* Repository::findNewestInstallablePackageVersion(
        const QString &package)
{
    PackageVersion* r = 0;

    QList<PackageVersion*> pvs = this->getPackageVersions(package);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* p = pvs.at(i);
        if (r == 0 || p->version.compare(r->version) > 0) {
            if (p->download.isValid())
                r = p;
        }
    }
    return r;
}

PackageVersion* Repository::findNewestInstalledPackageVersion(
        const QString &name)
{
    PackageVersion* r = 0;

    QList<PackageVersion*> pvs = this->getPackageVersions(name);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* p = pvs.at(i);
        if (p->installed()) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }
    return r;
}

PackageVersion* Repository::createPackageVersion(QDomElement* e, QString* err)
{
    return PackageVersion::parse(e, err);
}

Package* Repository::createPackage(QDomElement* e, QString* err)
{
    *err = "";

    QString name = e->attribute("name").trimmed();
    *err = WPMUtils::validateFullPackageName(name);
    if (!err->isEmpty()) {
        err->prepend("Error in attribute 'name' in <package>: ");
    }

    Package* a = new Package(name, name);

    if (err->isEmpty()) {
        a->title = XMLUtils::getTagContent(*e, "title");
        a->url = XMLUtils::getTagContent(*e, "url");
        a->description = XMLUtils::getTagContent(*e, "description");
    }

    if (err->isEmpty()) {
        a->icon = XMLUtils::getTagContent(*e, "icon");
        if (!a->icon.isEmpty()) {
            QUrl u(a->icon);
            if (!u.isValid() || u.isRelative() ||
                    !(u.scheme() == "http" || u.scheme() == "https")) {
                err->append(QString("Invalid icon URL for %1: %2").
                        arg(a->title).arg(a->icon));
            }
        }
    }

    if (err->isEmpty()) {
        a->license = XMLUtils::getTagContent(*e, "license");
    }

    if (err->isEmpty())
        return a;
    else {
        delete a;
        return 0;
    }
}

License* Repository::createLicense(QDomElement* e)
{
    QString name = e->attribute("name");
    License* a = new License(name, name);
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

License* Repository::findLicense(const QString& name)
{
    for (int i = 0; i < this->licenses.count(); i++) {
        if (this->licenses.at(i)->name == name)
            return this->licenses.at(i);
    }
    return 0;
}

QList<Package*> Repository::findPackages(const QString& name)
{
    QList<Package*> r;
    bool shortName = name.indexOf('.') < 0;
    QString suffix = '.' + name;
    for (int i = 0; i < this->packages.count(); i++) {
        Package* p = this->packages.at(i);
        if (p->name == name) {
            r.append(p);
        } else if (shortName && p->name.endsWith(suffix)) {
            r.append(p);
        }
    }
    return r;
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
            PackageVersion* newest = findNewestInstallablePackageVersion(
                    p->package);
            if (newest->version.compare(p->version) > 0 && !newest->installed())
                r++;
        }
    }
    return r;
}

void Repository::addWellKnownPackages()
{
    if (!this->findPackage("com.microsoft.Windows")) {
        Package* p = new Package("com.microsoft.Windows", "Windows");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        this->packages.append(p);
    }
    if (!this->findPackage("com.microsoft.Windows32")) {
        Package* p = new Package("com.microsoft.Windows32", "Windows/32 bit");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        this->packages.append(p);
    }
    if (!this->findPackage("com.microsoft.Windows64")) {
        Package* p = new Package("com.microsoft.Windows64", "Windows/64 bit");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        this->packages.append(p);
    }
    if (!findPackage("com.googlecode.windows-package-manager.Npackd")) {
        Package* p = new Package("com.googlecode.windows-package-manager.Npackd",
                "Npackd");
        p->url = "http://code.google.com/p/windows-package-manager/";
        p->description = "package manager";
        packages.append(p);
    }
    if (!this->findPackage("com.oracle.JRE")) {
        Package* p = new Package("com.oracle.JRE", "JRE");
        p->url = "http://www.java.com/";
        p->description = "Java runtime";
        this->packages.append(p);
    }
    if (!this->findPackage("com.oracle.JRE64")) {
        Package* p = new Package("com.oracle.JRE64", "JRE/64 bit");
        p->url = "http://www.java.com/";
        p->description = "Java runtime";
        this->packages.append(p);
    }
    if (!this->findPackage("com.oracle.JDK")) {
        Package* p = new Package("com.oracle.JDK", "JDK");
        p->url = "http://www.oracle.com/technetwork/java/javase/overview/index.html";
        p->description = "Java development kit";
        this->packages.append(p);
    }
    if (!this->findPackage("com.oracle.JDK64")) {
        Package* p = new Package("com.oracle.JDK64", "JDK/64 bit");
        p->url = "http://www.oracle.com/technetwork/java/javase/overview/index.html";
        p->description = "Java development kit";
        this->packages.append(p);
    }
    if (!this->findPackage("com.microsoft.DotNetRedistributable")) {
        Package* p = new Package("com.microsoft.DotNetRedistributable",
                ".NET redistributable runtime");
        p->url = "http://msdn.microsoft.com/en-us/netframework/default.aspx";
        p->description = ".NET runtime";
        this->packages.append(p);
    }
    if (!this->findPackage("com.microsoft.WindowsInstaller")) {
        Package* p = new Package("com.microsoft.WindowsInstaller",
                "Windows Installer");
        p->url = "http://msdn.microsoft.com/en-us/library/cc185688(VS.85).aspx";
        p->description = "Package manager";
        this->packages.append(p);
    }
    if (!this->findPackage("com.microsoft.MSXML")) {
        Package* p = new Package("com.microsoft.MSXML",
                "Microsoft Core XML Services (MSXML)");
        p->url = "http://www.microsoft.com/downloads/en/details.aspx?FamilyID=993c0bcf-3bcf-4009-be21-27e85e1857b1#Overview";
        p->description = "XML library";
        this->packages.append(p);
    }
}

QString Repository::planUpdates(const QList<Package*> packages,
        QList<InstallOperation*>& ops)
{
    QList<PackageVersion*> installed = getInstalled();
    QList<PackageVersion*> newest, newesti;
    QList<bool> used;

    QString err;

    for (int i = 0; i < packages.count(); i++) {
        Package* p = packages.at(i);

        PackageVersion* a = findNewestInstallablePackageVersion(p->name);
        if (a == 0) {
            err = QString("No installable version found for the package %1").
                    arg(p->title);
            break;
        }

        PackageVersion* b = findNewestInstalledPackageVersion(p->name);
        if (b == 0) {
            err = QString("No installed version found for the package %1").
                    arg(p->title);
            break;
        }

        if (a->version.compare(b->version) <= 0) {
            err = QString("The newest version (%1) for the package %2 is already installed").
                    arg(b->version.getVersionString()).arg(p->title);
            break;
        }

        newest.append(a);
        newesti.append(b);
        used.append(false);
    }

    if (err.isEmpty()) {
        // many packages cannot be installed side-by-side and overwrite for example
        // the shortcuts of the old version in the start menu. We try to find
        // those packages where the old version can be uninstalled first and then
        // the new version installed. This is the reversed order for an update.
        // If this is possible and does not affect other packages, we do this first.
        for (int i = 0; i < newest.count(); i++) {
            QList<PackageVersion*> avoid;
            QList<InstallOperation*> ops2;
            QList<PackageVersion*> installedCopy = installed;

            QString err = newesti.at(i)->planUninstallation(installedCopy, ops2);
            if (err.isEmpty()) {
                err = newest.at(i)->planInstallation(installedCopy, ops2, avoid);
                if (err.isEmpty()) {
                    if (ops2.count() == 2) {
                        used[i] = true;
                        installed = installedCopy;
                        ops.append(ops2[0]);
                        ops.append(ops2[1]);
                        ops2.clear();
                    }
                }
            }

            qDeleteAll(ops2);
        }
    }

    if (err.isEmpty()) {
        for (int i = 0; i < newest.count(); i++) {
            if (!used[i]) {
                QList<PackageVersion*> avoid;
                err = newest.at(i)->planInstallation(installed, ops, avoid);
                if (!err.isEmpty())
                    break;
            }
        }
    }

    if (err.isEmpty()) {
        for (int i = 0; i < newesti.count(); i++) {
            if (!used[i]) {
                err = newesti.at(i)->planUninstallation(installed, ops);
                if (!err.isEmpty())
                    break;
            }
        }
    }

    if (err.isEmpty()) {
        InstallOperation::simplify(ops);
    }

    return err;
}

QString Repository::writeTo(const QString& filename) const
{
    QString r;

    QDomDocument doc;
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    XMLUtils::addTextTag(root, "spec-version", "3");

    for (int i = 0; i < this->packages.count(); i++) {
        Package* p = packages.at(i);
        QDomElement package = doc.createElement("package");
        p->saveTo(package);
        root.appendChild(package);
    }

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        QDomElement version = doc.createElement("version");
        root.appendChild(version);
        pv->toXML(&version);
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream s(&file);
        doc.save(s, 4);
    } else {
        r = QString("Cannot open %1 for writing").arg(filename);
    }

    return "";
}

PackageVersion* Repository::findOrCreatePackageVersion(const QString &package,
        const Version &v)
{
    PackageVersion* pv = findPackageVersion(package, v);
    if (!pv) {
        pv = new PackageVersion(package);
        pv->version = v;
        pv->version.normalize();
        this->packageVersions.append(pv);
        this->package2versions.insert(package, pv);
    }
    return pv;
}

QStringList Repository::getAllInstalledPackagePaths() const
{
    QStringList r;
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = (PackageVersion*) this->packageVersions.at(i);
        if (pv->installed()) {
            QString dir = pv->getPath();
            r.append(dir);
        }
    }
    return r;
}

PackageVersion* Repository::findPackageVersionByMSIGUID(
        const QString& guid) const
{
    PackageVersion* r = 0;
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = (PackageVersion*) this->packageVersions.at(i);
        if (pv->msiGUID == guid) {
            r = pv;
            break;
        }
    }
    return r;
}

PackageVersion* Repository::findPackageVersion(const QString& package,
        const Version& version)
{
    PackageVersion* r = 0;

    QList<PackageVersion*> pvs = this->getPackageVersions(package);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* pv = pvs.at(i);
        if (pv->version.compare(version) == 0) {
            r = pv;
            break;
        }
    }
    return r;
}

void Repository::process(Job *job, const QList<InstallOperation *> &install)
{
    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        PackageVersion* pv = op->packageVersion;
        pv->lock();
    }

    int n = install.count();

    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        PackageVersion* pv = op->packageVersion;
        if (op->install)
            job->setHint(QString("Installing %1").arg(
                    pv->toString()));
        else
            job->setHint(QString("Uninstalling %1").arg(
                    pv->toString()));
        Job* sub = job->newSubJob(1.0 / n);
        if (op->install)
            pv->install(sub, pv->getPreferredInstallationDirectory());
        else
            pv->uninstall(sub);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(sub->getErrorMessage());
        delete sub;

        if (!job->getErrorMessage().isEmpty())
            break;
    }

    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        PackageVersion* pv = op->packageVersion;
        pv->unlock();
    }

    job->complete();
}

QString Repository::computeNpackdCLEnvVar()
{
    QString v;
    PackageVersion* pv;
    if (WPMUtils::is64BitWindows())
        pv = findNewestInstalledPackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL64");
    else
        pv = 0;

    if (!pv)
        pv = findNewestInstalledPackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL");

    if (pv)
        v = pv->getPath();

    return v;
}

void Repository::updateNpackdCLEnvVar()
{
    QString v = computeNpackdCLEnvVar();

    // ignore the error for the case NPACKD_CL does not yet exist
    QString err;
    QString cur = WPMUtils::getSystemEnvVar("NPACKD_CL", &err);

    if (v != cur) {
        if (WPMUtils::setSystemEnvVar("NPACKD_CL", v).isEmpty())
            WPMUtils::fireEnvChanged();
    }
}

void Repository::scan(const QString& path, Job* job, int level,
        QStringList& ignore)
{
    // TODO: this method is not thread-safe

    if (ignore.contains(path))
        return;

    QDir aDir(path);

    QMap<QString, QString> path2sha1;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        if (job && job->isCancelled())
            break;

        PackageVersion* pv = this->packageVersions.at(i);
        if (!pv->installed() && pv->detectFiles.count() > 0) {
            boolean ok = true;
            for (int j = 0; j < pv->detectFiles.count(); j++) {
                bool fileOK = false;
                DetectFile* df = pv->detectFiles.at(j);
                if (aDir.exists(df->path)) {
                    QString fullPath = path + "\\" + df->path;
                    QFileInfo f(fullPath);
                    if (f.isFile() && f.isReadable()) {
                        QString sha1 = path2sha1.value(df->path);
                        if (sha1.isEmpty()) {
                            sha1 = WPMUtils::sha1(fullPath);
                            path2sha1[df->path] = sha1;
                        }
                        if (df->sha1 == sha1) {
                            fileOK = true;
                        }
                    }
                }
                if (!fileOK) {
                    ok = false;
                    break;
                }
            }

            if (ok) {
                pv->setPath(path);
                return;
            }
        }
    }

    if (job && !job->isCancelled()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot | QDir::Dirs);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            if (job && job->isCancelled())
                break;

            QFileInfo entryInfo = entries[idx];
            QString name = entryInfo.fileName();

            if (job) {
                job->setHint(QString("%1").arg(name));
                if (job->isCancelled())
                    break;
            }

            Job* djob;
            if (level < 2)
                djob = job->newSubJob(1.0 / count);
            else
                djob = 0;
            scan(path + "\\" + name.toLower(), djob, level + 1, ignore);
            delete djob;

            if (job) {
                job->setProgress(((double) idx) / count);
            }
        }
    }

    if (job)
        job->complete();
}

void Repository::scanHardDrive(Job* job)
{
    QStringList ignore;
    ignore.append(WPMUtils::normalizePath(WPMUtils::getWindowsDir()));

    QFileInfoList fil = QDir::drives();
    for (int i = 0; i < fil.count(); i++) {
        if (job->isCancelled())
            break;

        QFileInfo fi = fil.at(i);

        job->setHint(QString("Scanning %1").arg(fi.absolutePath()));
        Job* djob = job->newSubJob(1.0 / fil.count());
        QString path = WPMUtils::normalizePath(fi.absolutePath());
        UINT t = GetDriveType((WCHAR*) path.utf16());
        if (t == DRIVE_FIXED)
            scan(path, djob, 0, ignore);
        delete djob;
    }

    job->complete();
}

void Repository::reload(Job *job, bool useCache)
{
    job->setHint("Loading repositories");
    Job* d = job->newSubJob(0.75);
    load(d, useCache);
    if (!d->getErrorMessage().isEmpty())
        job->setErrorMessage(d->getErrorMessage());
    delete d;

    addWellKnownPackages();

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        d = job->newSubJob(0.25);
        job->setHint("Refreshing installation statuses");
        refresh(d);
        delete d;
    }

    job->complete();
}

void Repository::refresh(Job *job)
{
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting directories deleted externally");
        for (int i = 0; i < this->packageVersions.count(); i++) {
            PackageVersion* pv = this->packageVersions.at(i);
            QString path = pv->getPath();
            if (!path.isEmpty()) {
                QDir d(path);
                d.refresh();
                if (!d.exists()) {
                    pv->setPath("");
                }
            }
        }
        job->setProgress(0.2);
    }

    InstalledPackages* ip = InstalledPackages::getDefault();
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier");
        ip->detectPre_1_15_Packages();
        job->setProgress(0.4);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Reading registry package database");
        InstalledPackages::getDefault()->readRegistryDatabase();
        job->setProgress(0.5);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting software");
        Job* d = job->newSubJob(0.2);
        ip->detect(d);
        delete d;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier (2)");
        ip->scanPre1_15Dir(true);
        job->setProgress(0.9);
    }

    if (job->shouldProceed(
            "Clearing information about installed package versions in nested directories")) {
        clearPackagesInNestedDirectories();
        job->setProgress(1);
    }

    job->complete();
}

void Repository::load(Job* job, bool useCache)
{
    qDeleteAll(this->packages);
    this->packages.clear();
    this->package2versions.clear();
    qDeleteAll(this->packageVersions);
    this->packageVersions.clear();

    QString err;
    QList<QUrl*> urls = getRepositoryURLs(&err);
    if (urls.count() > 0) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Repository %1 of %2").arg(i + 1).
                         arg(urls.count()));
            Job* s = job->newSubJob(1.0 / urls.count());
            loadOne(urls.at(i), s, useCache);
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
        job->setProgress(1);
    }

    // qDebug() << "Repository::load.3";

    qDeleteAll(urls);
    urls.clear();

    job->complete();
}

void Repository::loadOne(QUrl* url, Job* job, bool useCache) {
    job->setHint("Downloading");

    QTemporaryFile* f = 0;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        Job* djob = job->newSubJob(0.90);
        f = Downloader::download(djob, *url, 0, useCache);
        if (!djob->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Download failed: %2").
                    arg(djob->getErrorMessage()));
        delete djob;
    }

    QDomDocument doc;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        job->setHint("Parsing the content");
        // qDebug() << "Repository::loadOne.2";
        int errorLine;
        int errorColumn;
        QString errMsg;
        if (!doc.setContent(f, &errMsg, &errorLine, &errorColumn))
            job->setErrorMessage(QString(
                    "XML parsing failed at line %1, column %2: %3").
                    arg(errorLine).arg(errorColumn).arg(errMsg));
        else
            job->setProgress(0.91);
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        Job* djob = job->newSubJob(0.09);
        loadOne(&doc, djob);
        if (!djob->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error loading XML: %2").
                    arg(djob->getErrorMessage()));
        delete djob;
    }

    delete f;

    job->complete();
}

void Repository::loadOne(const QString& filename, Job* job)
{
    QFile f(filename);
    if (job->shouldProceed("Opening file")) {
        if (!f.open(QIODevice::ReadOnly))
            job->setErrorMessage("Cannot open the file");
        else
            job->setProgress(0.1);
    }

    QDomDocument doc;
    if (job->shouldProceed("Parsing XML")) {
        int errorLine, errorColumn;
        QString err;
        if (!doc.setContent(&f, false, &err, &errorLine, &errorColumn))
            job->setErrorMessage(QString(
                    "XML parsing failed at line %1, column %2: %3").
                    arg(errorLine).arg(errorColumn).arg(err));
        else
            job->setProgress(0.6);
    }

    if (job->shouldProceed("Analyzing the content")) {
        Job* sub = job->newSubJob(0.4);
        loadOne(&doc, sub);
        if (sub->getErrorMessage().isEmpty())
            job->setErrorMessage(sub->getErrorMessage());
        delete sub;
    }

    job->complete();
}

void Repository::loadOne(QDomDocument* doc, Job* job)
{
    QDomElement root;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        root = doc->documentElement();
        QDomNodeList nl = root.elementsByTagName("spec-version");
        if (nl.count() != 0) {
            QString specVersion = nl.at(0).firstChild().nodeValue();
            Version specVersion_;
            if (!specVersion_.setVersion(specVersion)) {
                job->setErrorMessage(QString(
                        "Invalid repository specification version: %1").
                        arg(specVersion));
            } else {
                if (specVersion_.compare(Version(4, 0)) >= 0)
                    job->setErrorMessage(QString(
                            "Incompatible repository specification version: %1. \n"
                            "Plese download a newer version of Npackd from http://code.google.com/p/windows-package-manager/").
                            arg(specVersion));
                else
                    job->setProgress(0.01);
            }
        } else {
            job->setProgress(0.01);
        }
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        for (QDomNode n = root.firstChild(); !n.isNull();
                n = n.nextSibling()) {
            if (n.isElement()) {
                QDomElement e = n.toElement();
                if (e.nodeName() == "version") {
                    QString err;
                    PackageVersion* pv = createPackageVersion(&e, &err);
                    if (pv) {
                        if (this->findPackageVersion(pv->package, pv->version))
                            delete pv;
                        else {
                            this->packageVersions.append(pv);
                            this->package2versions.insert(pv->package, pv);
                        }
                    } else {
                        job->setErrorMessage(err);
                        break;
                    }
                } else if (e.nodeName() == "package") {
                    QString err;
                    Package* p = createPackage(&e, &err);
                    if (p) {
                        if (this->findPackage(p->name))
                            delete p;
                        else
                            this->packages.append(p);
                    } else {
                        job->setErrorMessage(err);
                        break;
                    }
                } else if (e.nodeName() == "license") {
                    License* p = createLicense(&e);
                    if (this->findLicense(p->name))
                        delete p;
                    else
                        this->licenses.append(p);
                }
            }
        }
        job->setProgress(1);
    }

    job->complete();
}

void Repository::fireStatusChanged(PackageVersion *pv)
{
    emit statusChanged(pv);
}

PackageVersion* Repository::findLockedPackageVersion() const
{
    PackageVersion* r = 0;
    for (int i = 0; i < packageVersions.size(); i++) {
        PackageVersion* pv = packageVersions.at(i);
        if (pv->isLocked()) {
            r = pv;
            break;
        }
    }
    return r;
}

QStringList Repository::getRepositoryURLs(HKEY hk, const QString& path,
        QString* err)
{
    WindowsRegistry wr;
    *err = wr.open(hk, path, false, KEY_READ);
    QStringList urls;
    if (err->isEmpty()) {
        int size = wr.getDWORD("size", err);
        if (err->isEmpty()) {
            for (int i = 1; i <= size; i++) {
                WindowsRegistry er;
                *err = er.open(wr, QString("%1").arg(i), KEY_READ);
                if (err->isEmpty()) {
                    QString url = er.get("repository", err);
                    if (err->isEmpty())
                        urls.append(url);
                }
            }

            // ignore any errors while reading the entries
            *err = "";
        }
    }

    return urls;
}

QString Repository::checkLockedFilesForUninstall(
        const QList<InstallOperation*> &install)
{
    QStringList locked = WPMUtils::getProcessFiles();
    QStringList lockedUninstall;
    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        if (!op->install) {
            PackageVersion* pv = op->packageVersion;
            QString path = pv->getPath();
            for (int i = 0; i < locked.size(); i++) {
                if (WPMUtils::isUnder(locked.at(i), path)) {
                    lockedUninstall.append(locked.at(i));
                }
            }
        }
    }

    if (lockedUninstall.size() > 0) {
        QString locked_ = lockedUninstall.join(", \n");
        QString msg = QString("The package(s) cannot be uninstalled because "
                "the following files are in use "
                "(please close the corresponding applications): "
                "%1").arg(locked_);
        return msg;
    }

    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (!op->install) {
            PackageVersion* pv = op->packageVersion;
            if (pv->isDirectoryLocked()) {
                QString msg = QString("The package %1 cannot be uninstalled because "
                        "some files or directories under %2 are in use.").
                        arg(pv->toString()).
                        arg(pv->getPath());
                return msg;
            }
        }
    }

    return "";
}

QList<QUrl*> Repository::getRepositoryURLs(QString* err)
{
    // the most errors in this method are ignored so that we get the URLs even
    // if something cannot be done
    QString e;

    QStringList urls = getRepositoryURLs(HKEY_LOCAL_MACHINE,
            "Software\\Npackd\\Npackd\\Reps", &e);

    bool save = false;

    // compatibility for Npackd < 1.17
    if (urls.isEmpty()) {
        urls = getRepositoryURLs(HKEY_CURRENT_USER,
                "Software\\Npackd\\Npackd\\repositories", &e);
        if (urls.isEmpty())
            urls = getRepositoryURLs(HKEY_CURRENT_USER,
                    "Software\\WPM\\Windows Package Manager\\repositories",
                    &e);

        if (urls.isEmpty()) {
            urls.append(
                    "https://windows-package-manager.googlecode.com/hg/repository/Rep.xml");
            if (WPMUtils::is64BitWindows())
                urls.append(
                        "https://windows-package-manager.googlecode.com/hg/repository/Rep64.xml");
        }
        save = true;
    }

    QList<QUrl*> r;
    for (int i = 0; i < urls.count(); i++) {
        r.append(new QUrl(urls.at(i)));
    }

    if (save)
        setRepositoryURLs(r, &e);

    return r;
}

void Repository::setRepositoryURLs(QList<QUrl*>& urls, QString* err)
{
    WindowsRegistry wr;
    *err = wr.open(HKEY_LOCAL_MACHINE, "",
            false, KEY_CREATE_SUB_KEY);
    if (err->isEmpty()) {
        WindowsRegistry wrr = wr.createSubKey(
                "Software\\Npackd\\Npackd\\Reps", err,
                KEY_ALL_ACCESS);
        if (err->isEmpty()) {
            wrr.setDWORD("size", urls.count());
            for (int i = 0; i < urls.count(); i++) {
                WindowsRegistry r = wrr.createSubKey(QString("%1").arg(i + 1),
                        err, KEY_ALL_ACCESS);
                if (err->isEmpty()) {
                    r.set("repository", urls.at(i)->toString());
                }
            }
        }
    }
}

Repository* Repository::getDefault()
{
    return &def;
}


