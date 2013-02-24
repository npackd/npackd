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
#include <QDebug>

#include "package.h"
#include "repository.h"
#include "packageversion.h"
#include "wpmutils.h"

bool packageVersionLessThan3(const PackageVersion* a, const PackageVersion* b) {
    int r = a->package.compare(b->package);
    if (r == 0) {
        r = a->version.compare(b->version);
    }

    return r > 0;
}

DBRepository DBRepository::def;

DBRepository::DBRepository()
{
}

DBRepository* DBRepository::getDefault()
{
    return &def;
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
            "(ID, NAME, TITLE, URL, ICON, DESCRIPTION, LICENSE, FULLTEXT)"
            "VALUES(:ID, :NAME, :TITLE, :URL, :ICON, :DESCRIPTION, :LICENSE, "
            ":FULLTEXT)");
    q.bindValue(":ID", QVariant(QVariant::Int));
    q.bindValue(":NAME", p->name);
    q.bindValue(":TITLE", p->title);
    q.bindValue(":URL", p->url);
    q.bindValue(":ICON", p->icon);
    q.bindValue(":DESCRIPTION", p->description);
    q.bindValue(":LICENSE", p->license);
    q.bindValue(":FULLTEXT", (p->title + " " + p->description + " " +
            p->name).toLower());
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

QString DBRepository::insertLicense(License* p)
{
    QSqlQuery q;
    q.prepare("INSERT INTO LICENSE "
            "(ID, NAME, TITLE, DESCRIPTION, URL)"
            "VALUES(:ID, :NAME, :TITLE, :DESCRIPTION, :URL)");
    q.bindValue(":ID", QVariant(QVariant::Int));
    q.bindValue(":NAME", p->name);
    q.bindValue(":TITLE", p->title);
    q.bindValue(":DESCRIPTION", p->description);
    q.bindValue(":URL", p->url);
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

Package* DBRepository::findPackage(const QString& name)
{
    Package* r = 0;

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
        r = p;
    }

    return r;
}

PackageVersion* DBRepository::findPackageVersion(
        const QString& package, const Version& version)
{
    QString version_ = version.getVersionString();
    PackageVersion* r = 0;

    QSqlQuery q;
    q.prepare("SELECT ID, NAME, "
            "PACKAGE, CONTENT FROM PACKAGE_VERSION "
            "WHERE NAME = :NAME AND PACKAGE = :PACKAGE");
    q.bindValue(":NAME", version_);
    q.bindValue(":PACKAGE", package);
    q.exec();
    if (q.next()) {
        // TODO: handle error
        QDomDocument doc;
        int errorLine, errorColumn;
        QString err;
        if (!doc.setContent(q.value(3).toByteArray(),
                &err, &errorLine, &errorColumn))
            err = QString(
                    "XML parsing failed at line %1, column %2: %3").
                    arg(errorLine).arg(errorColumn).arg(err);

        QDomElement root = doc.documentElement();
        PackageVersion* p = PackageVersion::parse(&root, &err);

        // TODO: handle this error
        if (err.isEmpty()) {
            r = p;
        }
    }

    return r;
}

QList<PackageVersion*> DBRepository::getPackageVersions_(
        const QString& package, QString* err) const
{
    return this->getPackageVersions(package, err);
}

QList<PackageVersion*> DBRepository::getPackageVersions(
        const QString& package, QString* err) const
{
    *err = "";

    QList<PackageVersion*> r;

    QSqlQuery q;
    q.prepare("SELECT ID, NAME, "
            "PACKAGE, CONTENT FROM PACKAGE_VERSION "
            "WHERE PACKAGE = :PACKAGE");
    q.bindValue(":PACKAGE", package);
    if (!q.exec()) {
        *err = toString(q.lastError());
    }

    while (err->isEmpty() && q.next()) {
        QDomDocument doc;
        int errorLine, errorColumn;
        if (!doc.setContent(q.value(3).toByteArray(),
                err, &errorLine, &errorColumn)) {
            *err = QString(
                    "XML parsing failed at line %1, column %2: %3").
                    arg(errorLine).arg(errorColumn).arg(*err);
        }

        QDomElement root = doc.documentElement();

        if (err->isEmpty()) {
            PackageVersion* pv = PackageVersion::parse(&root, err);
            if (err->isEmpty())
                r.append(pv);
        }
    }

    // qDebug() << vs.count();

    qSort(r.begin(), r.end(), packageVersionLessThan3);

    return r;
}

License *DBRepository::findLicense(const QString& name)
{
    License* r = 0;

    QSqlQuery q;
    q.prepare("SELECT ID, NAME, TITLE, DESCRIPTION, URL "
            "FROM LICENSE "
            "WHERE NAME = :NAME");
    q.bindValue(":NAME", name);
    q.exec();
    if (q.next()) {
        // TODO: handle error
        License* p = new License(name, q.value(2).toString());
        p->description = q.value(3).toString();
        p->url = q.value(4).toString();
        r = p;
    }

    return r;
}

QList<Package*> DBRepository::findPackages(const QStringList& keywords) const
{
    // TODO: not all keywords are used
    // TODO: errors are not handled

    QList<Package*> r;

    QSqlQuery q;
    q.prepare("SELECT ID, NAME, TITLE, URL, ICON, "
            "DESCRIPTION, LICENSE FROM PACKAGE WHERE FULLTEXT LIKE :FULLTEXT");
    q.bindValue(":FULLTEXT", keywords.count() > 0 ?
            "%" + keywords.at(0).toLower() + "%" : "%");
    q.exec();
    while (q.next()) {
        Package* p = new Package(q.value(1).toString(), q.value(2).toString());
        p->url = q.value(3).toString();
        p->icon = q.value(4).toString();
        p->description = q.value(5).toString();
        p->license = q.value(6).toString();
        r.append(p);
    }

    return r;
}

PackageVersion* DBRepository::findNewestInstalledPackageVersion(
        const QString &name) const
{
    PackageVersion* r = 0;

    QString err; // TODO: error is not handled
    QList<PackageVersion*> pvs = this->getPackageVersions_(name, &err);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* p = pvs.at(i);
        if (p->installed()) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }

    if (r)
        r = r->clone();

    qDeleteAll(pvs);

    return r;
}

QString DBRepository::computeNpackdCLEnvVar() const
{
    QString v;
    PackageVersion* pv;
    if (WPMUtils::is64BitWindows())
        pv = findNewestInstalledPackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL64");
    else
        pv = 0;

    if (!pv)
        pv = findNewestInstalledPackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL");

    if (pv)
        v = pv->getPath();

    delete pv;

    return v;
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
        QString err = exec("DELETE FROM PACKAGE");
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

    if (job->shouldProceed("Clearing the licenses table")) {
        QString err = exec("DELETE FROM LICENSE");
        if (!err.isEmpty())
            job->setErrorMessage(err);
        else
            job->setProgress(0.96);
    }

    if (job->shouldProceed("Inserting data in the licenses table")) {
        QString err = insertLicenses(r);
        if (err.isEmpty())
            job->setProgress(0.98);
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

QString DBRepository::insertLicenses(Repository* r)
{
    QString err;
    for (int i = 0; i < r->licenses.count(); i++) {
        License* p = r->licenses.at(i);
        err = insertLicense(p);
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
                    "LICENSE TEXT, "
                    "FULLTEXT TEXT"
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

    if (err.isEmpty()) {
        e = tableExists(&db, "LICENSE", &err);
    }

    if (err.isEmpty()) {
        if (!e) {
            db.exec("CREATE TABLE LICENSE(ID INTEGER PRIMARY KEY, NAME TEXT, "
                    "TITLE TEXT, "
                    "DESCRIPTION TEXT, "
                    "URL TEXT"
                    ")");
            err = toString(db.lastError());
        }
    }

    return err;
}

