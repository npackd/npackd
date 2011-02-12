#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>
#include <stdlib.h>
#include <time.h>

#include "repository.h"
#include "qurl.h"
#include "QtDebug"
#include "qiodevice.h"
#include "qprocess.h"

#include "quazip.h"
#include "quazipfile.h"
#include "zlib.h"

#include "packageversion.h"
#include "job.h"
#include "downloader.h"
#include "wpmutils.h"
#include "repository.h"
#include "version.h"
#include "windowsregistry.h"

/**
 * Uses the Shell's IShellLink and IPersistFile interfaces
 * to create and store a shortcut to the specified object.
 *
 * @return the result of calling the member functions of the interfaces.
 * @param lpszPathObj - address of a buffer containing the path of the object.
 * @param lpszPathLink - address of a buffer containing the path where the
 *      Shell link is to be stored.
 * @param lpszDesc - address of a buffer containing the description of the
 *      Shell link.
 * @param workingDir working directory
 */
HRESULT CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc,
        LPCWSTR workingDir)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL,
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl);

    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the
        // description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
        psl->SetWorkingDirectory(workingDir);

        // Query IShellLink for the IPersistFile interface for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres)) {
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

PackageVersion::PackageVersion(const QString& package)
{
    this->package = package;
    this->type = 0;
    this->external = false;
}

PackageVersion::PackageVersion()
{
    this->package = "unknown";
    this->type = 0;
    this->external = false;
}

bool PackageVersion::isDirectoryLocked()
{
    QDir d = getDirectory();
    QDateTime now = QDateTime::currentDateTime();
    QString newName = QString("%1-%2").arg(d.absolutePath()).arg(now.toTime_t());

    if (!d.rename(d.absolutePath(), newName)) {
        return true;
    }

    if (!d.rename(newName, d.absolutePath())) {
        return true;
    }

    return false;
}


QString PackageVersion::toString()
{
    return this->getPackageTitle() + " " + this->version.getVersionString();
}

QString PackageVersion::getShortPackageName()
{
    QStringList sl = this->package.split(".");
    return sl.last();
}

PackageVersion::~PackageVersion()
{
    qDeleteAll(this->detectFiles);
    qDeleteAll(this->files);
    qDeleteAll(this->dependencies);
    qDeleteAll(this->fileHandlers);
}

QString PackageVersion::saveInstallationInfo()
{
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE);
    QString err;
    WindowsRegistry wr = machineWR.createSubKey(
            "SOFTWARE\\Npackd\\Npackd\\Packages\\" +
            this->package + "-" + this->version.getVersionString(), &err);
    if (!err.isEmpty())
        return err;

    wr.set("Path", this->path);
    wr.setDWORD("External", this->external ? 1 : 0);
    return "";
}

QString PackageVersion::getFullText()
{
    if (this->fullText.isEmpty()) {
        Repository* rep = Repository::getDefault();
        Package* package = rep->findPackage(this->package);
        QString r = this->package;
        if (package) {
            r.append(" ");
            r.append(package->title);
            r.append(" ");
            r.append(package->description);
        }
        r.append(" ");
        r.append(this->version.getVersionString());

        for (int i = 0; i < this->fileHandlers.count(); i++) {
            FileExtensionHandler* fh = this->fileHandlers.at(i);
            r.append(" ");
            r.append(fh->title);
            r.append(" ");
            r.append(fh->extensions.join(" "));
        }

        this->fullText = r.toLower();
    }
    return this->fullText;
}

bool PackageVersion::installed()
{
    if (this->path.trimmed().isEmpty()) {
        return false;
    } else {
        QDir d = getDirectory();
        d.refresh();
        return d.exists();
    }
}

void PackageVersion::registerFileHandlers()
{
    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows();
    HKEY hkeyClasses;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Classes",
                0, KEY_CREATE_SUB_KEY | KEY_WRITE | (w64bit ? KEY_WOW64_64KEY : 0),
                &hkeyClasses) != ERROR_SUCCESS)
        return;

    QDir dir = getDirectory();
    for (int i = 0; i < this->fileHandlers.count(); i++) {
        FileExtensionHandler* fh = this->fileHandlers.at(i);

        QString prg = fh->program;
        prg.replace('\\', '-').replace('/', '-');
        QString progId = "Npackd-" + this->package + "-" +
                this->version.getVersionString() + "-" +
                prg;

        QString openKey = "Applications\\" + progId +
                          "\\shell\\open";
        QString commandKey = openKey + "\\command";
        HKEY hkey;
        long res = RegCreateKeyExW(hkeyClasses, (WCHAR*) commandKey.utf16(),
                              0, 0, 0, KEY_WRITE, 0, &hkey, 0);
        if (res != ERROR_SUCCESS)
            continue;

        // Windows 7: no "Open With" is shown if the path contains /
        // instead of
        QString cmd = "\"" + dir.absolutePath() + "\\" + fh->program +
                "\" \"%1\"";
        cmd.replace('/', '\\');
        RegSetValueEx(hkey, 0, 0, REG_SZ, (BYTE*) cmd.utf16(),
                cmd.length() * 2 + 2);
        RegCloseKey(hkey);

        res = RegOpenKeyEx(hkeyClasses, (WCHAR*) openKey.utf16(), 0,
                KEY_WRITE, &hkey);
        if (res == ERROR_SUCCESS) {
            QString s = fh->title;
            if (s.contains(this->getPackageTitle()))
                s = s + " (" + this->version.getVersionString() + ")";
            else
                s = s + " (" + this->getPackageTitle() + " " +
                        this->version.getVersionString() + ")";
            RegSetValueEx(hkey, L"FriendlyAppName", 0, REG_SZ, (BYTE*) s.utf16(),
                    s.length() * 2 + 2);
            RegCloseKey(hkey);
        }

        for (int j = 0; j < fh->extensions.count(); j++) {
            // OpenWithProgids does not work on Windows 7 if OpenWithList is
            // also defined:
            // http://msdn.microsoft.com/en-us/library/bb166549.aspx#2
            QString key = fh->extensions.at(j) + "\\OpenWithList\\" + progId;
            res = RegCreateKeyExW(hkeyClasses, (WCHAR*) key.utf16(),
                                       0, 0, 0, KEY_WRITE, 0, &hkey, 0);
            if (res == ERROR_SUCCESS) {
                RegCloseKey(hkey);
            }
        }
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
}

void PackageVersion::unregisterFileHandlers()
{
    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = WPMUtils::is64BitWindows();
    HKEY hkeyClasses;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Classes",
                0, KEY_WRITE | (w64bit ? KEY_WOW64_64KEY : 0),
                &hkeyClasses) != ERROR_SUCCESS)
        return;

    for (int i = 0; i < this->fileHandlers.count(); i++) {
        FileExtensionHandler* fh = this->fileHandlers.at(i);

        QString prg = fh->program;
        prg.replace('\\', '-').replace('/', '-');
        QString progId = "Npackd-" + this->package + "-" +
                this->version.getVersionString() + "-" +
                prg;
        QString key = "Applications\\" + progId;
        WPMUtils::regDeleteTree(hkeyClasses, key);

        for (int j = 0; j < fh->extensions.count(); j++) {
            // OpenWithProgids does not work on Windows 7 if OpenWithList is
            // also defined:
            // http://msdn.microsoft.com/en-us/library/bb166549.aspx#2
            key = fh->extensions.at(j) + "\\OpenWithList\\" + progId;
            WPMUtils::regDeleteTree(hkeyClasses, key);
        }
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
}

void PackageVersion::deleteShortcuts(Job* job,
        bool menu, bool desktop, bool quickLaunch)
{
    if (menu) {
        job->setHint("Start menu");
        QDir d(WPMUtils::getShellDir(CSIDL_STARTMENU));
        deleteShortcuts(d);

        QDir d2(WPMUtils::getShellDir(CSIDL_COMMON_STARTMENU));
        deleteShortcuts(d2);
    }
    job->setProgress(0.33);

    if (desktop) {
        job->setHint("Desktop");
        QDir d3(WPMUtils::getShellDir(CSIDL_DESKTOP));
        deleteShortcuts(d3);

        QDir d4(WPMUtils::getShellDir(CSIDL_COMMON_DESKTOPDIRECTORY));
        deleteShortcuts(d4);
    }
    job->setProgress(0.66);

    if (quickLaunch) {
        job->setHint("Quick launch bar");
        const char* A = "\\Microsoft\\Internet Explorer\\Quick Launch";
        QDir d3(WPMUtils::getShellDir(CSIDL_APPDATA) + A);
        deleteShortcuts(d3);

        QDir d4(WPMUtils::getShellDir(CSIDL_COMMON_APPDATA) + A);
        deleteShortcuts(d4);
    }
    job->setProgress(1);

    job->complete();
}

void PackageVersion::uninstall(Job* job)
{
    if (external) {
        job->setProgress(1);
        job->complete();
        return;
    }

    QDir d = getDirectory();

    QString p = ".Npackd\\Uninstall.bat";
    if (!QFile::exists(d.absolutePath() + "\\" + p)) {
        p = ".WPM\\Uninstall.bat";
    }
    if (QFile::exists(d.absolutePath() + "\\" + p)) {
        job->setHint("Running the uninstallation script (this may take some time)");
        if (!d.exists(".Npackd"))
            d.mkdir(".Npackd");
        Job* sub = job->newSubJob(0.25);
        QStringList env;
        env.append("NPACKD_PACKAGE_NAME");
        env.append(this->package);
        env.append("NPACKD_PACKAGE_VERSION");
        env.append(this->version.getVersionString());
        this->executeFile(sub, p, ".Npackd\\Uninstall.log", env);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(sub->getErrorMessage());
        delete sub;
    }
    job->setProgress(0.25);

    // Uninstall.bat may have deleted some files
    d.refresh();

    if (job->getErrorMessage().isEmpty()) {
        job->setHint("Deleting shortcuts");
        Job* sub = job->newSubJob(0.20);
        deleteShortcuts(sub, true, true, true);
        delete sub;
    }

    if (job->getErrorMessage().isEmpty()) {
        Repository::getDefault()->somethingWasInstalledOrUninstalled();
        if (d.exists()) {
            job->setHint("Deleting files");
            Job* rjob = job->newSubJob(0.55);
            removeDirectory(rjob);
            if (!rjob->getErrorMessage().isEmpty())
                job->setErrorMessage(rjob->getErrorMessage());
            else {
                this->path = "";
                saveInstallationInfo();
            }
            delete rjob;
        } else {
            job->setProgress(1);
        }
    }

    job->complete();
}

void PackageVersion::removeDirectory(Job* job)
{
    QDir d = getDirectory();

    WPMUtils::moveToRecycleBin(d.absolutePath());
    job->setProgress(0.3);

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        d.refresh();
        if (d.exists()) {
            Sleep(5000); // 5 Seconds
            WPMUtils::moveToRecycleBin(d.absolutePath());
        }
        job->setProgress(0.6);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        d.refresh();
        if (d.exists()) {
            Sleep(5000); // 5 Seconds
            QString oldName = d.dirName();
            if (!d.cdUp())
                job->setErrorMessage("Cannot change directory to " +
                        d.absolutePath() + "\\..");
            else {
                QString trash = ".NpackdTrash";
                if (!d.exists(trash)) {
                    if (!d.mkdir(trash))
                        job->setErrorMessage(QString(
                                "Cannot create directory %0\%1").
                                arg(d.absolutePath()).arg(trash));
                }
                if (d.exists(trash)) {
                    QString nn = trash + "\\" + oldName + "_%1";
                    int i = 0;
                    while (true) {
                        QString newName = nn.arg(i);
                        if (!d.exists(newName)) {
                            if (!d.rename(oldName, newName)) {
                                job->setErrorMessage(QString(
                                        "Cannot rename %1 to %2 in %3").
                                        arg(oldName).arg(newName).
                                        arg(d.absolutePath()));
                            }
                            break;
                        } else {
                            i++;
                        }
                    }
                }
            }
        }
        job->setProgress(1);
    }

    job->complete();
}

QDir PackageVersion::getDirectory()
{
    QDir d(this->path);
    return d;
}

QString PackageVersion::planInstallation(QList<PackageVersion*>& installed,
        QList<InstallOperation*>& ops)
{
    QString res;

    for (int i = 0; i < this->dependencies.count(); i++) {
        Dependency* d = this->dependencies.at(i);
        bool depok = false;
        for (int j = 0; j < installed.size(); j++) {
            PackageVersion* pv = installed.at(j);
            if (pv != this && pv->package == d->package &&
                    d->test(pv->version)) {
                depok = true;
                break;
            }
        }
        if (!depok) {
            PackageVersion* pv = d->findBestMatchToInstall();
            if (!pv) {
                res = QString("Unsatisfied dependency: %1").
                           arg(d->toString());
                break;
            } else {
                res = pv->planInstallation(installed, ops);
                if (!res.isEmpty())
                    break;
            }
        }
    }

    if (res.isEmpty()) {
        InstallOperation* io = new InstallOperation();
        io->install = true;
        io->packageVersion = this;
        ops.append(io);
        installed.append(this);
    }

    return res;
}

QString PackageVersion::planUninstallation(QList<PackageVersion*>& installed,
        QList<InstallOperation*>& ops)
{
    // qDebug() << "PackageVersion::planUninstallation()" << this->toString();
    QString res;

    // this loop ensures that all the items in "installed" are processed
    // even if changes in the list were done in nested calls to
    // "planUninstallation"
    while (true) {
        int oldCount = installed.count();
        for (int i = 0; i < installed.count(); i++) {
            PackageVersion* pv = installed.at(i);
            if (pv != this) {
                for (int j = 0; j < pv->dependencies.count(); j++) {
                    Dependency* d = pv->dependencies.at(j);
                    if (d->package == this->package && d->test(this->version)) {
                        int n = 0;
                        for (int k = 0; k < installed.count(); k++) {
                            PackageVersion* pv2 = installed.at(k);
                            if (d->package == pv2->package && d->test(pv2->version)) {
                                n++;
                            }
                            if (n > 1)
                                break;
                        }
                        if (n <= 1) {
                            res = pv->planUninstallation(installed, ops);
                            if (!res.isEmpty())
                                break;
                        }
                    }
                }
                if (!res.isEmpty())
                    break;
            }
        }

        if (oldCount == installed.count() || !res.isEmpty())
            break;
    }

    if (res.isEmpty()) {
        InstallOperation* op = new InstallOperation();
        op->install = false;
        op->packageVersion = this;
        ops.append(op);
        installed.removeOne(this);
    }

    return res;
}

void PackageVersion::downloadTo(Job* job, QString filename)
{
    QString r;

    job->setHint("Downloading");
    QTemporaryFile* f = 0;
    Job* djob = job->newSubJob(0.9);
    f = Downloader::download(djob, this->download);
    if (!djob->getErrorMessage().isEmpty())
        job->setErrorMessage(QString("Download failed: %1").arg(
                djob->getErrorMessage()));
    delete djob;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Computing SHA1");
        r = WPMUtils::sha1(f->fileName());
        job->setProgress(0.95);

        if (!this->sha1.isEmpty() && this->sha1.toLower() != r.toLower()) {
            job->setErrorMessage(QString(
                    "Wrong SHA1: %1 was expected, but %2 found").
                    arg(this->sha1).arg(r));
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString errMsg;
        QString t = filename.replace('/', '\\');
        if (!CopyFileW((WCHAR*) f->fileName().utf16(),
                       (WCHAR*) t.utf16(), false)) {
            WPMUtils::formatMessage(GetLastError(), &errMsg);
            job->setErrorMessage(errMsg);
        } else {
            job->setProgress(1);
        }
    }

    delete f;

    job->complete();
}

QString PackageVersion::getFileExtension()
{
    QString fn = this->download.path();
    QStringList parts = fn.split('/');
    QString file = parts.at(parts.count() - 1);
    int index = file.lastIndexOf('.');
    if (index > 0)
        return file.right(file.length() - index);
    else
        return ".bin";
}

QString PackageVersion::downloadAndComputeSHA1(Job* job)
{
    QString r;

    job->setHint("Downloading");
    QTemporaryFile* f = 0;
    Job* djob = job->newSubJob(0.95);
    f = Downloader::download(djob, this->download);
    if (!djob->getErrorMessage().isEmpty())
        job->setErrorMessage(QString("Download failed: %1").arg(
                djob->getErrorMessage()));
    delete djob;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Computing SHA1");
        r = WPMUtils::sha1(f->fileName());
        job->setProgress(1);
    }

    if (f)
        delete f;

    job->complete();

    return r;
}

QString PackageVersion::getPackageTitle()
{
    Repository* rep = Repository::getDefault();

    QString pn;
    Package* package = rep->findPackage(this->package);
    if (package)
        pn = package->title;
    else
        pn = this->package;
    return pn;
}

bool PackageVersion::createShortcuts(QString *errMsg)
{
    QDir d = getDirectory();
    Package* p = Repository::getDefault()->findPackage(this->package);
    for (int i = 0; i < this->importantFiles.count(); i++) {
        QString ifile = this->importantFiles.at(i);
        QString ift = this->importantFilesTitles.at(i);

        QString path(ifile);
        path.prepend("\\");
        path.prepend(d.absolutePath());
        path = path.replace('/' , '\\');

        QString workingDir = path;
        int pos = workingDir.lastIndexOf('\\');
        workingDir.chop(workingDir.length() - pos);

        QString from = WPMUtils::getShellDir(CSIDL_COMMON_STARTMENU);
        from.append("\\");
        from.append(ift);
        from.append(" (");
        QString pt = this->getPackageTitle();
        if (pt != ift) {
            from.append(pt);
            from.append(" ");
        }
        from.append(this->version.getVersionString());
        from.append(")");
        from.append(".lnk");
        // qDebug() << "createShortcuts " << ifile << " " << p << " " <<
        //         from;

        QString desc;
        if (p)
            desc = p->description;
        if (desc.isEmpty())
            desc = this->package;
        HRESULT r = CreateLink((WCHAR*) path.replace('/', '\\').utf16(),
                               (WCHAR*) from.utf16(),
                               (WCHAR*) desc.utf16(),
                               (WCHAR*) workingDir.utf16());

        if (!SUCCEEDED(r)) {
            qDebug() << "shortcut creation failed";
            return false;
        }
    }
    return true;
}

void PackageVersion::install(Job* job)
{
    job->setHint("Preparing");

    if (installed() || external) {
        job->setProgress(1);
        job->complete();
        return;
    }

    // qDebug() << "install.2";
    QDir d(WPMUtils::getInstallationDirectory() + "\\" + this->package +
            "-" + this->version.getVersionString());

    // qDebug() << "install.dir=" << d;

    // qDebug() << "install.3";
    QTemporaryFile* f = 0;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Downloading");
        Job* djob = job->newSubJob(0.57);
        f = Downloader::download(djob, this->download);

        if (!f) {
            delete djob;
            job->setHint("Downloading (2nd try)");
            double rest = 0.57 - job->getProgress();
            if (rest < 0.01)
                rest = 0.01;
            djob = job->newSubJob(rest);
            f = Downloader::download(djob, this->download);
        }

        // qDebug() << "install.3.2 " << (f == 0) << djob->getErrorMessage();
        if (!djob->getErrorMessage().isEmpty())
            job->setErrorMessage(djob->getErrorMessage());
        delete djob;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (!this->sha1.isEmpty()) {
            job->setHint("Computing hash sum");
            QString h = WPMUtils::sha1(f->fileName());
            if (h.toLower() != this->sha1.toLower()) {
                job->setErrorMessage(QString(
                        "Hash sum (SHA1) %1 found, but %2 "
                        "was expected. The file has changed.").arg(h).
                        arg(this->sha1));
            } else {
                job->setProgress(0.64);
            }
        } else {
            job->setProgress(0.64);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (!d.mkpath(d.absolutePath())) {
            job->setErrorMessage(QString("Cannot create directory: %0").
                    arg(d.absolutePath()));
        } else {
            job->setProgress(0.65);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        Repository::getDefault()->somethingWasInstalledOrUninstalled();
        if (this->type == 0) {
            QString errMsg;
            job->setHint("Extracting files");
            // qDebug() << "install.6 " << f->size() << d.absolutePath();

            if (unzip(f->fileName(), d.absolutePath() + "\\", &errMsg)) {
                job->setProgress(0.74);
            } else {
                job->setErrorMessage(QString(
                        "Error unzipping file into directory %0: %1").
                                     arg(d.absolutePath()).arg(errMsg));
            }
        } else {
            job->setHint("Copying the file");
            QString t = d.absolutePath();
            t.append("\\");
            QString fn = this->download.path();
            QStringList parts = fn.split('/');
            t.append(parts.at(parts.count() - 1));
            // qDebug() << "install " << t.replace('/', '\\');

            QString errMsg;
            if (!CopyFileW((WCHAR*) f->fileName().utf16(),
                           (WCHAR*) t.replace('/', '\\').utf16(), false)) {
                WPMUtils::formatMessage(GetLastError(), &errMsg);
                job->setErrorMessage(errMsg);
            } else {
                job->setProgress(0.74);
            }
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString err;
        this->createShortcuts(&err); // ignore errors
        if (err.isEmpty())
            job->setProgress(0.84);
        else
            job->setErrorMessage(err);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString errMsg;
        if (!this->saveFiles(&errMsg)) {
            job->setErrorMessage(errMsg);
        } else {
            job->setProgress(0.85);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString p = ".Npackd\\Install.bat";
        if (!QFile::exists(d.absolutePath() +
                "\\" + p)) {
            p = ".WPM\\Install.bat";
        }
        if (QFile::exists(d.absolutePath() + "\\" + p)) {
            job->setHint("Running the installation script (this may take some time)");
            Job* exec = job->newSubJob(0.1);
            if (!d.exists(".Npackd"))
                d.mkdir(".Npackd");

            QStringList env;
            env.append("NPACKD_PACKAGE_NAME");
            env.append(this->package);
            env.append("NPACKD_PACKAGE_VERSION");
            env.append(this->version.getVersionString());
            this->executeFile(exec, p, ".Npackd\\Install.log", env);
            if (!exec->getErrorMessage().isEmpty())
                job->setErrorMessage(exec->getErrorMessage());
            else {
                this->path = d.absolutePath();
                this->external = false;
                this->path.replace('/', '\\');
                QString err = this->saveInstallationInfo();
                if (!err.isEmpty())
                    job->setErrorMessage(err);
            }
            delete exec;
        } else {
            this->path = d.absolutePath();
            this->external = false;
            this->path.replace('/', '\\');
            QString err = this->saveInstallationInfo();
            if (!err.isEmpty())
                job->setErrorMessage(err);
            job->setProgress(0.94);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString err = saveInstallationInfo();
        if (!err.isEmpty()) {
            job->setErrorMessage(err);
        } else {
            job->setProgress(0.95);
        }
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        job->setHint(QString("Deleting desktop shortcuts %1").
                     arg(WPMUtils::getShellDir(CSIDL_DESKTOP)));
        Job* sub = job->newSubJob(0.05);
        deleteShortcuts(sub, false, true, true);
        delete sub;
    }

    if (!job->getErrorMessage().isEmpty()) {
        // ignore errors
        Job* rjob = new Job();
        removeDirectory(rjob);
        delete rjob;
    }

    delete f;

    job->complete();
}

bool PackageVersion::unzip(QString zipfile, QString outputdir, QString* errMsg)
{
    QuaZip zip(zipfile);
    bool extractsuccess = false;
    if (!zip.open(QuaZip::mdUnzip)) {
        errMsg->append(QString("Cannot open the ZIP file %1: %2").
                       arg(zipfile).arg(zip.getZipError()));
        return false;
    }
    QuaZipFile file(&zip);
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        file.open(QIODevice::ReadOnly);
        QString name = zip.getCurrentFileName();
        name.prepend(outputdir); /* extract to path ....... */
        QFile meminfo(name);
        QFileInfo infofile(meminfo);
        QDir dira(infofile.absolutePath());
        if (dira.mkpath(infofile.absolutePath())) {
            if (meminfo.open(QIODevice::ReadWrite)) {
                meminfo.write(file.readAll());  /* write */
                meminfo.close();
                extractsuccess = true;
            }
        } else {
            errMsg->append(QString("Cannot create directory %1").arg(
                    infofile.absolutePath()));
            file.close();
            return false;
        }
        file.close(); // do not forget to close!
    }
    zip.close();

    return extractsuccess;
}

bool PackageVersion::saveFiles(QString* errMsg)
{
    bool success = false;
    QDir d = this->getDirectory();
    for (int i = 0; i < this->files.count(); i++) {
        PackageVersionFile* f = this->files.at(i);
        QString fullPath = d.absolutePath() + "\\" + f->path;
        QString fullDir = WPMUtils::parentDirectory(fullPath);
        if (d.mkpath(fullDir)) {
            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QTextStream stream(&file);
                stream << f->content;
                file.close();
                success = true;
            } else {
                *errMsg = QString("Could not create file %1").arg(
                        fullPath);
                break;
            }
        } else {
            *errMsg = QString("Could not create directory %1").arg(
                    fullDir);
            break;
        }
    }
    return success;
}

QStringList PackageVersion::findLockedFiles()
{
    QStringList files = WPMUtils::getProcessFiles();
    QStringList r;
    QString dir = getDirectory().absolutePath();
    for (int i = 0; i < files.count(); i++) {        
        if (WPMUtils::isUnder(files.at(i), dir)) {
            r.append(files.at(i));
        }
    }
    return r;
}

void PackageVersion::executeFile(Job* job, const QString& path,
        const QString& outputFile, const QStringList& env)
{
    QDir d = this->getDirectory();
    QProcess p(0);
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList params;
    p.setWorkingDirectory(d.absolutePath());
    QString exe = d.absolutePath() + "\\" + path;
    for (int i = 0; i < env.count(); i += 2) {
        p.processEnvironment().insert(env.at(i), env.at(i + 1));
    }
    p.start(exe, params);

    time_t start = time(NULL);
    while (true) {
        if (job->isCancelled()) {
            if (p.state() == QProcess::Running) {
                p.terminate();
                if (p.waitForFinished(10000))
                    break;
                p.kill();
            }
        }
        if (p.waitForFinished(5000) || p.state() == QProcess::NotRunning) {
            job->setProgress(1);
            if (p.exitCode() != 0) {
                job->setErrorMessage(
                        QString("Process %1 exited with the code %2").arg(
                        exe).arg(p.exitCode()));
            }
            QFile f(d.absolutePath() + "\\" + outputFile);
            if (f.open(QIODevice::WriteOnly)) {
                QByteArray output = p.readAll();
                if (!job->getErrorMessage().isEmpty()) {
                    QString log(job->getErrorMessage());
                    log.append("\n");
                    log.append(output);
                    job->setErrorMessage(log);
                }
                f.write(output);
                f.close();
            }
            break;
        }
        time_t seconds = time(NULL) - start;
        double percents = ((double) seconds) / 300; // 5 Minutes
        if (percents > 0.9)
            percents = 0.9;
        job->setProgress(percents);
        job->setHint(QString("%1 minutes").arg(seconds / 60));
    }
    job->complete();
}

void PackageVersion::deleteShortcuts(QDir& d)
{
    // Get a pointer to the IShellLink interface.
    IShellLink* psl;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl);

    if (SUCCEEDED(hres)) {
        QDir instDir = getDirectory();
        QString instPath = instDir.absolutePath();
        QFileInfoList entries = d.entryInfoList(
                QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            QFileInfo entryInfo = entries[idx];
            QString path = entryInfo.absoluteFilePath();
            // qDebug() << "PackageVersion::deleteShortcuts " << path;
            if (entryInfo.isDir()) {
                QDir dd(path);
                deleteShortcuts(dd);
            } else {
                if (path.toLower().endsWith(".lnk")) {
                    // qDebug() << "deleteShortcuts " << path;
                    IPersistFile* ppf;

                    // Query IShellLink for the IPersistFile interface for saving the
                    // shortcut in persistent storage.
                    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

                    if (SUCCEEDED(hres)) {
                        // Save the link by calling IPersistFile::Save.
                        hres = ppf->Load((WCHAR*) path.utf16(), STGM_READ);
                        if (SUCCEEDED(hres)) {
                            WCHAR info[MAX_PATH + 1];
                            hres = psl->GetPath(info, MAX_PATH,
                                    (WIN32_FIND_DATAW*) 0, SLGP_UNCPRIORITY);
                            if (SUCCEEDED(hres)) {
                                QString targetPath;
                                targetPath.setUtf16((ushort*) info, wcslen(info));
                                // qDebug() << "deleteShortcuts " << targetPath << " " <<
                                //        instPath;
                                if (WPMUtils::isUnder(targetPath,
                                                      instPath)) {
                                    QFile::remove(path);
                                    // qDebug() << "deleteShortcuts removed";
                                }
                            }
                        }
                        ppf->Release();
                    }
                }
            }
        }
        psl->Release();
    }
}
