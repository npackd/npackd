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
     */
    void load();

    /**
     * @return newly created object pointing to the repository or 0
     */
    static QUrl* getRepositoryURL();

    /*
     * TODO: comment
     */
    static void setRepositoryURL(QUrl& url);

    /**
     * @return default repository
     */
    static Repository* getDefault();

    /**
     * @return directory like "C:\Program Files"
     */
    static QString getProgramFilesDir();

    /**
     * @param this URL will be downloaded
     * @return temporary file
     */
    static QTemporaryFile* download(const QUrl& url);
};

#endif // REPOSITORY_H
