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

Repository Repository::def;

Repository::Repository(): QObject()
{
    /*
    // allow access to the \Packages for all users
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString err;
    WindowsRegistry packagesWR = machineWR.createSubKey(
            "SOFTWARE\\Npackd\\Npackd\\Packages", &err);
    if (err.isEmpty()) {
        // ignore errors
        packagesWR.allowReadAccessToEverybody();
    }
    */

    addWellKnownPackages();
}

QList<PackageVersion*> Repository::getInstalled()
{
    QList<PackageVersion*> ret;

    Repository* r = Repository::getDefault();
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        if (pv->installed()) {
            ret.append(pv);
        }
    }

    return ret;
}

Digraph* Repository::createInstalledGraph()
{
    Digraph* installedGraph = new Digraph();
    Node* user = installedGraph->addNode(0);
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        if (pv->installed()) {
            Node* n = installedGraph->addNode(pv);
            user->to.append(n);
        }
    }

    for (int i = 1; i < installedGraph->nodes.count(); i++) {
        Node* n = installedGraph->nodes.at(i);
        PackageVersion* pv = (PackageVersion*) n->userData;
        for (int j = 0; j < pv->dependencies.count(); j++) {
            Dependency* d = pv->dependencies.at(j);
            PackageVersion* pv2 = d->findHighestInstalledMatch();
            Node* n2 = installedGraph->findNodeByUserData(pv2);
            if (!n2) {
                n2 = installedGraph->addNode(pv2);
            }
            n->to.append(n2);
        }
    }

    return installedGraph;
}

Repository::~Repository()
{
    qDeleteAll(this->packages);
    qDeleteAll(this->packageVersions);
    qDeleteAll(this->licenses);
}

PackageVersion* Repository::findNewestInstallablePackageVersion(
        const QString &name)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == name) {
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

DetectFile* Repository::createDetectFile(QDomElement* e)
{
    DetectFile* a = new DetectFile();
    QDomNodeList nl = e->elementsByTagName("path");
    if (nl.count() != 0) {
        a->path = nl.at(0).firstChild().nodeValue().trimmed();
        a->path.replace('/', '\\');
    }
    nl = e->elementsByTagName("sha1");
    if (nl.count() != 0)
        a->sha1 = nl.at(0).firstChild().nodeValue().trimmed().toLower();

    return a;
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
    a->version.normalize();

    QDomNodeList sha1 = e->elementsByTagName("sha1");
    if (sha1.count() > 0)
        a->sha1 = sha1.at(0).firstChild().nodeValue().trimmed().toLower();

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

    QDomNodeList detectFiles = e->elementsByTagName("detect-file");
    for (int i = 0; i < detectFiles.count(); i++) {
        QDomElement e = detectFiles.at(i).toElement();
        a->detectFiles.append(createDetectFile(&e));
    }

    QDomNodeList deps = e->elementsByTagName("dependency");
    for (int i = 0; i < deps.count(); i++) {
        QDomElement e = deps.at(i).toElement();
        Dependency* d = createDependency(&e);
        if (d)
            a->dependencies.append(d);
    }

    QDomNodeList guid = e->elementsByTagName("detect-msi");
    if (guid.count() > 0)
        a->msiGUID = guid.at(0).firstChild().nodeValue().trimmed().toLower();

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
    nl = e->elementsByTagName("icon");
    if (nl.count() != 0) {
        a->icon = nl.at(0).firstChild().nodeValue().trimmed();
    }
    nl = e->elementsByTagName("license");
    if (nl.count() != 0) {
        a->license = nl.at(0).firstChild().nodeValue();
    }

    return a;
}

PackageVersionFile* Repository::createPackageVersionFile(QDomElement* e)
{
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
    if (d->setVersions(e->attribute("versions")))
        return d;
    else {
        delete d;
        return 0;
    }

    // qDebug() << d->toString();

    return d;
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

void Repository::detectMSIProducts()
{
    QStringList all = WPMUtils::findInstalledMSIProducts();

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* pv = this->packageVersions.at(i);
        if (pv->msiGUID.length() == 38) {
            if (all.contains(pv->msiGUID)) {
                if (!pv->installed() || pv->isExternal()) {
                    QString err;
                    QString p = WPMUtils::getMSIProductLocation(
                            pv->msiGUID, &err);
                    if (p.isEmpty() || !err.isEmpty())
                        p = WPMUtils::getWindowsDir();

                    pv->setPath(p);
                    pv->setExternal(true);
                }
            } else {
                pv->setPath("");
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
        pv->locked = true;
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
        pv->locked = false;
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
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                    (LPARAM) L"Environment",
                    0, 1000, 0);
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
        f = Downloader::download(djob, *url);
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
                if (specVersion_.compare(Version(3,0)) >= 0)
                    job->setErrorMessage(QString(
                            "Incompatible repository specification version: %1").
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
                    PackageVersion* pv = createPackageVersion(&e);
                    if (this->findPackageVersion(pv->package, pv->version))
                        delete pv;
                    else {
                        this->packageVersions.append(pv);
                    }
                } else if (e.nodeName() == "package") {
                    Package* p = createPackage(&e);
                    if (this->findPackage(p->name))
                        delete p;
                    else
                        this->packages.append(p);
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
        if (pv->locked) {
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


