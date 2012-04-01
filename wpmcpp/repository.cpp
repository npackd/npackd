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

Repository Repository::def;

Repository::Repository(): QObject()
{
    addWellKnownPackages();
}

bool packageVersionLessThan(const PackageVersion* a, const PackageVersion* b) {
    int r = a->package.compare(b->package);
    if (r == 0) {
        r = a->version.compare(b->version);
    }

    return r < 0;
}

QList<PackageVersion*> Repository::getPackageVersions(const QString& package)
{
    QList<PackageVersion*> ret;

    for (int i = 0; i < packageVersions.count(); i++) {
        PackageVersion* pv = packageVersions.at(i);
        if (pv->package == package) {
            ret.append(pv);
        }
    }

    qSort(ret.begin(), ret.end(), packageVersionLessThan);

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

PackageVersion* Repository::findNewestInstallablePackageVersion(
        const QString &package)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == package) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                if (p->download.isValid())
                    r = p;
            }
        }
    }
    return r;
}

PackageVersion* Repository::findNewestInstalledPackageVersion(
        const QString &name)
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

DetectFile* Repository::createDetectFile(QDomElement* e, QString* err)
{
    *err = "";

    DetectFile* a = new DetectFile();
    a->path = XMLUtils::getTagContent(*e, "path").trimmed();
    a->path.replace('/', '\\');
    if (a->path.isEmpty()) {
        err->append("Empty tag <path> under <detect-file>");
    }

    if (err->isEmpty()) {
        a->sha1 = XMLUtils::getTagContent(*e, "sha1").trimmed().toLower();
        *err = WPMUtils::validateSHA1(a->sha1);
        if (!err->isEmpty()) {
            err->prepend("Wrong SHA1 in <detect-file>: ");
        }
    }

    if (err->isEmpty())
        return a;
    else {
        delete a;
        return 0;
    }
}

PackageVersion* Repository::createPackageVersion(QDomElement* e, QString* err)
{
    *err = "";

    // qDebug() << "Repository::createPackageVersion.1" << e->attribute("package");

    QString packageName = e->attribute("package").trimmed();
    *err = WPMUtils::validateFullPackageName(packageName);
    if (!err->isEmpty()) {
        err->prepend("Error in the attribute 'package' in <version>: ");
    }

    PackageVersion* a = new PackageVersion(packageName);

    if (err->isEmpty()) {
        QString url = XMLUtils::getTagContent(*e, "url").trimmed();
        if (!url.isEmpty()) {
            a->download.setUrl(url);
            QUrl d = a->download;
            if (!d.isValid() || d.isRelative() ||
                    (d.scheme() != "http" && d.scheme() != "https")) {
                err->append(QString("Not a valid download URL for %1: %2").
                        arg(a->package).arg(url));
            }
        }
    }

    if (err->isEmpty()) {
        QString name = e->attribute("name", "1.0").trimmed();
        if (a->version.setVersion(name)) {
            a->version.normalize();
        } else {
            err->append(QString("Not a valid version for %1: %2").
                    arg(a->package).arg(name));
        }
    }

    if (err->isEmpty()) {
        a->sha1 = XMLUtils::getTagContent(*e, "sha1").trimmed().toLower();
        if (!a->sha1.isEmpty()) {
            *err = WPMUtils::validateSHA1(a->sha1);
            if (!err->isEmpty()) {
                err->prepend(QString("Invalid SHA1 for %1: ").
                        arg(a->toString()));
            }
        }
    }

    if (err->isEmpty()) {
        QString type = e->attribute("type", "zip").trimmed();
        if (type == "one-file")
            a->type = 1;
        else if (type == "" || type == "zip")
            a->type = 0;
        else {
            err->append(QString("Wrong value for the attribute 'type' for %1: %3").
                    arg(a->toString()).arg(type));
        }
    }

    if (err->isEmpty()) {
        QDomNodeList ifiles = e->elementsByTagName("important-file");
        for (int i = 0; i < ifiles.count(); i++) {
            QDomElement e = ifiles.at(i).toElement();
            QString p = e.attribute("path").trimmed();
            if (p.isEmpty())
                p = e.attribute("name").trimmed();

            if (p.isEmpty()) {
                err->append(QString("Empty 'path' attribute value for <important-file> for %1").
                        arg(a->toString()));
                break;
            }

            if (a->importantFiles.contains(p)) {
                err->append(QString("More than one <important-file> with the same 'path' attribute %1 for %2").
                        arg(p).arg(a->toString()));
                break;
            }

            a->importantFiles.append(p);

            QString title = e.attribute("title").trimmed();
            if (title.isEmpty()) {
                err->append(QString("Empty 'title' attribute value for <important-file> for %1").
                        arg(a->toString()));
                break;
            }

            a->importantFilesTitles.append(title);
        }
    }

    if (err->isEmpty()) {
        QDomNodeList files = e->elementsByTagName("file");
        for (int i = 0; i < files.count(); i++) {
            QDomElement e = files.at(i).toElement();
            PackageVersionFile* pvf = createPackageVersionFile(&e, err);
            if (pvf) {
                a->files.append(pvf);
            } else {
                break;
            }
        }
    }

    if (err->isEmpty()) {
        for (int i = 0; i < a->files.count() - 1; i++) {
            PackageVersionFile* fi = a->files.at(i);
            for (int j = i + 1; j < a->files.count(); j++) {
                PackageVersionFile* fj = a->files.at(j);
                if (fi->path == fj->path) {
                    err->append(QString("Duplicate <file> entry for %1 in %2").
                            arg(fi->path).arg(a->toString()));
                    goto out;
                }
            }
        }
    out:;
    }

    if (err->isEmpty()) {
        QDomNodeList detectFiles = e->elementsByTagName("detect-file");
        for (int i = 0; i < detectFiles.count(); i++) {
            QDomElement e = detectFiles.at(i).toElement();
            DetectFile* df = createDetectFile(&e, err);
            if (df) {
                a->detectFiles.append(df);
            } else {
                err->prepend(QString("Invalid <detect-file> for %1: ").
                        arg(a->toString()));
                break;
            }
        }
    }

    if (err->isEmpty()) {
        for (int i = 0; i < a->detectFiles.count() - 1; i++) {
            DetectFile* fi = a->detectFiles.at(i);
            for (int j = i + 1; j < a->detectFiles.count(); j++) {
                DetectFile* fj = a->detectFiles.at(j);
                if (fi->path == fj->path) {
                    err->append(QString("Duplicate <detect-file> entry for %1 in %2").
                            arg(fi->path).arg(a->toString()));
                    goto out2;
                }
            }
        }
    out2:;
    }

    if (err->isEmpty()) {
        QDomNodeList deps = e->elementsByTagName("dependency");
        for (int i = 0; i < deps.count(); i++) {
            QDomElement e = deps.at(i).toElement();
            Dependency* d = createDependency(&e);
            if (d)
                a->dependencies.append(d);
        }
    }

    if (err->isEmpty()) {
        for (int i = 0; i < a->dependencies.count() - 1; i++) {
            Dependency* fi = a->dependencies.at(i);
            for (int j = i + 1; j < a->dependencies.count(); j++) {
                Dependency* fj = a->dependencies.at(j);
                if (fi->autoFulfilledIf(*fj) ||
                        fj->autoFulfilledIf(*fi)) {
                    err->append(QString("Duplicate <dependency> for %1 in %2").
                            arg(fi->package).arg(a->toString()));
                    goto out3;
                }
            }
        }
    out3:;
    }

    if (err->isEmpty()) {
        a->msiGUID = XMLUtils::getTagContent(*e, "detect-msi").trimmed().
                toLower();
        if (!a->msiGUID.isEmpty()) {
            if (a->msiGUID.length() != 38) {
                err->append(QString("Wrong MSI GUID for %1: %3").
                        arg(a->toString()).arg(a->msiGUID));
            } else {
                for (int i = 0; i < a->msiGUID.length(); i++) {
                    QChar c = a->msiGUID.at(i);
                    bool valid;
                    if (i == 9 || i == 14 || i == 19 || i == 24) {
                        valid = c == '-';
                    } else if (i == 0) {
                        valid = c == '{';
                    } else if (i == 37) {
                        valid = c == '}';
                    } else {
                        valid = (c >= '0' && c <= '9') ||
                                (c >= 'a' && c <= 'f') ||
                                (c >= 'A' && c <= 'F');
                    }

                    if (!valid) {
                        err->append(QString("Wrong MSI GUID for %1: %3").
                                arg(a->toString()).
                                arg(a->msiGUID));
                        break;
                    }
                }
            }
        }
    }

    if (err->isEmpty())
        return a;
    else {
        delete a;
        return 0;
    }
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

PackageVersionFile* Repository::createPackageVersionFile(QDomElement* e,
        QString* err)
{
    *err = "";

    QString path = e->attribute("path");
    QString content = e->firstChild().nodeValue();
    PackageVersionFile* a = new PackageVersionFile(path, content);

    return a;
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

Dependency* Repository::createDependency(QDomElement* e)
{
    // qDebug() << "Repository::createDependency";

    QString package = e->attribute("package").trimmed();

    Dependency* d = new Dependency();
    d->package = package;

    d->var = XMLUtils::getTagContent(*e, "variable");

    if (d->setVersions(e->attribute("versions")))
        return d;
    else {
        delete d;
        return 0;
    }
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

/* TODO
QString Repository::writeTo(const QString& filename) const
{
    QString r;

    QDomDocument doc;
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    XMLUtils::addTextTag(root, "spec-version", "3");

    for (int i = 0; i < getPackageCount(); i++) {
        Package* p = packages.at(i);
        QDomElement package = doc.createElement("package");
        p->saveTo(package);
        root.appendChild(package);
    }

    for (int i = 0; i < getPackageVersionCount(); i++) {
        PackageVersion* pv = getPackageVersion(i);
        QDomElement version = doc.createElement("version");
        version.setAttribute("name", pv->version.getVersionString());
        version.setAttribute("package", pv->getPackage()->name);
        if (pv->download.isValid())
            XMLUtils::addTextTag(version, "url", pv->download.toString());
        root.appendChild(version);
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
*/

void Repository::detectWindows()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    Version v;
    v.setVersion(osvi.dwMajorVersion, osvi.dwMinorVersion,
            osvi.dwBuildNumber);

    clearExternallyInstalled("com.microsoft.Windows");
    clearExternallyInstalled("com.microsoft.Windows32");
    clearExternallyInstalled("com.microsoft.Windows64");

    PackageVersion* pv = findOrCreatePackageVersion("com.microsoft.Windows", v);
    pv->setPath(WPMUtils::getWindowsDir());
    pv->setExternal(true);
    if (WPMUtils::is64BitWindows()) {
        pv = findOrCreatePackageVersion("com.microsoft.Windows64", v);
        pv->setPath(WPMUtils::getWindowsDir());
        pv->setExternal(true);
    } else {
        pv = findOrCreatePackageVersion("com.microsoft.Windows32", v);
        pv->setPath(WPMUtils::getWindowsDir());
        pv->setExternal(true);
    }
}

void Repository::recognize(Job* job)
{
    job->setProgress(0);

    if (!job->isCancelled()) {
        job->setHint("Detecting Windows");
        detectWindows();
        job->setProgress(0.1);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JRE");
        detectJRE(false);
        if (WPMUtils::is64BitWindows())
            detectJRE(true);
        job->setProgress(0.4);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JDK");
        detectJDK(false);
        if (WPMUtils::is64BitWindows())
            detectJDK(true);
        job->setProgress(0.7);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        detectDotNet();
        job->setProgress(0.8);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Control Panel programs");
        detectControlPanelPrograms();
        job->setProgress(0.85);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting MSI packages");
        detectMSIProducts();
        job->setProgress(0.9);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Windows Installer");
        detectMicrosoftInstaller();
        job->setProgress(0.95);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Microsoft Core XML Services (MSXML)");
        detectMSXML();
        job->setProgress(0.97);
    }

    if (!job->isCancelled()) {
        job->setHint("Updating NPACKD_CL");
        updateNpackdCLEnvVar();
        job->setProgress(1);
    }

    job->complete();
}

void Repository::detectJRE(bool w64bit)
{
    clearExternallyInstalled(w64bit ? "com.oracle.JRE64" : "com.oracle.JRE");

    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    WindowsRegistry jreWR;
    QString err = jreWR.open(HKEY_LOCAL_MACHINE,
            "Software\\JavaSoft\\Java Runtime Environment", !w64bit, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = jreWR.list(&err);
        for (int i = 0; i < entries.count(); i++) {
            QString v_ = entries.at(i);
            v_ = v_.replace('_', '.');
            Version v;
            if (!v.setVersion(v_) || v.getNParts() <= 2)
                continue;

            WindowsRegistry wr;
            err = wr.open(jreWR, entries.at(i), KEY_READ);
            if (!err.isEmpty())
                continue;

            QString path = wr.get("JavaHome", &err);
            if (!err.isEmpty())
                continue;

            QDir d(path);
            if (!d.exists())
                continue;

            PackageVersion* pv = findOrCreatePackageVersion(
                    w64bit ? "com.oracle.JRE64" :
                    "com.oracle.JRE", v);
            if (!pv->installed()) {
                pv->setPath(path);
                pv->setExternal(true);
            }
        }
    }
}

void Repository::detectJDK(bool w64bit)
{
    QString p = w64bit ? "com.oracle.JDK64" : "com.oracle.JDK";

    clearExternallyInstalled(p);

    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "Software\\JavaSoft\\Java Development Kit",
            !w64bit, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = wr.list(&err);
        if (err.isEmpty()) {
            for (int i = 0; i < entries.count(); i++) {
                QString v_ = entries.at(i);
                WindowsRegistry r;
                err = r.open(wr, v_, KEY_READ);
                if (!err.isEmpty())
                    continue;

                v_.replace('_', '.');
                Version v;
                if (!v.setVersion(v_) || v.getNParts() <= 2)
                    continue;

                QString path = r.get("JavaHome", &err);
                if (!err.isEmpty())
                    continue;

                QDir d(path);
                if (!d.exists())
                    continue;

                PackageVersion* pv = findOrCreatePackageVersion(
                        p, v);
                if (!pv->installed()) {
                    pv->setPath(path);
                    pv->setExternal(true);
                }
            }
        }
    }
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
    }
    return pv;
}

void Repository::clearExternallyInstalled(QString package)
{
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        if (pv->isExternal() && pv->package == package) {
            pv->setPath("");
        }
    }
}

void Repository::detectOneDotNet(const WindowsRegistry& wr,
        const QString& keyName)
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
            QString err;
            QString value_ = wr.get("Version", &err);
            if (err.isEmpty() && v.setVersion(value_)) {
                found = true;
            }
        } else {
            WindowsRegistry r;
            QString err = r.open(wr, "Full", KEY_READ);
            if (err.isEmpty()) {
                QString value_ = r.get("Version", &err);
                if (err.isEmpty() && v.setVersion(value_)) {
                    found = true;
                }
            }
        }
    }

    if (found) {
        PackageVersion* pv = findOrCreatePackageVersion(packageName, v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
}

void Repository::detectControlPanelPrograms()
{
    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            KEY_READ
    );
    if (err.isEmpty()) {
        QStringList entries = wr.list(&err);
        for (int i = 0; i < entries.count(); i++) {
            WindowsRegistry k;
            err = k.open(wr, entries.at(i), KEY_READ);
            if (err.isEmpty()) {
                QString title = k.get("DisplayName", &err);
                if (!err.isEmpty() || title.isEmpty())
                    title = entries.at(i);
                QString package = entries.at(i);
                package.replace('.', '_');
                package = WPMUtils::makeValidFullPackageName(
                        "control-panel." + package);
                Package* p = this->findPackage(package);
                if (!p) {
                    p = new Package(package, title);
                    this->packages.append(p);
                }
                p->title = title;
                p->description = "[Control Panel] " + p->title;

                Version version;
                QString version_ = k.get("DisplayVersion", &err);
                if (err.isEmpty())
                    version.setVersion(version_);

                PackageVersion* pv = this->findPackageVersion(package, version);
                if (!pv) {
                    pv = new PackageVersion(package);
                    pv->detectionInfo = "control-panel:" + entries.at(i);
                    pv->version = version;
                    this->packageVersions.append(pv);
                }

                pv->setExternal(true);

                QString dir = WPMUtils::getWindowsDir();
                pv->setPath(dir);
            }
        }
    }
}

QStringList Repository::getAllInstalledPackagePaths() const
{
    QString windowsDir = WPMUtils::getWindowsDir();

    QStringList r;
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = (PackageVersion*) this->packageVersions.at(i);
        if (pv->installed()) {
            QString dir = pv->getPath();
            if (!WPMUtils::pathEquals(dir, windowsDir)) {
                r.append(dir);
            }
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

void Repository::detectMSIProducts()
{
    QStringList all = WPMUtils::findInstalledMSIProducts();
    // qDebug() << all.at(0);

    QStringList packagePaths = getAllInstalledPackagePaths();

    for (int i = 0; i < all.count(); i++) {
        QString guid = all.at(i);

        PackageVersion* pv = findPackageVersionByMSIGUID(guid);
        if (!pv) {
            QString package = "msi." + guid.mid(1, 36);

            QString err;
            QString version_ = WPMUtils::getMSIProductAttribute(guid,
                    INSTALLPROPERTY_VERSIONSTRING, &err);
            Version version;
            if (err.isEmpty()) {
                if (!version.setVersion(version_))
                    version.setVersion(1, 0);
                else
                    version.normalize();
            }

            pv = this->findPackageVersion(package, version);
            if (!pv) {
                pv = new PackageVersion(package);
                pv->version = version;
                this->packageVersions.append(pv);
            }
        }

        Package* p = findPackage(pv->package);
        if (!p) {
            QString err;
            QString title = WPMUtils::getMSIProductName(guid, &err);
            if (!err.isEmpty())
                title = guid;

            p = new Package(pv->package, title);
            this->packages.append(p);

            p->description = "[MSI database] " + p->title + " GUID: " + guid;
        }

        if (p->url.isEmpty()) {
            QString err;
            QString url = WPMUtils::getMSIProductAttribute(guid,
                    INSTALLPROPERTY_URLINFOABOUT, &err);
            if (err.isEmpty() && QUrl(url).isValid())
                p->url = url;
        }

        if (p->url.isEmpty()) {
            QString err;
            QString url = WPMUtils::getMSIProductAttribute(guid,
                    INSTALLPROPERTY_HELPLINK, &err);
            if (err.isEmpty() && QUrl(url).isValid())
                p->url = url;
        }

        if (!pv->installed()) {
            QDir d;
            QString err;
            QString dir = WPMUtils::getMSIProductLocation(guid, &err);
            if (!err.isEmpty())
                dir = "";

            if (!dir.isEmpty()) {
                dir = WPMUtils::normalizePath(dir);

                bool pathUsed = false;
                for (int j = 0; j < packagePaths.count(); j++) {
                    if (WPMUtils::pathEquals(dir, packagePaths.at(j)) ||
                            WPMUtils::isUnder(dir, packagePaths.at(j))) {
                        pathUsed = true;
                        break;
                    }
                }

                if (pathUsed)
                    dir = "";
            }

            pv->detectionInfo = "msi:" + guid;
            if (dir.isEmpty() || !d.exists(dir)) {
                dir = WPMUtils::getInstallationDirectory() +
                        "\\NpackdDetected\\" +
                WPMUtils::makeValidFilename(p->title, '_');
                if (d.exists(dir)) {
                    dir = WPMUtils::findNonExistingFile(dir + "-" +
                            pv->version.getVersionString() + "%1");
                }
                d.mkpath(dir);
            }

            if (d.exists(dir)) {
                if (d.mkpath(dir + "\\.Npackd")) {
                    QFile file(dir + "\\.Npackd\\Uninstall.bat");
                    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QTextStream stream(&file);
                        stream.setCodec("UTF-8");
                        QString txt = "msiexec.exe /qn /norestart /Lime .Npackd\\UninstallMSI.log /x" + guid + "\r\n" +
                                "set err=%errorlevel%" + "\r\n" +
                                "type .Npackd\\UninstallMSI.log" + "\r\n" +
                                "rem 3010=restart required" + "\r\n" +
                                "if %err% equ 3010 exit 0" + "\r\n" +
                                "if %err% neq 0 exit %err%" + "\r\n";

                        stream << txt;
                        file.close();
                        pv->setPath(dir);
                        pv->setExternal(false);
                    }
                }
            }
        }
    }
}

void Repository::detectDotNet()
{
    // http://stackoverflow.com/questions/199080/how-to-detect-what-net-framework-versions-and-service-packs-are-installed

    clearExternallyInstalled("com.microsoft.DotNetRedistributable");

    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\NET Framework Setup\\NDP", false, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = wr.list(&err);
        if (err.isEmpty()) {
            for (int i = 0; i < entries.count(); i++) {
                QString v_ = entries.at(i);
                Version v;
                if (v_.startsWith("v") && v.setVersion(
                        v_.right(v_.length() - 1))) {
                    WindowsRegistry r;
                    err = r.open(wr, v_, KEY_READ);
                    if (err.isEmpty())
                        detectOneDotNet(r, v_);
                }
            }
        }
    }
}

void Repository::detectMicrosoftInstaller()
{
    clearExternallyInstalled("com.microsoft.WindowsInstaller");

    Version v = WPMUtils::getDLLVersion("MSI.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.WindowsInstaller", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
}

void Repository::detectMSXML()
{
    clearExternallyInstalled("com.microsoft.MSXML");

    Version v = WPMUtils::getDLLVersion("msxml.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
    v = WPMUtils::getDLLVersion("msxml2.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
    v = WPMUtils::getDLLVersion("msxml3.dll");
    if (v.compare(nullNull) > 0) {
        v.prepend(3);
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
    v = WPMUtils::getDLLVersion("msxml4.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
    v = WPMUtils::getDLLVersion("msxml5.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
    v = WPMUtils::getDLLVersion("msxml6.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
            pv->setExternal(true);
        }
    }
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

void Repository::scanPre1_15Dir(bool exact)
{
    QDir aDir(WPMUtils::getInstallationDirectory());
    if (!aDir.exists())
        return;

    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString err;
    WindowsRegistry packagesWR = machineWR.createSubKey(
            "SOFTWARE\\Npackd\\Npackd\\Packages", &err);
    if (!err.isEmpty())
        return;

    QFileInfoList entries = aDir.entryInfoList(
            QDir::NoDotAndDotDot | QDir::Dirs);
    int count = entries.size();
    QString dirPath = aDir.absolutePath();
    dirPath.replace('/', '\\');
    for (int idx = 0; idx < count; idx++) {
        QFileInfo entryInfo = entries[idx];
        QString name = entryInfo.fileName();
        int pos = name.lastIndexOf("-");
        if (pos > 0) {
            QString packageName = name.left(pos);
            QString versionName = name.right(name.length() - pos - 1);

            if (Package::isValidName(packageName)) {
                Version version;
                if (version.setVersion(versionName)) {
                    if (!exact || this->findPackage(packageName)) {
                        // using getVersionString() here to fix a bug in earlier
                        // versions where version numbers were not normalized
                        WindowsRegistry wr = packagesWR.createSubKey(
                                packageName + "-" + version.getVersionString(),
                                &err);
                        if (err.isEmpty()) {
                            wr.set("Path", dirPath + "\\" +
                                    name);
                            wr.setDWORD("External", 0);
                        }
                    }
                }
            }
        }
    }
}

QString Repository::computeNpackdCLEnvVar()
{
    QString v;
    PackageVersion* pv = findNewestInstalledPackageVersion(
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

void Repository::detectPre_1_15_Packages()
{
    QString regPath = "SOFTWARE\\Npackd\\Npackd";
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString err;
    WindowsRegistry npackdWR = machineWR.createSubKey(regPath, &err);
    if (err.isEmpty()) {
        DWORD b = npackdWR.getDWORD("Pre1_15DirScanned", &err);
        if (!err.isEmpty() || b != 1) {
            // store the references to packages in the old format (< 1.15)
            // in the registry
            scanPre1_15Dir(false);
            npackdWR.setDWORD("Pre1_15DirScanned", 1);
        }
    }
}

void Repository::readRegistryDatabase()
{
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false, KEY_READ);

    QString err;
    WindowsRegistry packagesWR;
    err = packagesWR.open(machineWR,
            "SOFTWARE\\Npackd\\Npackd\\Packages", KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = packagesWR.list(&err);
        for (int i = 0; i < entries.count(); ++i) {
            QString name = entries.at(i);
            int pos = name.lastIndexOf("-");
            if (pos > 0) {
                QString packageName = name.left(pos);
                if (Package::isValidName(packageName)) {
                    QString versionName = name.right(name.length() - pos - 1);
                    Version version;
                    if (version.setVersion(versionName)) {
                        PackageVersion* pv = findOrCreatePackageVersion(
                                packageName, version);
                        pv->loadFromRegistry();
                    }
                }
            }
        }
    }
}

void Repository::scan(const QString& path, Job* job, int level,
        QStringList& ignore)
{
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
                        QString sha1 = path2sha1[df->path];
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
                pv->setExternal(true);
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

void Repository::reload(Job *job)
{
    job->setHint("Loading repositories");
    Job* d = job->newSubJob(0.75);
    load(d);
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
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier");
        this->detectPre_1_15_Packages();
        job->setProgress(0.4);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Reading registry package database");
        this->readRegistryDatabase();
        job->setProgress(0.5);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting software");
        Job* d = job->newSubJob(0.2);
        this->recognize(d);
        delete d;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier (2)");
        scanPre1_15Dir(true);
        job->setProgress(1);
    }

    job->complete();
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

    qDeleteAll(urls);
    urls.clear();

    job->complete();
}

void Repository::loadOne(QUrl* url, Job* job) {
    job->setHint("Downloading");

    QTemporaryFile* f = 0;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        Job* djob = job->newSubJob(0.90);
        f = Downloader::download(djob, *url, 0, true);
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
            job->setErrorMessage(QString("XML parsing failed: %1").
                                 arg(errMsg));
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

QList<QUrl*> Repository::getRepositoryURLs()
{
    QList<QUrl*> r;
    QSettings s1("Npackd", "Npackd");
    int size = s1.beginReadArray("repositories");
    for (int i = 0; i < size; ++i) {
        s1.setArrayIndex(i);
        QString v = s1.value("repository").toString();
        r.append(new QUrl(v));
    }
    s1.endArray();

    if (size == 0) {
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
            }
        }
        setRepositoryURLs(r);
    }
    
    return r;
}

void Repository::setRepositoryURLs(QList<QUrl*>& urls)
{
    QSettings s("Npackd", "Npackd");
    s.beginWriteArray("repositories", urls.count());
    for (int i = 0; i < urls.count(); ++i) {
        s.setArrayIndex(i);
        s.setValue("repository", urls.at(i)->toString());
    }
    s.endArray();
}

Repository* Repository::getDefault()
{
    return &def;
}


