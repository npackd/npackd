#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qdom.h"

#include "package.h"
#include "packageversion.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository
{
private:
    // TODO: this is never freed
    static Repository* def;

    static Package* createPackage(QDomElement* e);
    static PackageVersionFile* createPackageVersionFile(QDomElement* e);
    static PackageVersion* createPackageVersion(QDomElement* e);

    void loadOne(QUrl* url, Job* job);
public:
    /**
     * Package versions.
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
    Package* findPackage(QString& name);

    /**
     * Find the newest available package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package or 0
     */
    PackageVersion* findNewestPackageVersion(QString& name);

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
