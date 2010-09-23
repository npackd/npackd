#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "version.h"

class PackageVersion;

/**
 * A dependency from another package.
 */
class Dependency
{
public:
    /** dependency on this package */
    QString package;

    /** ( or [ */
    bool minIncluded;

    /** lower bound */
    Version min;

    /** ) or ] */
    bool maxIncluded;

    /** high bound */
    Version max;

    /**
     * [0, 1)
     */
    Dependency();

    /**
     * @param v a version
     * @return true if the version can be used for this dependency
     */
    bool test(const Version& v);

    /**
     * @param res all package versions that match this dependency and are
     *     installed will be stored here
     */
    void findAllInstalledMatches(QList<PackageVersion*>& res);

    /**
     * @return the newest package version that matches this dependency by
     *     being installed. Never returns externally installed packages.
     */
    PackageVersion* findBestMatchToInstall();

    /**
     * @return the newest package version that matches this dependency by
     *     being installed.
     */
    PackageVersion* findHighestInstalledMatch();

    /**
     * @return true if a package, that satisfies this dependency, is installed
     */
    bool isInstalled();

    /**
     * @return human readable representation of this dependency
     */
    QString toString();
};

#endif // DEPENDENCY_H
