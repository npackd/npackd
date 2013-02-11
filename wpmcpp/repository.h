#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <windows.h>

#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qdom.h"
#include <QMutex>
#include <QMultiMap>

#include "package.h"
#include "packageversion.h"
#include "license.h"
#include "windowsregistry.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository: public QObject
{
    Q_OBJECT
private:
    static Repository def;

    static Package* createPackage(QDomElement* e, QString* err);
    static License* createLicense(QDomElement* e);
    static PackageVersion* createPackageVersion(QDomElement* e,
            QString* err);

    void loadOne(QUrl* url, Job* job, bool useCache=true);

    void addWindowsPackage();

    /**
     * All paths should be in lower case
     * and separated with \ and not / and cannot end with \.
     *
     * @param path directory
     * @param ignore ignored directories
     */
    void scan(const QString& path, Job* job, int level, QStringList& ignore);

    /**
     * Loads the content from the URLs. None of the packages has the information
     * about installation path after this method was called.
     *
     * @param job job for this method
     * @param useCache true = cache will be used
     */
    void load(Job* job, bool useCache=true);

    void addWellKnownPackages();

    void clearPackagesInNestedDirectories();

    /**
     * @param hk root key
     * @param path registry path
     * @param err error message will be stored here
     * @return list of repositories in the specified registry key
     */
    static QStringList getRepositoryURLs(HKEY hk, const QString &path,
            QString *err);
public:
    /** full package name -> all defined package versions */
    QMultiMap<QString, PackageVersion*> package2versions;

    /**
     * @brief any operation (reading or writing) on repositories, packages,
     *     package versions, licenses etc. should be done under this lock
     */
    static QMutex mutex;

    /**
     * Checks the directories of packages in the uninstall operations in the
     * given list for locked files.
     *
     * @return error message if a file or a directory is locked and cannot be
     *     uninstalled
     */
    static QString checkLockedFilesForUninstall(
            const QList<InstallOperation*> &install);

    /**
     * @param err error message will be stored here
     * @return newly created list of repositories
     */
    static QList<QUrl*> getRepositoryURLs(QString *err);

    /*
     * Changes the default repository url.
     *
     * @param urls new URLs
     * @param err error message will be stored here
     */
    static void setRepositoryURLs(QList<QUrl*>& urls, QString *err);

    /**
     * @return default repository
     */
    static Repository* getDefault();

    /**
     * @brief searches for a package version by the associated MSI GUID
     * @param guid MSI package GUID
     * @return found package version or 0
     */
    PackageVersion* findPackageVersionByMSIGUID(
            const QString& guid) const;

    /**
     * @brief paths to all installed package versions
     * @return list of directories
     */
    QStringList getAllInstalledPackagePaths() const;

    /**
     * Package versions. All version numbers should be normalized.
     */
    QList<PackageVersion*> packageVersions;

    /**
     * Packages.
     */
    QList<Package*> packages;

    /**
     * Licenses.
     */
    QList<License*> licenses;

    /**
     * Creates an empty repository.
     */
    Repository();

    virtual ~Repository();

    void process(Job* job, const QList<InstallOperation*> &install);

    /**
     * Plans updates for the given packages.
     *
     * @param packages these packages should be updated. No duplicates are
     *     allowed here
     * @param ops installation operations will be appended here
     * @return error message or ""
     */
    QString planUpdates(const QList<Package*> packages,
            QList<InstallOperation*>& ops);

    /**
     * Loads one repository from an XML document.
     *
     * @param doc repository
     * @param job Job
     */
    void loadOne(QDomDocument* doc, Job* job);

    /**
     * Loads one repository from a file.
     *
     * @param filename repository file name
     * @param job Job
     */
    void loadOne(const QString& filename, Job* job);

    /**
     * Changes the value of the system-wide NPACKD_CL variable to point to the
     * newest installed version of NpackdCL.
     */
    void updateNpackdCLEnvVar();

    /**
     * @return new NPACKD_CL value
     */
    QString computeNpackdCLEnvVar();

    /**
     * Writes this repository to an XML file.
     *
     * @param filename output file name
     * @return error message or ""
     */
    QString writeTo(const QString& filename) const;

    /**
     * Finds or creates a new package version.
     *
     * @param package package name
     * @param v found version
     * @return package version
     */
    PackageVersion* findOrCreatePackageVersion(const QString &package,
            const Version &v);

    /**
     * Finds all installed packages.
     *
     * @return the list of installed package versions (the objects should not
     *     be freed)
     */
    QList<PackageVersion*> getInstalled();

    /**
     * Finds all package versions.
     *
     * @param package full package name
     * @return the list of package versions (the objects should not
     *     be freed) sorted by the version number. The first returned object
     *     has the highest version number.
     */
    QList<PackageVersion*> getPackageVersions(const QString& package) const;

    /**
     * Finds all installed package versions.
     *
     * @param package full package name
     * @return the list of installed package versions (the objects should not
     *     be freed) sorted by the version number. The first returned object
     *     has the highest version number.
     */
    QList<PackageVersion*> getInstalledPackageVersions(
            const QString& package) const;

    /**
     * Counts the number of installed packages that can be updated.
     *
     * @return the number
     */
    int countUpdates();

    /**
     * Reloads all repositories.
     *
     * @param job job for this method
     * @param useCache true = cache will be used
     */
    void reload(Job* job, bool useCache=true);

    /**
     * Reloads the database about installed packages from the
     * registry and performs a quick detection of packages.
     *
     * @param job job for this method
     */
    void refresh(Job* job);

    /**
     * Scans the hard drive for existing applications.
     *
     * @param job job for this method
     * @threadsafe
     */
    void scanHardDrive(Job* job);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package or 0
     */
    Package* findPackage(const QString& name);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word" or the short
     *     name "Word"
     * @return found packages
     */
    QList<Package*> findPackages(const QString& name);

    /**
     * Searches for a license by name.
     *
     * @param name name of the license like "org.gnu.GPLv3"
     * @return found license or 0
     */
    License* findLicense(const QString& name);

    /**
     * Find the newest installable package version.
     *
     * @param package name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestInstallablePackageVersion(const QString& package);

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestInstalledPackageVersion(const QString& name);

    /**
     * Find the newest available package version.
     *
     * @param package name of the package like "org.server.Word"
     * @param version package version
     * @return found package version or 0
     */
    PackageVersion* findPackageVersion(const QString& package,
            const Version& version);

    /**
     * @return the first found locked PackageVersion or 0
     */
    PackageVersion* findLockedPackageVersion() const;

    /**
     * Emits the statusChanged(PackageVersion*) signal.
     *
     * @param pv this PackageVersion has changed
     */
    void fireStatusChanged(PackageVersion* pv);
signals:
    /**
     * This signal will be fired each time the status of a package changes.
     * For example, this happens if a package is installed.
     */
    void statusChanged(PackageVersion* pv);
};

#endif // REPOSITORY_H
