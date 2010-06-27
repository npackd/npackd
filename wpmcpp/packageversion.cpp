#include "packageversion.h"
#include "repository.h"
#include "qurl.h"
#include "QtDebug"

#include "quazip.h"
#include "quazipfile.h"

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

void PackageVersion::uninstall()
{
    QDir d = getDirectory();
    if (d.exists())
        RemoveDirectory(d); // TODO: handle errors
}

bool PackageVersion::RemoveDirectory(QDir &aDir)
{
    // TODO: verify the implementation
    bool has_err = false;
    if (aDir.exists())//QDir::NoDotAndDotDot
    {
        QFileInfoList entries = aDir.entryInfoList(QDir::NoDotAndDotDot |
                                                   QDir::Dirs | QDir::Files);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++)
        {
            QFileInfo entryInfo = entries[idx];
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir())
            {
                QDir dd(path);
                has_err = RemoveDirectory(dd);
            }
            else
            {
                QFile file(path);
                if (!file.remove())
                    has_err = true;
            }
            if (has_err)
                break;
        }
        if (!aDir.rmdir(aDir.absolutePath()))
            has_err = true;
    }
    return(has_err);
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
        QTemporaryFile* f = Repository::download(this->download, errMsg);
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
