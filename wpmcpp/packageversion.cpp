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

bool PackageVersion::installed()
{
    return getDirectory().exists();
}

void PackageVersion::uninstall()
{
    // TODO
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

void PackageVersion::install()
{
    qDebug() << "install.1";
    if (!installed()) {
        qDebug() << "install.2";
        QDir d = getDirectory();
        qDebug() << "install.dir=" << d;

        qDebug() << "install.3";
        QTemporaryFile* f = Repository::download(this->download);
        qDebug() << "install.4";
        d.mkdir(this->package + "-" + this->getVersionString());
        qDebug() << "install.5";
    }
}

bool PackageVersion::MakezipDir( QString dirtozip )
{
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



}

bool  PackageVersion::UnzipTo( QString zipfile, QString outputdir )
{
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
