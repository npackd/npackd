#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <xapian.h>

#include <windows.h>

#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qdom.h"
#include <QReadWriteLock>
#include <QHash>

#include "package.h"
#include "packageversion.h"
#include "license.h"
#include "windowsregistry.h"
#include "installedpackageversion.h"
#include "packageversionhandle.h"
#include "abstractrepository.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository: public AbstractRepository
{
    Q_OBJECT
private:
    static Repository def;

    static License* createLicense(QDomElement* e);

    Xapian::WritableDatabase* db;
    Xapian::Enquire* enquire;
    Xapian::QueryParser* queryParser;
    Xapian::TermGenerator indexer;
    Xapian::Stem stemmer;

    /**
     * @param sha1 0 or a pointer to a string where the SHA1 of the downloaded
     *     file will be stored
     */
    void loadOne(QTemporaryFile* f, Job* job, bool index);

    /**
     * @param query search query for packages
     * @return found packages (should be destroyed)
     */
    QList<Package*> find(const Xapian::Query& query) const;

    /**
     * @param query search query for package versions
     * @return found package versions. The
     *     objects in the list should be destroyed.
     */
    QList<PackageVersion*> findPackageVersions(const Xapian::Query& query) const;

    void clearExternallyInstalled(QString package);

    /**
     * Loads the information about a package from the Windows registry.
     */
    void loadInstallationInfoFromRegistry(const QString& package,
            const Version& version);

    void detectOneDotNet(const WindowsRegistry& wr, const QString& keyName);
    void detectMSIProducts();
    void detectDotNet();
    void detectMicrosoftInstaller();
    void detectMSXML();
    void detectJRE(bool w64bit);
    void detectJDK(bool w64bit);
    void detectWindows();

    /**
     * @param exact if true, only exact matches to packages from current
     *     repositories recognized as existing software (e.g. something like
     *     com.mysoftware.MySoftware-2.2.3). This setting should help in rare
     *     cases when Npackd 1.14 and 1.15 are used in parallel for some time
     *     If the value is false, also
     *     packages not known in current repositories are recognized as
     *     installed.
     */
    void scanPre1_15Dir(bool exact);

    /**
     * All paths should be in lower case
     * and separated with \ and not / and cannot end with \.
     *
     * @param path directory
     * @param ignore ignored directories
     */
    void scan(const QString& path, Job* job, int level, QStringList& ignore);

    /**
     * Adds unknown in the repository, but installed packages.
     */
    void detectPre_1_15_Packages();

    void indexCreateDocument(PackageVersion* pv, Xapian::Document& doc);
    void indexCreateDocument(Package* p, Xapian::Document& doc);
    QString indexUpdatePackageVersion(PackageVersion* pv);
public:
    /**
     * @return newly created object pointing to the repositories
     */
    static QList<QUrl*> getRepositoryURLs();

    /*
     * Changes the default repository url.
     *
     * @param urls new URLs
     */
    static void setRepositoryURLs(QList<QUrl*>& urls);

    /**
     * @return default repository
     */
    static Repository* getDefault();

    /** locked package versions */
    QList<PackageVersionHandle*> locked;

    /**
     * Licenses.
     */
    QList<License*> licenses;

    /**
     * Information about installed package versions.
     * TODO:         saveInstallationInfo();   emitStatusChanged();
     */
    QList<InstalledPackageVersion*> installedPackageVersions;

    /**
     * Creates an empty repository.
     */
    Repository();

    virtual ~Repository();

    void process(Job* job, const QList<InstallOperation*> &install);

    /**
     * @param package full package name
     * @param version package version
     * @return true if the package version is locked by a running operation
     */
    bool isLocked(const QString& package, const Version& version) const;

    /**
     * Locks a package version.
     *
     * @param package full package name
     * @param version package version
     * @return true if the package version is locked by a running operation
     */
    void lock(const QString& package, const Version& version);

    /**
     * Unlocks a package version.
     *
     * @param package full package name
     * @param version package version
     * @return true if the package version is locked by a running operation
     */
    void unlock(const QString& package, const Version& version);

    /**
     * @param pv package version
     * @return installation information for a package version or 0
     */
    InstalledPackageVersion* findInstalledPackageVersion(
            const PackageVersion* pv);

    /**
     * @param package full package name
     * @param version package version
     * @return installation information for a package version or 0
     */
    InstalledPackageVersion* findInstalledPackageVersion(
            const QString& package, const Version& version) const;

    /**
     * Removes a package version from the list of installed.
     *
     * @param pv a package version
     */
    void removeInstalledPackageVersion(PackageVersion* pv);

    /**
     * Adds a package version to the list of installed if it is not already
     * there.
     *
     * @param package full package name
     * @param version package version
     * @param ipath installation path
     * @param external true = external
     */
    void addInstalledPackageVersionIfAbsent(const QString& package,
            const Version& version, const QString& ipath, bool external);

    /**
     * Writes this repository to an XML file.
     *
     * @param filename output file name
     * @return error message or ""
     */
    // TODO: QString writeTo(const QString& filename) const;

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
     * @param index true = index the contents
     */
    void loadOne(QDomDocument* doc, Job* job, bool index);

    /**
     * @param package package name
     * @return all package versions for the specified package ordered by the
     *     version number in increasing order (older versions first)
     */
    QList<PackageVersion*> getPackageVersions(QString package) const;

    /**
     * @param package package name
     * @return all package versions for the specified package ordered by the
     *     version number in increasing order (older versions first)
     */
    QList<Version> getPackageVersions2(QString package) const;

    /**
     * @param package package name
     * @return installed package versions for the specified package ordered by
     *     the version number in increasing order (older versions first)
     */
    QList<Version> getInstalledPackageVersions(QString package) const;

    /**
     * Reads the package statuses from the registry.
     */
    void readRegistryDatabase();

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
     * Recognizes some applications installed without Npackd. This method does
     * not scan the hard drive and is fast.
     *
     * @param job job object
     */
    void recognize(Job* job);

    /**
     * Finds all installed packages. This method lists all directories in the
     * installation directory and finds the corresponding package versions
     *
     * @return the list of installed package versions (the objects should not
     *     be freed)
     */
    QList<PackageVersion*> getInstalled();

    /**
     * Reloads all repositories.
     *
     * @param job job for this method
     */
    void reload(Job* job);

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
     */
    void scanHardDrive(Job* job);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package or 0. The returned object should be destroyed.
     */
    Package* findPackage(const QString& name) const;

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word" or the short
     *     name "Word"
     * @return found packages. The returned objects should be destroyed.
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
     * @return found package version or 0. The returned object should be
     *     destroyed.
     */
    PackageVersion* findNewestInstallablePackageVersion(const QString& package);

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0. The returned object should be
     *     destroyed.
     */
    PackageVersion* findNewestInstalledPackageVersion(const QString& name);

    /**
     * Find the newest available package version.
     *
     * @param package name of the package like "org.server.Word"
     * @param version package version
     * @return found package version or 0. The returned object should be
     *     destroyed.
     */
    PackageVersion* findPackageVersion(const QString& package,
            const Version& version) const;

    /**
     * Checks for existance of a package version.
     *
     * @param package name of the package like "org.server.Word"
     * @param version package version
     * @return true if the package version exists
     */
    bool packageVersionExists(const QString& package,
            const Version& version) const;

    /**
     * @return the first found locked PackageVersion or 0
     */
    PackageVersion* findLockedPackageVersion() const;

    /**
     * @param text search terms
     * @param type 0 = all, 1 = installed, 2 = installed, updateable
     * @param warning a warning is stored here
     * @return found packages (should be destroyed)
     */
    QList<Package*> find(const QString& text, int type,
            QString* warning);

    /**
     * @param package full package name or "" if versions for all packages
     *     should be returned
     * @param type 0 = all, 1 = detectable
     * @return package versions that can be detected (via MSI or SHA1). The
     *     objects in the list should be destroyed.
     */
    QList<PackageVersion*> findPackageVersions(const QString& package,
            int type) const;

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
