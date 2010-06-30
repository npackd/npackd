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
     * @param errMsg the error message will be stored here
     * @return true if the repository was loaded successfully
     */
    bool load(QString* errMsg);

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

    /**
     * @return directory like "C:\Program Files"
     */
    static QString getProgramFilesDir();

    /**
     * @param this URL will be downloaded
     * @param errMsg error message will be stored here
     * @return temporary file or 0 if an error occured
     */
    static QTemporaryFile* download(const QUrl& url, QString* errMsg);
};

#endif // REPOSITORY_H
