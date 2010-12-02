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
    qDeleteAll(this->files);
    qDeleteAll(this->dependencies);
    qDeleteAll(this->fileHandlers);
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

        this->fullText = r.toLower();
    }
    return this->fullText;
}

bool PackageVersion::installed()
{
    if (external) {
        return true;
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
                0, KEY_READ | (w64bit ? KEY_WOW64_64KEY : 0),
                &hkeyClasses) != ERROR_SUCCESS)
        return;

    for (int i = 0; i < this->fileHandlers.count(); i++) {
        FileExtensionHandler* fh = this->fileHandlers.at(i);

        QString progId = this->package + "-" +
                this->version.getVersionString() + "-" +
                fh->program.replace('\\', '-').replace('/', '-');
        QString key = "Applications\\" + progId +
                "\\shell";
        HKEY hkey;
        if (RegCreateKeyExW(hkeyClasses, (WCHAR*) key.utf16(),
                0, 0, 0, 0, 0, &hkey, 0) == ERROR_SUCCESS) {
            RegCloseKey(hkey);

            key = fh->extension + "\\OpenWithProgids";
            if (RegCreateKeyExW(hkeyClasses, (WCHAR*) key.utf16(),
                    0, 0, 0, 0, 0, &hkey, 0) == ERROR_SUCCESS) {
                RegSetValueEx(hkey, (WCHAR*) progId.utf16(), 0,
                        REG_BINARY, 0, 0);
                RegCloseKey(hkey);
            }
        }
    }
}

void PackageVersion::unregisterFileHandlers()
{

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

    QString errMsg;
    QString p = ".Npackd\\Uninstall.bat";
    if (!QFile::exists(d.absolutePath() + "\\" + p)) {
        p = ".WPM\\Uninstall.bat";
    }
    if (QFile::exists(d.absolutePath() + "\\" + p)) {
        job->setHint("Running the uninstallation script (this may take some time)");
        Job* sub = job->newSubJob(0.25);
        this->executeFile(sub, p); // ignore errors
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
            bool r = WPMUtils::removeDirectory2(rjob, d, &errMsg);
            if (r) {
                job->setProgress(1);
            } else
                job->setErrorMessage(errMsg);
        } else {
            job->setProgress(1);
        }
    }

    job->complete();
}

QDir PackageVersion::getDirectory()
{
    QDir d(WPMUtils::getInstallationDirectory() + "\\" + this->package + "-" +
           this->version.getVersionString());
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
    QDir d = getDirectory();
    // qDebug() << "install.dir=" << d;

    // qDebug() << "install.3";
    QTemporaryFile* f = 0;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Downloading");
        Job* djob = job->newSubJob(0.60);
        f = Downloader::download(djob, this->download);

        // qDebug() << "install.3.2 " << (f == 0) << djob->getErrorMessage();
        if (f == 0)
            job->setErrorMessage(QString("Download failed: %1").arg(
                    djob->getErrorMessage()));
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
                job->setProgress(0.7);
            }
        } else {
            job->setProgress(0.7);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (!d.mkdir(d.absolutePath())) {
            job->setErrorMessage(QString("Cannot create directory: %0").
                    arg(d.absolutePath()));
        } else {
            job->setProgress(0.71);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        Repository::getDefault()->somethingWasInstalledOrUninstalled();
        if (this->type == 0) {
            QString errMsg;
            job->setHint("Extracting files");
            // qDebug() << "install.6 " << f->size() << d.absolutePath();

            if (unzip(f->fileName(), d.absolutePath() + "\\", &errMsg)) {
                job->setProgress(0.80);
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
                job->setProgress(0.80);
            }
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString err;
        this->createShortcuts(&err); // ignore errors
        if (err.isEmpty())
            job->setProgress(0.9);
        else
            job->setErrorMessage(err);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString errMsg;
        if (!this->saveFiles(&errMsg)) {
            job->setErrorMessage(errMsg);
        } else {
            job->setProgress(0.91);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        QString p = ".Npackd\\Install.bat";
        if (!QFile::exists(getDirectory().absolutePath() +
                "\\" + p)) {
            p = ".WPM\\Install.bat";
        }
        if (QFile::exists(getDirectory().absolutePath() +
                "\\" + p)) {
            job->setHint("Running the installation script (this may take some time)");
            Job* exec = job->newSubJob(0.04);
            this->executeFile(exec, p);
            delete exec;
        } else {
            job->setProgress(0.95);
        }
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        job->setHint(QString("Deleting desktop shortcuts %1").
                     arg(WPMUtils::getShellDir(CSIDL_DESKTOP)));
        Job* sub = job->newSubJob(0.04);
        deleteShortcuts(sub, false, true, true);
        delete sub;
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        registerFileHandlers();
        job->setProgress(1);
    }

    if (!job->getErrorMessage().isEmpty()) {
        // ignore errors
        Job* rjob = new Job();
        QString errMsg;
        WPMUtils::removeDirectory2(rjob, d, &errMsg);
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

void PackageVersion::executeFile(Job* job, QString& path)
{
    QDir d = this->getDirectory();
    QProcess p(0);
    QStringList params;
    p.setWorkingDirectory(d.absolutePath());
    QString exe = d.absolutePath() + "\\" + path;
    p.start(exe, params);

    time_t start = time(NULL);
    while (true) {
        if (job->isCancelled()) {
            if (p.state() == QProcess::Running)
                p.terminate();
        }
        if (p.waitForFinished(5000) || p.state() == QProcess::NotRunning) {
            job->setProgress(1);
            if (p.exitCode() != 0) {
                job->setErrorMessage(
                        QString("Process %1 exited with the code %2").arg(
                        exe).arg(p.exitCode()));
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
                                    // qDebug() << "deleteShortcuts true" << ok;
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
