#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>
#include <stdlib.h>

#include "repository.h"
#include "qurl.h"
#include "QtDebug"
#include "qiodevice.h"
#include "qprocess.h"
#include "qmessagebox.h"
#include "qapplication.h"

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
 */
HRESULT CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc)
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
    this->download = QUrl("http://www.younamehere.com/download.zip");
    this->type = 0;
}

PackageVersion::PackageVersion()
{
    this->package = "unknown";
    this->download = QUrl("http://www.younamehere.com/download.zip");
    this->type = 0;
}

QString PackageVersion::getShortPackageName()
{
    QStringList sl = this->package.split(".");
    return sl.last();
}

PackageVersion::~PackageVersion()
{
    qDeleteAll(this->files);
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
    QDir d = getDirectory();
    d.refresh();
    return d.exists();
}

void PackageVersion::update(Job* job)
{
    job->setCancellable(true);
    Repository* r = Repository::getDefault();
    PackageVersion* newest = r->findNewestPackageVersion(this->package);
    if (newest->version.compare(this->version) > 0 && !newest->installed()) {
        job->setHint("Uninstalling the old version");
        Job* sub = job->newSubJob(0.1);
        uninstall(sub);
        if (sub->getErrorMessage().isEmpty() && !job->isCancelled()) {
            delete sub;
            job->setHint("Installing the new version");
            sub = job->newSubJob(0.9);
            newest->install(sub);
            delete sub;
        } else {
            delete sub;
        }
    } else {
        job->setProgress(1);
    }
    job->complete();
}

void PackageVersion::uninstall(Job* job)
{
    QDir d = getDirectory();

    QString errMsg;
    QString p = ".WPM\\Uninstall.bat";
    if (QFile::exists(d.absolutePath() + "\\" + p)) {
        job->setHint("Running the uninstallation script");
        this->executeFile(p, &errMsg); // ignore errors
    }
    job->setProgress(0.25);

    // Uninstall.bat may have deleted some files
    d.refresh();

    if (job->getErrorMessage().isEmpty()) {
        job->setHint("Deleting shortcuts");
        QDir d(WPMUtils::getProgramShortcutsDir());
        deleteShortcuts(d);
        job->setProgress(0.3);

        QDir d2(WPMUtils::getCommonProgramShortcutsDir());
        deleteShortcuts(d2);
        job->setProgress(0.35);

        QDir d3(WPMUtils::getShellDir(CSIDL_DESKTOP));
        deleteShortcuts(d3);
        job->setProgress(0.40);

        QDir d4(WPMUtils::getShellDir(CSIDL_COMMON_DESKTOPDIRECTORY));
        deleteShortcuts(d4);
        job->setProgress(0.45);
    }

    if (job->getErrorMessage().isEmpty()) {
        if (d.exists()) {
            job->setHint("Deleting files");
            bool r = WPMUtils::removeDirectory(d, &errMsg);
            job->setProgress(0.75);
            if (!r) {
                Sleep(5000); // 5 Seconds
                r = WPMUtils::removeDirectory(d, &errMsg);
            }
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
    QString pf = WPMUtils::getProgramFilesDir();
    QDir d(pf + "\\WPM\\" + this->package + "-" +
           this->version.getVersionString());
    return d;
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

        QString from = WPMUtils::getProgramShortcutsDir();
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
        qDebug() << "createShortcuts " << ifile << " " << p << " " <<
                from;

        QString desc;
        if (p)
            desc = p->description;
        if (desc.isEmpty())
            desc = this->package;
        HRESULT r = CreateLink((WCHAR*) path.replace('/', '\\').utf16(),
                               (WCHAR*) from.utf16(),
                               (WCHAR*) desc.utf16());
        // TODO: error message
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

    // qDebug() << "install.1";
    if (!installed()) {
        job->setCancellable(true);

        // qDebug() << "install.2";
        QDir d = getDirectory();
        // qDebug() << "install.dir=" << d;

        // qDebug() << "install.3";
        job->setHint("Downloading");
        Job* djob = job->newSubJob(0.60);
        QString errMsg;
        QTemporaryFile* f = Downloader::download(djob, this->download);
        // qDebug() << "install.3.2 " << (f == 0) << djob->getErrorMessage();
        if (!djob->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Download failed: %1").arg(
                    djob->getErrorMessage()));
        delete djob;

        job->setCancellable(false);
        if (f) {
            if (!this->sha1.isEmpty()) {
                job->setHint("Computing hash sum");
                QString h = WPMUtils::sha1(f->fileName());
                if (h.toLower() != this->sha1.toLower()) {
                    job->setErrorMessage(QString("Hash sum %1 found, but %2 "
                            "was expected. The file has changed.").arg(h).
                            arg(this->sha1));
                }
            }
            job->setProgress(0.7);

            if (job->getErrorMessage().isEmpty()) {
                if (d.mkdir(d.absolutePath())) {
                    if (this->type == 0) {
                        job->setHint("Extracting files");
                        qDebug() << "install.6 " << f->size() << d.absolutePath();

                        if (unzip(f->fileName(), d.absolutePath() + "\\", &errMsg)) {
                            QString err;
                            this->createShortcuts(&err); // ignore errors
                            job->setProgress(0.90);
                        } else {
                            job->setErrorMessage(QString(
                                    "Error unzipping file into directory %0: %1").
                                                 arg(d.absolutePath()).arg(errMsg));
                            WPMUtils::removeDirectory2(d, &errMsg); // ignore errors
                        }
                    } else {
                        job->setHint("Copying the file");
                        QString t = d.absolutePath();
                        t.append("\\");
                        QString fn = this->download.path();
                        QStringList parts = fn.split('/');
                        t.append(parts.at(parts.count() - 1));
                        qDebug() << "install " << t.replace('/', '\\');
                        if (!CopyFileW((WCHAR*) f->fileName().utf16(),
                                       (WCHAR*) t.replace('/', '\\').utf16(), false)) {
                            WPMUtils::formatMessage(GetLastError(), &errMsg);
                            job->setErrorMessage(errMsg);
                            WPMUtils::removeDirectory2(d, &errMsg); // ignore errors
                        } else {
                            QString err;
                            this->createShortcuts(&err); // ignore errors
                            job->setProgress(0.90);
                        }
                    }
                    if (job->getErrorMessage().isEmpty()) {
                        if (!this->saveFiles(&errMsg)) {
                            job->setErrorMessage(errMsg);
                            job->setProgress(1);
                        }
                    }
                    if (job->getErrorMessage().isEmpty()) {
                        QString p = ".WPM\\Install.bat";
                        if (QFile::exists(getDirectory().absolutePath() +
                                "\\" + p)) {
                            job->setHint("Running the installation script");
                            if (this->executeFile(p, &errMsg)) {
                                job->setProgress(0.99);
                                job->setHint(QString("Deleting desktop shortcuts %1").
                                             arg(WPMUtils::getShellDir(CSIDL_DESKTOP)));
                                QDir d2(WPMUtils::getShellDir(CSIDL_DESKTOP));
                                deleteShortcuts(d2);
                                QDir d3(WPMUtils::getShellDir(CSIDL_COMMON_DESKTOPDIRECTORY));
                                deleteShortcuts(d3);
                                job->setProgress(1);
                            } else {
                                job->setErrorMessage(errMsg);

                                // ignore errors
                                WPMUtils::removeDirectory2(d, &errMsg);
                            }
                        } else {
                            job->setProgress(1);
                        }
                    }
                } else {
                    job->setErrorMessage(QString("Cannot create directory: %0").
                            arg(d.absolutePath()));
                }
            }
            delete f;
        }
    } else {
        job->setProgress(1);
    }

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

bool PackageVersion::executeFile(QString& path, QString* errMsg)
{
    bool success = false;
    QDir d = this->getDirectory();
    QProcess p(0);
    QStringList params;
    p.setWorkingDirectory(d.absolutePath());
    QString exe = d.absolutePath() + "\\" + path;
    p.start(exe, params);

    // 5 minutes
    if (p.waitForFinished(300000)){
        if (p.exitCode() != 0) {
            *errMsg = QString("Process %1 exited with the code %2").arg(
                    exe).arg(p.exitCode());
        } else {
            success = true;
        }
    } else {
        *errMsg = QString("Timeout waiting for %1 to finish").arg(
                exe);
    }
    return success;
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
        psl->Release();
    }
}
