#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <windows.h>

#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qdom.h"

#include "package.h"
#include "packageversion.h"
#include "node.h"
#include "digraph.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository
{
private:
    // TODO: this is never freed
    static Repository* def;

    Digraph* installedGraph;

    static Package* createPackage(QDomElement* e);
    static PackageVersionFile* createPackageVersionFile(QDomElement* e);
    static Dependency* createDependency(QDomElement* e);
    static PackageVersion* createPackageVersion(QDomElement* e);

    void loadOne(QUrl* url, Job* job);
    void addUnknownExistingPackages();
    void addWindowsPackage();

    void versionDetected(const QString& package, const Version& v);

    void detectOneDotNet(HKEY hk2, const QString& keyName);
    void detectMSIProducts();
    void detectDotNet();
    void detectMicrosoftInstaller();
    void detectJRE(bool w64bit);
    void detectJDK(bool w64bit);
    void detectNpackd();

    /**
     * Recognizes applications installed without Npackd.
     *
     * @param job job object
     */
    void recognize(Job* job);
public:
    /**
     * @return C:\Program Files\WPM - repository directory
     */
    QDir getDirectory();

    /**
     * Package versions. All version numbers should be normalized.
     */
    QList<PackageVersion*> packageVersions;

    /**
     * Packages.
     */
    QList<Package*> packages;

    /**
     * Creates an empty repository.
     */
    Repository();

    ~Repository();

    /**
     * Finds all installed packages. This method lists all directories in the
     * installation directory and finds the corresponding package versions
     *
     * @return the list of installed package versions (the objects should not
     *     be freed)
     */
    QList<PackageVersion*> getInstalled();

    /**
     * @return digraph with installed package versions. Each Node.userData is
     *     of type PackageVersion* and represents an installed package version.
     *     The memory should not be freed. The first object in the list has
     *     the userData==0 and represents the user which "depends" on a list
     *     of packages (uses some programs).
     */
    Digraph* getInstalledGraph();

    /**
     * This method should always be called after something was installed or
     * uninstalled so that the Repository object can re-calculate some internal
     * data.
     */
    void somethingWasInstalledOrUninstalled();

    /**
     * Counts the number of installed packages that can be updated.
     *
     * @return the number
     */
    int countUpdates();

    /**
     * Loads the content from the URL.
     *
     * @param job job for this method
     */
    void load(Job* job);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package or 0
     */
    Package* findPackage(const QString& name);

    /**
     * Find the newest available package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestPackageVersion(const QString& name);

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestInstalledPackageVersion(QString& name);

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
};

#endif // REPOSITORY_H
