#ifndef ABSTRACTREPOSITORY_H
#define ABSTRACTREPOSITORY_H

#include <QList>
#include <QString>

#include "packageversion.h"
#include "package.h"

/**
 * @brief basis for repositories
 */
class AbstractRepository
{
private:
    static AbstractRepository* def;
public:
    /**
     * @return default repository
     */
    static AbstractRepository* getDefault_();

    /**
     * @param d default repository
     */
    static void setDefault_(AbstractRepository* d);

    /**
     * @brief creates a new instance
     */
    AbstractRepository();

    virtual ~AbstractRepository();

    /**
     * @brief searches for a package with the given name
     * @param name full package name
     * @return found package or 0. The returned object should be destroyed later.
     */
    virtual Package* findPackage_(const QString& name) = 0;

    /**
     * Finds all package versions.
     *
     * @param package full package name
     * @param err error message will be stored here
     * @return the list of package versions (the objects should not
     *     be freed) sorted by the version number. The first returned object
     *     has the highest version number. The returned objects should be
     *     destroyed later.
     */
    virtual QList<PackageVersion*> getPackageVersions_(
            const QString& package, QString* err) const = 0;

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0. The returned object should be
     *     destroyed later.
     */
    PackageVersion *findNewestInstalledPackageVersion_(
            const QString &name) const;

    /**
     * Find the newest installable package version.
     *
     * @param package name of the package like "org.server.Word"
     * @return found package version or 0. The returned object should be
     *     destroyed later.
     */
    PackageVersion* findNewestInstallablePackageVersion_(
            const QString& package) const;

    /**
     * @return new NPACKD_CL value
     */
    QString computeNpackdCLEnvVar_() const;

    /**
     * Changes the value of the system-wide NPACKD_CL variable to point to the
     * newest installed version of NpackdCL.
     */
    void updateNpackdCLEnvVar();

    /**
     * @brief adds an existing package version
     * @param package full package name
     * @param version version number
     */
    virtual void addPackageVersion(const QString& package,
            const Version& version) = 0;

    /**
     * @brief saves (creates or updates) the data about a package
     * @param p [ownership:copy] package
     * @return error message
     */
    virtual QString savePackage(Package* p) = 0;

    /**
     * @brief searches for a package version by the associated MSI GUID
     * @param guid MSI package GUID
     * @return [ownership:new] found package version or 0
     */
    virtual PackageVersion* findPackageVersionByMSIGUID_(
            const QString& guid) const = 0;

    /**
     * Find the newest available package version.
     *
     * @param package name of the package like "org.server.Word"
     * @param version package version
     * @return [ownership:new] found package version or 0
     */
    virtual PackageVersion* findPackageVersion_(const QString& package,
            const Version& version) = 0;
};

#endif // ABSTRACTREPOSITORY_H
