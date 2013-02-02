#include "dbrepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QVariant>
#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>
#include <QByteArray>

#include "package.h"
#include "repository.h"
#include "packageversion.h"

DBRepository DBRepository::def;

DBRepository::DBRepository()
{
}

DBRepository* DBRepository::getDefault()
{
    return &def;
}

QString DBRepository::clearPackages()
{
    QSqlQuery q;
    q.exec("DELETE FROM PACKAGE");
    return toString(q.lastError());
}

QString DBRepository::exec(const QString& sql)
{
    QSqlQuery q;
    q.exec(sql);
    return toString(q.lastError());
}

QString DBRepository::insertPackage(Package* p)
{
    QSqlQuery q;
    q.prepare("INSERT INTO PACKAGE "
            "(ID, NAME, TITLE, URL, ICON, DESCRIPTION, LICENSE)"
            "VALUES(:ID, :NAME, :TITLE, :URL, :ICON, :DESCRIPTION, :LICENSE)");
    q.bindValue(":ID", QVariant(QVariant::Int));
    q.bindValue(":NAME", p->name);
    q.bindValue(":TITLE", p->title);
    q.bindValue(":URL", p->url);
    q.bindValue(":ICON", p->icon);
    q.bindValue(":DESCRIPTION", p->description);
    q.bindValue(":LICENSE", p->license);
    q.exec();
    return toString(q.lastError());
}

QString DBRepository::insertPackageVersion(PackageVersion* p)
{
    QSqlQuery q;
    q.prepare("INSERT INTO PACKAGE_VERSION "
            "(ID, NAME, PACKAGE, CONTENT)"
            "VALUES(:ID, :NAME, :PACKAGE, :CONTENT)");
    q.bindValue(":ID", QVariant(QVariant::Int));
    q.bindValue(":NAME", p->version.getVersionString());
    q.bindValue(":PACKAGE", p->package);
    QDomDocument doc;
    QDomElement root = doc.createElement("version");
    doc.appendChild(root);
    p->toXML(&root);
    QByteArray file;
    QTextStream s(&file);
    doc.save(s, 4);

    q.bindValue(":CONTENT", QVariant(file));
    q.exec();
    return toString(q.lastError());
}

bool DBRepository::tableExists(QSqlDatabase* db,
        const QString& table, QString* err)
{
    *err = "";
    QString sql = QString("SELECT name FROM sqlite_master WHERE "
            "type='table' AND name='%1'").arg(table);
    QSqlQuery q = db->exec(sql);
    *err = toString(q.lastError());

    bool e = false;
    if (err->isEmpty()) {
        e = q.next();
    }
    return e;
}

QSharedPointer<Package> DBRepository::findPackage(const QString& name)
{
    QSharedPointer<Package> r;
    QWeakPointer<Package> wp = this->packagesCache.value(name);
    r = wp.toStrongRef();

    if (!r) {
        QSqlQuery q;
        q.prepare("SELECT ID, NAME, TITLE, URL, ICON, "
                "DESCRIPTION, LICENSE FROM PACKAGE WHERE NAME = :NAME");
        q.bindValue(":NAME", name);
        q.exec();
        if (q.next()) {
            Package* p = new Package(name, q.value(2).toString());
            p->url = q.value(3).toString();
            p->icon = q.value(4).toString();
            p->description = q.value(5).toString();
            p->license = q.value(6).toString();
            r = QSharedPointer<Package>(p);
            this->packagesCache.insert(name, r);
        }
    }

    return r;
}

void DBRepository::reloadFrom(Job* job, Repository* r)
{
    if (job->shouldProceed("Starting an SQL transaction")) {
        QString err = exec("BEGIN TRANSACTION");
        if (!err.isEmpty())
            job->setErrorMessage(err);
        else
            job->setProgress(0.01);
    }

    if (job->shouldProceed("Clearing the packages table")) {
        QString err = clearPackages();
        if (!err.isEmpty())
            job->setErrorMessage(err);
        else
            job->setProgress(0.1);
    }

    if (job->shouldProceed("Inserting data in the packages table")) {
        QString err = insertPackages(r);
        if (err.isEmpty())
            job->setProgress(0.6);
        else
            job->setErrorMessage(err);
    }

    if (job->shouldProceed("Clearing the package versions table")) {
        QString err = exec("DELETE FROM PACKAGE_VERSION");
        if (!err.isEmpty())
            job->setErrorMessage(err);
        else
            job->setProgress(0.7);
    }

    if (job->shouldProceed("Inserting data in the package versions table")) {
        QString err = insertPackageVersions(r);
        if (err.isEmpty())
            job->setProgress(0.95);
        else
            job->setErrorMessage(err);
    }

    if (job->shouldProceed("Commiting the SQL transaction")) {
        QString err = exec("COMMIT");
        if (!err.isEmpty())
            job->setErrorMessage(err);
        else
            job->setProgress(1);
    }

    job->complete();
}

QString DBRepository::insertPackages(Repository* r)
{
    QString err;
    for (int i = 0; i < r->packages.count(); i++) {
        Package* p = r->packages.at(i);
        err = insertPackage(p);
        if (!err.isEmpty())
            break;
    }

    return err;
}

QString DBRepository::insertPackageVersions(Repository* r)
{
    QString err;
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* p = r->packageVersions.at(i);
        err = insertPackageVersion(p);
        if (!err.isEmpty())
            break;
    }

    return err;
}

QString DBRepository::toString(const QSqlError& e)
{
    return e.type() == QSqlError::NoError ? "" : e.text();
}

QString DBRepository::open()
{
    QString err;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString path(QDir::home().path());
    path.append(QDir::separator()).append("Npackd.db");
    path = QDir::toNativeSeparators(path);
    db.setDatabaseName(path);
    db.open();
    err = toString(db.lastError());

    bool e = false;
    if (err.isEmpty()) {
        e = tableExists(&db, "PACKAGE", &err);
    }

    if (err.isEmpty()) {
        if (!e) {
            db.exec("CREATE TABLE PACKAGE(ID INTEGER PRIMARY KEY, NAME TEXT, "
                    "TITLE TEXT, "
                    "URL TEXT, "
                    "ICON TEXT, "
                    "DESCRIPTION TEXT, "
                    "LICENSE TEXT"
                    ")");
            err = toString(db.lastError());
        }
    }

    if (err.isEmpty()) {
        e = tableExists(&db, "PACKAGE_VERSION", &err);
    }

    if (err.isEmpty()) {
        if (!e) {
            db.exec("CREATE TABLE PACKAGE_VERSION(ID INTEGER PRIMARY KEY, NAME TEXT, "
                    "PACKAGE TEXT, "
                    "CONTENT BLOB"
                    ")");
            err = toString(db.lastError());
        }
    }

    return err;
}

