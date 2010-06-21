#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <qlist.h>
#include "packageversion.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository
{
private:
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
     * @return default repository
     */
    static Repository* getDefault();
};

#endif // REPOSITORY_H
