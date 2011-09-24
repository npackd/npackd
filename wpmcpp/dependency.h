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
     * @param avoid list of package versions that should be avoided and cannot
     *     be considered to be a match
     * @return the newest package version that matches this dependency by
     *     being installed. Never returns externally installed packages.
     */
    PackageVersion* findBestMatchToInstall(const QList<PackageVersion*>& avoid);

    /**
     * Changes the versions.
     *
     * @param versions something like "[2.12, 3.4)"
     */
    bool setVersions(const QString versions);

    /**
     * Checks whether this dependency is automatically fulfilled if the
     * specified dependency is already fulfilled.
     *
     * @param dep another dependency
     * @return true if this dependency is automatically fulfilled
     */
    bool autoFulfilledIf(const Dependency& dep);

    /**
     * @return the newest package version that matches this dependency and are
     *     installed
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
