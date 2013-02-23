#include "installedpackages.h"

#include <windows.h>
#include <QDebug>
#include <msi.h>

#include "windowsregistry.h"
#include "package.h"
#include "version.h"
#include "packageversion.h"
#include "repository.h"
#include "wpmutils.h"

InstalledPackages InstalledPackages::def;

InstalledPackages* InstalledPackages::getDefault()
{
    return &def;
}

InstalledPackages::InstalledPackages()
{
}

InstalledPackageVersion* InstalledPackages::find(const QString& package,
        const Version& version) const
{
    return this->data.value(package + "/" + version.getVersionString());
}

InstalledPackageVersion* InstalledPackages::findOrCreate(const QString& package,
        const Version& version)
{
    QString key = package + "/" + version.getVersionString();
    InstalledPackageVersion* r = this->data.value(key);
    if (!r) {
        r = new InstalledPackageVersion(package, version, "");
        this->data.insert(key, r);

        // qDebug() << "InstalledPackages::findOrCreate " << package;
        r->save();
    }
    return r;
}

QString InstalledPackages::setPackageVersionPath(const QString& package,
        const Version& version,
        const QString& directory)
{
    QString err;

    InstalledPackageVersion* ipv = this->find(package, version);
    if (!ipv) {
        ipv = new InstalledPackageVersion(package, version, directory);
        this->data.insert(package + "/" + version.getVersionString(), ipv);
        err = ipv->save();
    } else {
        ipv->setPath(directory);
    }

    // TODO: PackageVersion::emitStatusChanged();

    return err;
}

void InstalledPackages::readRegistryDatabase()
{
    this->data.clear();

    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false, KEY_READ);

    Repository* rep = Repository::getDefault();

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
                        rep->findOrCreatePackageVersion(
                                packageName, version);
                        InstalledPackageVersion* ipv = this->find(packageName, version);
                        if (!ipv) {
                            ipv = new InstalledPackageVersion(packageName, version, "");
                            this->data.insert(packageName + "/" + version.getVersionString(),
                                    ipv);
                        }

                        // qDebug() << "loading " << packageName << ":" << version.getVersionString();

                        ipv->loadFromRegistry();
                    }
                }
            }
        }
    }
}

void InstalledPackages::detect(Job* job)
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
        Repository* rep = Repository::getDefault();
        rep->updateNpackdCLEnvVar();
        job->setProgress(1);
    }

    job->complete();
}

void InstalledPackages::detectJRE(bool w64bit)
{
    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    Repository* rep = Repository::getDefault();
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

            PackageVersion* pv = rep->findOrCreatePackageVersion(
                    w64bit ? "com.oracle.JRE64" :
                    "com.oracle.JRE", v);
            if (!pv->installed()) {
                pv->setPath(path);
            }
        }
    }
}

void InstalledPackages::detectJDK(bool w64bit)
{
    QString p = w64bit ? "com.oracle.JDK64" : "com.oracle.JDK";

    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    Repository* rep = Repository::getDefault();
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

                PackageVersion* pv = rep->findOrCreatePackageVersion(
                        p, v);
                if (!pv->installed()) {
                    pv->setPath(path);
                }
            }
        }
    }
}

void InstalledPackages::detectWindows()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    Version v;
    v.setVersion(osvi.dwMajorVersion, osvi.dwMinorVersion,
            osvi.dwBuildNumber);

    Repository* rep = Repository::getDefault();
    PackageVersion* pv = rep->findOrCreatePackageVersion("com.microsoft.Windows", v);
    pv->setPath(WPMUtils::getWindowsDir());
    if (WPMUtils::is64BitWindows()) {
        pv = rep->findOrCreatePackageVersion("com.microsoft.Windows64", v);
        pv->setPath(WPMUtils::getWindowsDir());
    } else {
        pv = rep->findOrCreatePackageVersion("com.microsoft.Windows32", v);
        pv->setPath(WPMUtils::getWindowsDir());
    }
}

void InstalledPackages::detectOneDotNet(const WindowsRegistry& wr,
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
        Repository* rep = Repository::getDefault();
        PackageVersion* pv = rep->findOrCreatePackageVersion(packageName, v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
}

void InstalledPackages::detectControlPanelPrograms()
{
    Repository* rep = Repository::getDefault();
    QStringList packagePaths = rep->getAllInstalledPackagePaths();

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
    QMapIterator<QString, InstalledPackageVersion*> i(data);
    while (i.hasNext()) {
        i.next();
        InstalledPackageVersion* ipv = i.value();
        if (ipv->detectionInfo.indexOf("control-panel:") == 0 &&
                ipv->installed() &&
                !foundDetectionInfos.contains(ipv->detectionInfo)) {
            qDebug() << "control-panel package removed: " << ipv->package;
            ipv->setPath("");
        }
    }
}

void InstalledPackages::detectControlPanelProgramsFrom(HKEY root,
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

void InstalledPackages::detectOneControlPanelProgram(const QString& registryPath,
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

    Repository* rep = Repository::getDefault();

    //qDebug() << "InstalledPackages::detectOneControlPanelProgram.0";

    PackageVersion* pv = rep->findOrCreatePackageVersion(package, version);
    InstalledPackageVersion* ipv = this->findOrCreate(package, version);
    QString di = "control-panel:" + registryPath;
    ipv->setDetectionInfo(di);
    foundDetectionInfos->append(di);

    Package* p = rep->findPackage(package);
    if (!p) {
        p = new Package(package, package);
        rep->packages.append(p);
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

                    qDebug() << "InstalledPackages::detectOneControlPanelProgram "
                            "setting path for " << pv->toString() << " to" << dir;
                    pv->setPath(dir);
                }
            }
            packagePaths->append(dir);
        }
    }
}

void InstalledPackages::detectMSIProducts()
{
    QStringList all = WPMUtils::findInstalledMSIProducts();
    // qDebug() << all.at(0);

    Repository* rep = Repository::getDefault();
    QStringList packagePaths = rep->getAllInstalledPackagePaths();

    for (int i = 0; i < all.count(); i++) {
        QString guid = all.at(i);

        PackageVersion* pv = rep->findPackageVersionByMSIGUID(guid);
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

            pv = rep->findPackageVersion(package, version);
            if (!pv) {
                pv = new PackageVersion(package);
                pv->version = version;
                rep->packageVersions.append(pv);
                rep->package2versions.insert(package, pv);
            }
        }

        Package* p = rep->findPackage(pv->package);
        if (!p) {
            QString err;
            QString title = WPMUtils::getMSIProductName(guid, &err);
            if (!err.isEmpty())
                title = guid;

            p = new Package(pv->package, title);
            rep->packages.append(p);

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

            InstalledPackageVersion* ipv = this->findOrCreate(pv->package,
                    pv->version);
            ipv->setDetectionInfo("msi:" + guid);
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
    QMapIterator<QString, InstalledPackageVersion*> i(this->data);
    while (i.hasNext()) {
        i.next();
        InstalledPackageVersion* ipv = i.value();
        if (ipv->detectionInfo.length() == 4 + 38 &&
                ipv->detectionInfo.left(4) == "msi:" &&
                ipv->installed() &&
                !all.contains(ipv->detectionInfo.right(38))) {
            // DEBUG qDebug() << "uninstall " << pv->package << " " <<
            // DEBUG         pv->version.getVersionString();
            ipv->setPath("");
        }
    }
}

void InstalledPackages::detectDotNet()
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

void InstalledPackages::detectMicrosoftInstaller()
{
    Version v = WPMUtils::getDLLVersion("MSI.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        Repository* rep = Repository::getDefault();
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.WindowsInstaller", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
}

void InstalledPackages::detectMSXML()
{
    Version v = WPMUtils::getDLLVersion("msxml.dll");
    Version nullNull(0, 0);
    Repository* rep = Repository::getDefault();
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml2.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml3.dll");
    if (v.compare(nullNull) > 0) {
        v.prepend(3);
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());

        }
    }
    v = WPMUtils::getDLLVersion("msxml4.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml5.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
    v = WPMUtils::getDLLVersion("msxml6.dll");
    if (v.compare(nullNull) > 0) {
        PackageVersion* pv = rep->findOrCreatePackageVersion(
                "com.microsoft.MSXML", v);
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getWindowsDir());
        }
    }
}

void InstalledPackages::scanPre1_15Dir(bool exact)
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
                    Repository* rep = Repository::getDefault();
                    if (!exact || rep->findPackage(packageName)) {
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

void InstalledPackages::detectPre_1_15_Packages()
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


