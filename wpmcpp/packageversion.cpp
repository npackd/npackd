#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>

#include "repository.h"
#include "qurl.h"
#include "QtDebug"

#include "quazip.h"
#include "quazipfile.h"

#include "packageversion.h"
#include "job.h"
#include "downloader.h"

PackageVersion::PackageVersion(const QString& package)
{
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
    this->package = package;
    this->download = QUrl("http://www.younamehere.com/download.zip");
}

PackageVersion::PackageVersion()
{
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
    this->package = "unknown";
    this->download = QUrl("http://www.younamehere.com/download.zip");
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

bool PackageVersion::uninstall(QString* errMsg)
{
    QDir d = getDirectory();
    if (d.exists()) {
        return removeDirectory(d, errMsg);
    } else {
        return true;
    }
}

bool PackageVersion::removeDirectory(QDir &aDir, QString* errMsg)
{
    bool ok = true;
    if (aDir.exists()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::Dirs | QDir::Files);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            QFileInfo entryInfo = entries[idx];
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir()) {
                QDir dd(path);
                ok = removeDirectory(dd, errMsg);
                if (!ok)
                    qDebug() << "PackageVersion::removeDirectory.3" << *errMsg;
            } else {
                QFile file(path);
                ok = file.remove();
                if (!ok) {
                    ok = false;
                    errMsg->clear();
                    errMsg->append("Cannot delete the file: ").append(path);
                    qDebug() << "PackageVersion::removeDirectory.1" << *errMsg;
                }
            }
            if (!ok)
                break;
        }
        if (ok && !aDir.rmdir(aDir.absolutePath())) {
            qDebug() << "PackageVersion::removeDirectory.2";
            ok = false;
            errMsg->clear();
            errMsg->append("Cannot delete the directory: ").append(
                    aDir.absolutePath());
        }
    }
    qDebug() << "PackageVersion::removeDirectory: " << aDir << " " << ok <<
            *errMsg;
    return ok;
}

QDir PackageVersion::getDirectory()
{
    QString pf = Repository::getProgramFilesDir();
    QDir d(pf + "\\wpm\\" + this->package + "-" +
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

bool PackageVersion::install(QString* errMsg)
{
    qDebug() << "install.1";
    bool result = false;
    errMsg->clear();
    if (!installed()) {
        // TODO: error handling/free memory
        qDebug() << "install.2";
        QDir d = getDirectory();
        qDebug() << "install.dir=" << d;

        qDebug() << "install.3";
        QTemporaryFile* f = Downloader::download(this->download, errMsg);
        if (f) {
            qDebug() << "install.4";
            if (d.mkdir(d.absolutePath())) {
                qDebug() << "install.5";
                qDebug() << "install.6 " << f->size() << d.absolutePath();

                if (unzip(f->fileName(), d.absolutePath() + "\\", errMsg))
                    result = true;
                else {
                    // TODO: delete the directory
                }
            } else {
                errMsg->append("Cannot create directory: ").append(d.absolutePath());
            }
            delete f;
        }
    } else {
        result = true;
    }
    return result;
}

bool PackageVersion::MakezipDir( QString dirtozip )
{
    // TODO: verify the implementation
    // TODO: this method is not used. Remove?
    const QString cartella = QDir::currentPath();
    char c;
    QString zipfile;
    QString ultimacartellaaperta = dirtozip.left(dirtozip.lastIndexOf("/"))+"/";
    QDir dir(ultimacartellaaperta);
    QString dirname = dir.dirName();
    zipfile = dirname.append(".zip");
    if (dir.exists())
    {
       QuaZip zip(zipfile);
       if(!zip.open(QuaZip::mdCreate)) {
       qWarning("testCreate(): zip.open(): %d", zip.getZipError());
       return false;
       }

      QFile inFile;
      QuaZipFile outFile(&zip);
      const QFileInfoList list = dir.entryInfoList();
      QFileInfo fi;

      for (int l = 0; l < list.size(); l++)
      {
         fi = list.at(l);
         if (fi.isDir() && fi.fileName() != "." && fi.fileName() != "..") {
             /* dir */
         }  else if (fi.isFile() and fi.fileName() != zipfile ) {
             // TODO: CoPrint(QString("File on dirzip : %1").arg( fi.fileName() ),1);
              QDir::setCurrent(dir.absolutePath());
               inFile.setFileName(fi.fileName());
               if(!inFile.open(QIODevice::ReadOnly)) {
                  qWarning("testCreate(): inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
                   return false;
                }
               if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(inFile.fileName(), inFile.fileName()))) {
               qWarning("testCreate(): outFile.open(): %d", outFile.getZipError());
               return false;
               }
                while(inFile.getChar(&c)&&outFile.putChar(c));
                if(outFile.getZipError()!=UNZ_OK) {
                  qWarning("testCreate(): outFile.putChar(): %d", outFile.getZipError());
                  return false;
                }
                outFile.close();
                if(outFile.getZipError()!=UNZ_OK) {
                  qWarning("testCreate(): outFile.close(): %d", outFile.getZipError());
                  return false;
                }
                inFile.close();

         }

      }

        zip.close();
          if(zip.getZipError()!=0) {
          qWarning("testCreate(): zip.close(): %d", zip.getZipError());
          QDir::setCurrent(cartella);
          return false;
          }
          // TODO: CoPrint(QString("Successful created zip file on:"),1);
    // TODO: CoPrint(QString("%2/%1").arg( zipfile ).arg(cartella),1);
    QDir::setCurrent(cartella);
    return true;
    }
    return true;
}

bool PackageVersion::unzip(QString zipfile, QString outputdir, QString* errMsg)
{
    // TODO: verify the implementation
    QuaZip zip(zipfile);
    bool extractsuccess = false;
    zip.open(QuaZip::mdUnzip);
    QuaZipFile file(&zip);
       for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile()) {
           file.open(QIODevice::ReadOnly);
           QString name = zip.getCurrentFileName();
                   name.prepend(outputdir);   /* extract to path ....... */
           QFile meminfo(name);
           QFileInfo infofile(meminfo);
           QDir dira(infofile.absolutePath());
           if ( dira.mkpath(infofile.absolutePath()) ) {
           /* dir is exist*/
           //////qDebug() << "### name  " << name;
           /////qDebug() << "### namedir yes  " << infofile.absolutePath();
               if ( meminfo.open(QIODevice::ReadWrite) ) {
               meminfo.write(file.readAll());   /* write */
               meminfo.close();
               extractsuccess = true;
               //////////RegisterImage(name);
               }
           } else {
             file.close();
             return false;
           }
           file.close(); // do not forget to close!
        }
    zip.close();

    return extractsuccess;
}

/**
  * TODO: correct
// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces
//   to create and store a shortcut to the specified object.
// Returns the result of calling the member functions of the interfaces.
// lpszPathObj - address of a buffer containing the path of the object.
// lpszPathLink - address of a buffer containing the path where the
//   Shell link is to be stored.
// lpszDesc - address of a buffer containing the description of the
//   Shell link.
 */
HRESULT CreateLink(LPCWSTR lpszPathObj,
    LPCSTR lpszPathLink, LPCWSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    CoInitialize( NULL );

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
        hres = psl->QueryInterface(IID_IPersistFile,
            (LPVOID*)&ppf);

        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1,
                wsz, MAX_PATH);

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
  else printf( "failed 2\n");
        psl->Release();
    }
 else printf( "failed 1\n" );
    return hres;
}

