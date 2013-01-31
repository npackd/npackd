#include "dbrepository.h"

#include "QSqlDatabase"
#include "QSqlError"
#include "QSqlQuery"
#include "QDir"

DBRepository DBRepository::def;

DBRepository::DBRepository()
{
}

DBRepository* DBRepository::getDefault()
{
    return &def;
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

