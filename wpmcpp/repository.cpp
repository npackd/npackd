#include <windows.h>
#include <shlobj.h>

#include "qhttp.h"
#include "qtemporaryfile.h"
#include "downloader.h"
#include "qsettings.h"
#include "qdom.h"
#include "qdebug.h"

#include "repository.h"
#include "downloader.h"
#include "packageversionfile.h"

Repository* Repository::def = 0;

Repository::Repository()
{
}

Repository::~Repository()
{
    qDeleteAll(this->packages);
    qDeleteAll(this->packageVersions);
}

PackageVersion* Repository::findNewestPackageVersion(QString &name)
{
    PackageVersion* r = 0;

    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->package == name) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }
    return r;
}

PackageVersion* Repository::createPackageVersion(QDomElement* e)
{
    PackageVersion* a = new PackageVersion(
            e->attribute("package"));
    QString url = e->elementsByTagName("url").at(0).
                  firstChild().nodeValue();
    a->download.setUrl(url);
    QString name = e->attribute("name", "1.0");
    a->version.setVersion(name);

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

    QDomNodeList files = e->elementsByTagName("file");
    for (int i = 0; i < files.count(); i++) {
        QDomElement e = files.at(i).toElement();
        a->files.append(createPackageVersionFile(&e));
    }

    return a;
}

Package* Repository::createPackage(QDomElement* e)
{
    QString name = e->attribute("name");
    Package* a = new Package(name, name);
    QDomNodeList nl = e->elementsByTagName("title");
    if (nl.count() != 0)
        a->title = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("url");
    if (nl.count() != 0)
        a->url = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("description");
    if (nl.count() != 0)
        a->description = nl.at(0).firstChild().nodeValue();

    return a;
}

PackageVersionFile* Repository::createPackageVersionFile(QDomElement* e)
{
    QString path = e->attribute("path");
    QString content = e->firstChild().nodeValue();
    PackageVersionFile* a = new PackageVersionFile(path, content);

    return a;
}

Package* Repository::findPackage(QString& name)
{
    for (int i = 0; i < this->packages.count(); i++) {
        if (this->packages.at(i)->name == name)
            return this->packages.at(i);
    }
    return 0;
}

int Repository::countUpdates()
{
    int r = 0;
    for (int i = 0; i < this->packageVersions.count(); i++) {
        PackageVersion* p = this->packageVersions.at(i);
        if (p->installed()) {
            PackageVersion* newest = findNewestPackageVersion(p->package);
            if (newest->version.compare(p->version) > 0 && !newest->installed())
                r++;
        }
    }
    return r;
}

void Repository::load(Job* job)
{
    qDeleteAll(this->packages);
    this->packages.clear();
    qDeleteAll(this->packageVersions);
    this->packageVersions.clear();

    QList<QUrl*> urls = getRepositoryURLs();
    if (urls.count() > 0) {
        job->setAmountOfWork(100 * urls.count() + 1);
        for (int i = 0; i < urls.count(); i++) {
            Job* s = job->newSubJob(100);
            loadOne(urls.at(i), s);
            if (!s->getErrorMessage().isEmpty()) {
                job->setErrorMessage(QString(
                        "Error loading the repository %1: %2").arg(
                        urls.at(i)->toString()).arg(
                        s->getErrorMessage()));
                delete s;
                break;
            }
            delete s;

            if (job->isCancelled())
                break;
        }
    } else {
        job->setErrorMessage("No repositories defined");
    }
    qDebug() << "Repository::load.3";

    qDeleteAll(urls);
    urls.clear();

    job->complete();
}

void Repository::loadOne(QUrl* url, Job* job) {
    job->setAmountOfWork(100);
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
                    } else if (e.nodeName() == "package") {
                        this->packages.append(
                                createPackage(&e));
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

    job->complete();
}


QList<QUrl*> Repository::getRepositoryURLs()
{
    QList<QUrl*> r;
    QSettings s("WPM", "Windows Package Manager");

    int size = s.beginReadArray("repositories");
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        QString v = s.value("repository").toString();
        r.append(new QUrl(v));
    }
    s.endArray();

    if (size == 0) {
        QString v = s.value("repository", "").toString();
        if (v != "") {
            r.append(new QUrl(v));
            setRepositoryURLs(r);
        }
    }
    
    return r;
}

void Repository::setRepositoryURLs(QList<QUrl*>& urls)
{
    QSettings s("WPM", "Windows Package Manager");
    s.beginWriteArray("repositories", urls.count());
    for (int i = 0; i < urls.count(); ++i) {
        s.setArrayIndex(i);
        s.setValue("repository", urls.at(i)->toString());
    }
    s.endArray();
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}


