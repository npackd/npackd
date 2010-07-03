#include <windows.h>
#include <shlobj.h>

#include "repository.h"
#include "qhttp.h"
#include "qtemporaryfile.h"
#include "downloader.h"
#include "qsettings.h"
#include "qdom.h"
#include "qdebug.h"

#include "downloader.h"

Repository* Repository::def = 0;

Repository::Repository()
{
}

PackageVersion* createPackageVersion(QDomElement* e)
{
    PackageVersion* a = new PackageVersion(
            e->attribute("package"));
    QString url = e->elementsByTagName("url").at(0).
                  firstChild().nodeValue();
    a->download.setUrl(url);
    QString name = e->attribute("name", "1.0");
    a->setVersion(name);

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

    return a;
}

void Repository::load(Job* job)
{
    job->setAmountOfWork(100);
    this->packageVersions.clear();
    QUrl* url = getRepositoryURL();
    if (url) {
        job->setHint("Downloading");
        Job* djob = job->newSubJob(50);
        QTemporaryFile* f = Downloader::download(djob, *url);
        qDebug() << "Repository::load.1";
        if (f) {
            job->setHint("Parsing the content");
            qDebug() << "Repository::load.2";
            QDomDocument doc;
            int errorLine;
            int errorColumn;
            QString errMsg;
            if (doc.setContent(f, &errMsg, &errorLine, &errorColumn)) {
                QDomElement root = doc.documentElement();
                for (QDomNode n = root.firstChild(); !n.isNull();
                        n = n.nextSibling()) {
                    if (n.isElement()) {
                        QDomElement e = n.toElement();
                        if (e.nodeName() == "version") {
                            this->packageVersions.append(
                                    createPackageVersion(&e));
                        }
                    }
                }
                job->done(-1);
            } else {
                job->setErrorMessage(QString("XML parsing failed: %1").
                                     arg(errMsg));
            }
            delete f;
        } else {
            job->setErrorMessage(QString("Download failed: %2").
                    arg(djob->getErrorMessage()));
        }
        delete djob;
    } else {
        job->setErrorMessage("No repository defined");
    }
    qDebug() << "Repository::load.3";
}

QUrl* Repository::getRepositoryURL()
{
    QSettings s("WPM", "Windows Package Manager");
    QString v = s.value("repository", "").toString();
    if (v != "")
        return new QUrl(v);
    else
        return 0;
}

void Repository::setRepositoryURL(QUrl& url)
{
    QSettings s("WPM", "Windows Package Manager");
    s.setValue("repository", url.toString());
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}


