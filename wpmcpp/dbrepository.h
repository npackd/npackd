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

    QString insertPackage(Package* p);
    QString insertPackageVersion(PackageVersion* p);
    QString insertLicense(License* p);

    QString insertPackages(Repository* r);
    QString insertPackageVersions(Repository* r);
    QString insertLicenses(Repository* r);

    QString exec(const QString& sql);

    void addWellKnownPackages();
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
    void insertAll(Job* job, Repository* r);

    /**
     * @brief searches for a package with the given name
     * @param name full package name
     * @return [ownership:caller] found package or 0
     */
    Package* findPackage(const QString& name);

    Package* findPackage_(const QString& name);

    /**
     * @brief searches for a package version
     * @param package full package name
     * @param version version number
     * @return [ownership:caller] found package or 0
     */
    PackageVersion* findPackageVersion(
            const QString& package, const Version& version);

    /**
     * @brief searches for a license
     * @param name full internal name of the license
     * @return found license or 0. The returned object should be destroyed later.
     */
    License* findLicense(const QString& name);

    /**
     * Finds all versions of a package.
     *
     * @param package full package name
     * @param err error message will be stored here
     * @return the list of package versions sorted by the version number.
     *     The first returned object has the highest version number. The objects
     *     must be destroyed later.
     */
    QList<PackageVersion*> getPackageVersions(const QString& package,
            QString* err) const;

    QList<PackageVersion*> getPackageVersions_(const QString& package,
            QString* err) const;

    /**
     * @brief searches for packages that match the specified keywords
     * @param keywords list of keywords
     * @return [ownership:caller] found packages
     */
    QList<Package*> findPackages(const QStringList& keywords) const;

    /**
     * @return [ownership:caller] found package versions
     */
    QList<PackageVersion*> findPackageVersions() const;

    /**
     * @return new NPACKD_CL value
     */
    QString computeNpackdCLEnvVar() const;

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0. The returned object should be
     *     destroyed later.
     */
    PackageVersion *findNewestInstalledPackageVersion(const QString &name) const;

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
