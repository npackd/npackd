#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>

#include "repository.h"
#include "qurl.h"
#include "QtDebug"

#include "quazip.h"
#include "quazipfile.h"

#include "zlib.h"

#include "packageversion.h"
#include "job.h"
#include "downloader.h"
#include "wpmutils.h"

/**
 * Uses the Shell's IShellLink and IPersistFile interfaces
 *   to create and store a shortcut to the specified object.
 *
 * @return the result of calling the member functions of the interfaces.
 * @param lpszPathObj - address of a buffer containing the path of the object.
 * @param lpszPathLink - address of a buffer containing the path where the
 *   Shell link is to be stored.
 * @param lpszDesc - address of a buffer containing the description of the
 *   Shell link.
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
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
    this->package = package;
    this->download = QUrl("http://www.younamehere.com/download.zip");
    this->type = 0;
}

PackageVersion::PackageVersion()
{
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
    this->package = "unknown";
    this->download = QUrl("http://www.younamehere.com/download.zip");
    this->type = 0;
}

PackageVersion::~PackageVersion()
{
    delete[] this->parts;
}

void PackageVersion::setVersion(int a, int b)
{
    delete[] this->parts;
    this->parts = new int[2];
    this->parts[0] = a;
    this->parts[1] = b;
    this->nparts = 2;
}

void PackageVersion::setVersion(QString& v)
{
    delete[] this->parts;
    QStringList sl = v.split(".", QString::KeepEmptyParts);

    this->parts = new int[sl.count()];
    this->nparts = sl.count();
    for (int i = 0; i < sl.count(); i++) {
        this->parts[i] = sl.at(i).toInt();
    }
}

bool PackageVersion::installed()
{
    QDir d = getDirectory();
    d.refresh();
    return d.exists();
}

void PackageVersion::uninstall(Job* job)
{
    job->setAmountOfWork(1);
    QDir d = getDirectory();
    if (d.exists()) {
        QString errMsg;
        bool r = WPMUtils::removeDirectory(d, &errMsg);
        if (r)
            job->done(-1);
        else
            job->setErrorMessage(errMsg);
    } else {
        job->done(-1);
    }
}

QDir PackageVersion::getDirectory()
{
    QString pf = WPMUtils::getProgramFilesDir();
    QDir d(pf + "\\WPM\\" + this->package + "-" +
           this->getVersionString());
    return d;
}

QString PackageVersion::getVersionString()
{
    QString r;
    for (int i = 0; i < this->nparts; i++) {
        if (i != 0)
            r.append(".");
        r.append(QString("%1").arg(this->parts[i]));
    }
    return r;
}

bool PackageVersion::createShortcuts(QString *errMsg)
{
    QDir d = getDirectory();
    for (int i = 0; i < this->importantFiles.count(); i++) {
        QString ifile = this->importantFiles.at(i);
        QString p(ifile);
        p.prepend("\\");
        p.prepend(d.absolutePath());
        QString from = WPMUtils::getProgramShortcutsDir();
        from.append("\\");
        from.append(ifile.replace('\\', "_").replace('/', '_'));
        from.append(".lnk");
        qDebug() << "createShortcuts " << ifile << " " << p << " " <<
                from;
        HRESULT r = CreateLink((WCHAR*) p.replace('/', '\\').utf16(),
                               (WCHAR*) from.utf16(),
                               (WCHAR*) ifile.utf16());
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
    job->setAmountOfWork(10);

    qDebug() << "install.1";
    if (!installed()) {
        // TODO: error handling/free memory
        qDebug() << "install.2";
        QDir d = getDirectory();
        qDebug() << "install.dir=" << d;

        qDebug() << "install.3";
        job->setHint("Downloading");
        Job* djob = job->newSubJob(6);
        QString errMsg;
        QTemporaryFile* f = Downloader::download(djob, this->download);
        delete djob;
        if (f) {
            if (this->type == 0) {
                job->setHint("Extracting files");
                qDebug() << "install.4";
                if (d.mkdir(d.absolutePath())) {
                    qDebug() << "install.5";
                    qDebug() << "install.6 " << f->size() << d.absolutePath();

                    if (unzip(f->fileName(), d.absolutePath() + "\\", &errMsg)) {
                        QString err;
                        this->createShortcuts(&err); // ignore errors
                        job->done(-1);
                    } else {
                        job->setErrorMessage(QString(
                                "Error unzipping file into directory %0: %1").
                                             arg(d.absolutePath()).arg(errMsg));
                        WPMUtils::removeDirectory(d, &errMsg); // ignore errors
                    }
                } else {
                    job->setErrorMessage(QString("Cannot create directory: %0").
                            arg(d.absolutePath()));
                }
            } else {
                job->setHint("Copying the file");
                QString t = d.absolutePath();
                t.append("\\");
                QString fn = this->download.toString();
                QStringList parts = fn.split('\\');
                t.append(parts.at(parts.count() - 1));
                if (!CopyFileW((WCHAR*) f->fileName().utf16(),
                               (WCHAR*) t.utf16(), false)) {
                    WPMUtils::formatMessage(GetLastError(), &errMsg);
                    job->setErrorMessage(errMsg);
                } else {
                    job->done(-1);
                }
            }
            delete f;
        } else {
            job->setErrorMessage(errMsg);
        }
    } else {
        job->done(-1);
    }
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

