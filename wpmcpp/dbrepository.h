#ifndef DBREPOSITORY_H
#define DBREPOSITORY_H

#include <QString>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QMap>
#include <QWeakPointer>
#include <QMultiMap>

#include "package.h"
#include "repository.h"
#include "packageversion.h"
#include "license.h"

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
    QMap<QString, QWeakPointer<PackageVersion> > packageVersionsCache;
    QMap<QString, QWeakPointer<License> > licensesCache;

    QString insertPackage(Package* p);
    QString insertPackageVersion(PackageVersion* p);
    QString insertLicense(License* p);

    QString insertPackages(Repository* r);
    QString insertPackageVersions(Repository* r);
    QString insertLicenses(Repository* r);

    QString exec(const QString& sql);
public:
    /**
     * @return default repository
     * @threadsafe
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

    /**
     * @brief searches for a package version
     * @param package full package name
     * @param version version number
     * @return found package or 0
     */
    QSharedPointer<PackageVersion> findPackageVersion(
            const QString& package, const Version& version);

    /**
     * @brief searches for a license
     * @param name full internal name of the license
     * @return found license or 0
     */
    QSharedPointer<License> findLicense(const QString& name);

    /**
     * Finds all versions of a package.
     *
     * @param package full package name
     * @param err error message will be stored here
     * @return the list of package versions sorted by the version number.
     *     The first returned object has the highest version number.
     */
    QList<QSharedPointer<PackageVersion> > getPackageVersions(
            const QString& package, QString* err);

    /**
     * @brief searches for packages that match the specified keywords
     * @param keywords list of keywords
     * @return found packages. The objects should be destroyed later.
     */
    QList<Package*> findPackages(const QStringList& keywords) const;
};

#endif // DBREPOSITORY_H
