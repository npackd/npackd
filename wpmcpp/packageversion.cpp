#include "packageversion.h"
#include "repository.h"
#include "qurl.h"
#include "QtDebug"

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
