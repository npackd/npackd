#ifndef INSTALLEDPACKAGEVERSION_H
#define INSTALLEDPACKAGEVERSION_H

#include <QString>

#include "version.h"
#include "package.h"

class InstalledPackageVersion
{
public:
    /**
     * true = externally installed package (not by Npackd).
     * Those packages cannot be uninstalled, but are used for
     * dependencies.
     */
    bool external_;

    /** installation directory or "", if the package version is not installed */
    QString ipath;

    /** package version */
    Version version;

    /**
     * package definition
     */
    QString package_;

    /**
     * @param package referenced package
     * @param version package version
     * @param ipath installation path (e.g. C:\Program Files\MyPackage). This
     *     cannot be empty
     * @param external true = externally (not by Npackd) installed package
     */
    InstalledPackageVersion(const QString& package, const Version& version,
            const QString& ipath, bool external);
};

#endif // INSTALLEDPACKAGEVERSION_H
