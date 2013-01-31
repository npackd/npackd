#ifndef DBREPOSITORY_H
#define DBREPOSITORY_H

#include <QString>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QMap>
#include <QWeakPointer>

#include "package.h"
#include "repository.h"
#include "packageversion.h"

/**
 * @brief A repository stored in an SQLite database.
 */
class DBRepository
{
private:
    static DBRepository def;
    static bool tableExists(QSqlDatabase* db,
            const QString& table, QString* err);
    static QString toString(const QSqlError& e);

    QMap<QString, QWeakPointer<Package> > packagesCache;

    QString insertPackage(Package* p);
    QString insertPackageVersion(PackageVersion* p);
    QString insertPackages(Repository* r);
    QString insertPackageVersions(Repository* r);
    QString clearPackages();
    QString clearPackageVersions();
public:
    /**
     * @return default repository
     */
    static DBRepository* getDefault();

    /**
     * @brief -
     */
    DBRepository();

    /**
     * @brief opens the database
     * @return error
     */
    QString open();

    /**
     * @brief reloadFrom clears the database and populates the table with the
     *     data from the given repository
     * @param job job
     * @param r the repository
     * @return error message
     */
    void reloadFrom(Job* job, Repository* r);

    /**
     * @brief searches for a package with the given name
     * @param name full package name
     * @return found package or 0
     */
    QSharedPointer<Package> findPackage(const QString& name);
};

#endif // DBREPOSITORY_H
