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

void Repository::detectWindows()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    Version v;
    v.setVersion(osvi.dwMajorVersion, osvi.dwMinorVersion,
            osvi.dwBuildNumber);

    PackageVersion* pv = findOrCreatePackageVersion("com.microsoft.Windows", v);
    pv->setPath(WPMUtils::getWindowsDir());
    if (WPMUtils::is64BitWindows()) {
        pv = findOrCreatePackageVersion("com.microsoft.Windows64", v);
        pv->setPath(WPMUtils::getWindowsDir());
    } else {
        pv = findOrCreatePackageVersion("com.microsoft.Windows32", v);
        pv->setPath(WPMUtils::getWindowsDir());
    }
}

void Repository::detect(Job* job)
{
    job->setProgress(0);

    if (!job->isCancelled()) {
        job->setHint("Detecting Windows");
        detectWindows();
        job->setProgress(0.01);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JRE");
        detectJRE(false);
        if (WPMUtils::is64BitWindows())
            detectJRE(true);
        job->setProgress(0.1);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JDK");
        detectJDK(false);
        if (WPMUtils::is64BitWindows())
            detectJDK(true);
        job->setProgress(0.2);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        detectDotNet();
        job->setProgress(0.3);
    }

    // MSI package detection should happen before the detection for
    // control panel programs
    if (!job->isCancelled()) {
        job->setHint("Detecting MSI packages");
        detectMSIProducts();
        job->setProgress(0.5);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Control Panel programs");
        detectControlPanelPrograms();
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
            }
        }
    }
}

void Repository::detectJDK(bool w64bit)
{
    QString p = w64bit ? "com.oracle.JDK64" : "com.oracle.JDK";

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
        this->package2versions.insert(package, pv);
    }
    return pv;
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
        }
    }
}

void Repository::detectControlPanelPrograms()
{
    // TODO: each detectControlPanelProgramsFrom may also change this list
    QStringList packagePaths = getAllInstalledPackagePaths();

    QStringList foundDetectionInfos;

    detectControlPanelProgramsFrom(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            false, &packagePaths, &foundDetectionInfos
    );
    if (WPMUtils::is64BitWindows())
        detectControlPanelProgramsFrom(HKEY_LOCAL_MACHINE,
                "SOFTWARE\\WoW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                false, &packagePaths, &foundDetectionInfos
        );
    detectControlPanelProgramsFrom(HKEY_CURRENT_USER,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            false, &packagePaths, &foundDetectionInfos
    );
    if (WPMUtils::is64BitWindows())
        detectControlPanelProgramsFrom(HKEY_CURRENT_USER,
                "SOFTWARE\\WoW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                false, &packagePaths, &foundDetectionInfos
        );

    // remove uninstalled packages
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        if (pv->detectionInfo.indexOf("control-panel:") == 0 &&
                pv->installed() &&
                !foundDetectionInfos.contains(pv->detectionInfo)) {
            // qDebug() << "uninstall " << pv->package << " " <<
            //         pv->version.getVersionString();
            pv->setPath("");
        }
    }
}

void Repository::detectControlPanelProgramsFrom(HKEY root,
        const QString& path, bool useWoWNode, QStringList* packagePaths,
        QStringList* foundDetectionInfos) {
    WindowsRegistry wr;
    QString err;
    err = wr.open(root, path, useWoWNode, KEY_READ);
    if (err.isEmpty()) {
        QString fullPath;
        if (root == HKEY_CLASSES_ROOT)
            fullPath = "HKEY_CLASSES_ROOT";
        else if (root == HKEY_CURRENT_USER)
            fullPath = "HKEY_CURRENT_USER";
        else if (root == HKEY_LOCAL_MACHINE)
            fullPath = "HKEY_LOCAL_MACHINE";
        else if (root == HKEY_USERS)
            fullPath = "HKEY_USERS";
        else if (root == HKEY_PERFORMANCE_DATA)
            fullPath = "HKEY_PERFORMANCE_DATA";
        else if (root == HKEY_CURRENT_CONFIG)
            fullPath = "HKEY_CURRENT_CONFIG";
        else if (root == HKEY_DYN_DATA)
            fullPath = "HKEY_DYN_DATA";
        else
            fullPath = QString("%1").arg((uintptr_t) root);
        fullPath += "\\" + path;

        QStringList entries = wr.list(&err);
        for (int i = 0; i < entries.count(); i++) {
            WindowsRegistry k;
            err = k.open(wr, entries.at(i), KEY_READ);
            if (err.isEmpty()) {
                detectOneControlPanelProgram(fullPath + "\\" + entries.at(i),
                    k, entries.at(i), packagePaths, foundDetectionInfos);
            }
        }
    }
}

void Repository::detectOneControlPanelProgram(const QString& registryPath,
        WindowsRegistry& k,
        const QString& keyName, QStringList* packagePaths,
        QStringList* foundDetectionInfos)
{
    QString package = keyName;
    package.replace('.', '_');
    package = WPMUtils::makeValidFullPackageName(
            "control-panel." + package);

    bool versionFound = false;
    Version version;
    QString err;
    QString version_ = k.get("DisplayVersion", &err);
    if (err.isEmpty()) {
        version.setVersion(version_);
        version.normalize();
        versionFound = true;
    }
    if (!versionFound) {
        DWORD major = k.getDWORD("VersionMajor", &err);
        if (err.isEmpty()) {
            DWORD minor = k.getDWORD("VersionMinor", &err);
            if (err.isEmpty())
                version.setVersion(major, minor);
            else
                version.setVersion(major, 0);
            version.normalize();
            versionFound = true;
        }
    }
    if (!versionFound) {
        QString major = k.get("VersionMajor", &err);
        if (err.isEmpty()) {
            QString minor = k.get("VersionMinor", &err);
            if (err.isEmpty()) {
                if (version.setVersion(major)) {
                    versionFound = true;
                    version.normalize();
                }
            } else {
                if (version.setVersion(major + "." + minor)) {
                    versionFound = true;
                    version.normalize();
                }
            }
        }
    }
    if (!versionFound) {
        QString displayName = k.get("DisplayName", &err);
        if (err.isEmpty()) {
            QStringList parts = displayName.split(' ');
            if (parts.count() > 1 && parts.last().contains('.')) {
                version.setVersion(parts.last());
                version.normalize();
                versionFound = true;
            }
        }
    }

    PackageVersion* pv = this->findPackageVersion(package, version);
    if (!pv) {
        pv = new PackageVersion(package);
        pv->detectionInfo = "control-panel:" + registryPath;
        pv->version = version;
        this->packageVersions.append(pv);
        this->package2versions.insert(package, pv);
    }

    Package* p = this->findPackage(package);
    if (!p) {
        p = new Package(package, package);
        this->packages.append(p);
    }

    QString title = k.get("DisplayName", &err);
    if (!err.isEmpty() || title.isEmpty())
        title = keyName;
    p->title = title;
    p->description = "[Control Panel] " + p->title;

    QString url = k.get("URLInfoAbout", &err);
    if (!err.isEmpty() || url.isEmpty() || !QUrl(url).isValid())
        url = "";
    if (url.isEmpty())
        url = k.get("URLUpdateInfo", &err);
    if (!err.isEmpty() || url.isEmpty() || !QUrl(url).isValid())
        url = "";
    p->url = url;

    QDir d;

    bool useThisEntry = !pv->installed();

    QString uninstall;
    if (useThisEntry) {
        uninstall = k.get("QuietUninstallString", &err);
        if (!err.isEmpty())
            uninstall = "";
        if (uninstall.isEmpty())
            uninstall = k.get("UninstallString", &err);
        if (!err.isEmpty())
            uninstall = "";

        // some programs store in UninstallString the complete path to
        // the uninstallation program with spaces
        if (!uninstall.isEmpty() && uninstall.contains(" ") &&
                !uninstall.contains("\"") &&
                d.exists(uninstall))
            uninstall = "\"" + uninstall + "\"";

        if (uninstall.trimmed().isEmpty())
            useThisEntry = false;
    }

    // already detected as an MSI package
    if (uninstall.length() == 14 + 38 &&
            (uninstall.indexOf("MsiExec.exe /X", 0, Qt::CaseInsensitive) == 0 ||
            uninstall.indexOf("MsiExec.exe /I", 0, Qt::CaseInsensitive) == 0) &&
            WPMUtils::validateGUID(uninstall.right(38)) == "") {
        useThisEntry = false;
    }

    QString dir;
    if (useThisEntry) {
        dir = k.get("InstallLocation", &err);
        if (!err.isEmpty())
            dir = "";

        if (dir.isEmpty() && !uninstall.isEmpty()) {
            QStringList params = WPMUtils::parseCommandLine(uninstall, &err);
            if (err.isEmpty() && params.count() > 0 && d.exists(params[0])) {
                dir = WPMUtils::parentDirectory(params[0]);
            } /* DEBUG else {
                qDebug() << "cannot parse " << uninstall << " " << err <<
                        " " << params.count();
                if (params.count() > 0)
                    qDebug() << "cannot parse2 " << params[0] << " " <<
                            d.exists(params[0]);
            }*/
        }
    }

    if (useThisEntry) {
        if (!dir.isEmpty()) {
            dir = WPMUtils::normalizePath(dir);
            if (WPMUtils::isUnderOrEquals(dir, *packagePaths))
                useThisEntry = false;
        }
    }

    if (useThisEntry) {
        if (dir.isEmpty()) {
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
                    QString txt = uninstall + "\r\n";

                    stream << txt;
                    file.close();
                    pv->setPath(dir);
                }
            }
            packagePaths->append(dir);
        }

        foundDetectionInfos->append(pv->detectionInfo);
    }
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
                this->package2versions.insert(package, pv);
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
                if (WPMUtils::isUnderOrEquals(dir, packagePaths))
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
                    }
                }
            }
        }
    }

    // remove uninstalled MSI packages
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        if (pv->detectionInfo.length() == 4 + 38 &&
                pv->detectionInfo.left(4) == "msi:" &&
                pv->installed() &&
                !all.contains(pv->detectionInfo.right(38))) {
            // DEBUG qDebug() << "uninstall " << pv->package << " " <<
            // DEBUG         pv->version.getVersionString();
            pv->setPath("");
        }
    }
}

void Repository::detectDotNet()
{
    // http://stackoverflow.com/questions/199080/how-to-detect-what-net-framework-versions-and-service-packs-are-installed

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
    Version v = WPMUtils::getDLLVersion("MSI.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.WindowsInstaller", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
}

void Repository::detectMSXML()
{
    Version v = WPMUtils::getDLLVersion("msxml.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml2.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml3.dll");
    if (v.compare(nullNull) > 0) {
        v.prepend(3);
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());

        }
    }
    v = WPMUtils::getDLLVersion("msxml4.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml5.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml6.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
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
        this->detect(d);
        delete d;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier (2)");
        scanPre1_15Dir(true);
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

    QList<QUrl*> urls = getRepositoryURLs();
    if (urls.count() > 0) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Repository %1 of %2").arg(i + 1).
                         arg(urls.count()));
            Job* s = job->newSubJob(0.9 / urls.count());
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
        job->setProgress(0.9);
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


