#ifndef PACKAGEVERSIONHANDLE_H
#define PACKAGEVERSIONHANDLE_H

#include <QString>

#include "version.h"

/**
 * Handle for a package version.
 */
class PackageVersionHandle
{
public:
    /** full package name */
    QString package;

    /** package version */
    Version version;

    /**
     * @param package full package name
     * @param version version
     */
    PackageVersionHandle(const QString& package, const Version& version);
};

#endif // PACKAGEVERSIONHANDLE_H
