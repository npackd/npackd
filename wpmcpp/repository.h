#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "packageversion.h"
#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository
{
private:
    // TODO: this is never freed
    static Repository* def;
public:
    /**
     * Package versions.
     * TODO: does this leak memory?
     */
    QList<PackageVersion*> packageVersions;

    /**
     * Creates an empty repository.
     */
    Repository();

    /**
     * Loads the content from the URL.
     *
     * @param job job for this method
     */
    void load(Job* job);

    /**
     * @return newly created object pointing to the repository or 0
     */
    static QUrl* getRepositoryURL();

    /*
     * Changes the default repository url.
     *
     * @param url new URL
     */
    static void setRepositoryURL(QUrl& url);

    /**
     * @return default repository
     */
    static Repository* getDefault();
};

#endif // REPOSITORY_H
