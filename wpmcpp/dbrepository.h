#ifndef DBREPOSITORY_H
#define DBREPOSITORY_H

#include <QString>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QMap>
#include <QWeakPointer>
#include <QMultiMap>
#include <QCache>

#include "package.h"
#include "repository.h"
#include "packageversion.h"
#include "license.h"
#include "abstractrepository.h"

/**
 * @brief A repository stored in an SQLite database.
 */
class DBRepository: public AbstractRepository
{
private:
    static DBRepository def;
    static bool tableExists(QSqlDatabase* db,
            const QString& table, QString* err);
    static QString toString(const QSqlError& e);

    QCache<QString, License> licenses;

    QString insertPackage(Package* p);
    QString insertPackageVersion(PackageVersion* p);
    QString insertLicense(License* p);

    QString insertPackages(Repository* r);
    QString insertPackageVersions(Repository* r);
    QString insertLicenses(Repository* r);

    QString exec(const QString& sql);

    void addWellKnownPackages();
    QString updateStatus(const QString &package);
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

    virtual ~DBRepository();

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
    void insertAll(Job* job, Repository* r);

    /**
     * @brief updates the status for currently installed packages in
     *     PACKAGE.STATUS
     */
    void updateStatusForInstalled();

    Package* findPackage_(const QString& name);

    QList<PackageVersion*> getPackageVersions_(const QString& package,
            QString* err) const;

    /**
     * @brief searches for packages that match the specified keywords
     * @param status filter for the package status if filterByStatus is true
     * @param filterByStatus if true, the packages will be filtered by status
     * @param query search query (keywords)
     * @return [ownership:caller] found packages
     */
    QList<Package*> findPackages(Package::Status status, bool filterByStatus,
            const QString &query) const;

    /**
     * @return [ownership:caller] found package versions
     */
    QList<PackageVersion*> findPackageVersions() const;

    /**
     * @brief loads does all the necessary updates when F5 is pressed. The
     *    repositories from the Internet are loaded and the MSI database and
     *    "Software" control panel data will be scanned.
     * @param job job
     */
    void updateF5(Job *job);

    QString savePackage(Package* p);

    QString savePackageVersion(PackageVersion* p);

    PackageVersion* findPackageVersionByMSIGUID_(const QString& guid) const;

    PackageVersion* findPackageVersion_(const QString& package,
            const Version& version);

    License* findLicense_(const QString& name);

    QString clear();
};

#endif // DBREPOSITORY_H
