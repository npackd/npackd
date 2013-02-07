#ifndef INSTALLEDPACKAGES_H
#define INSTALLEDPACKAGES_H

#include <QMap>

#include "installedpackageversion.h"
#include "version.h"

/**
 * @brief information about installed packages
 */
class InstalledPackages
{
private:
    static InstalledPackages def;

    QMap<QString, InstalledPackageVersion*> data;

    InstalledPackages();
public:
    /**
     * @return default instance
     */
    static InstalledPackages* getDefault();

    /**
     * @brief finds the specified installed package version
     * @param package full package name
     * @param version package version
     * @return found information or 0 if the specified package version is not
     *     installed
     */
    InstalledPackageVersion* find(const QString& package,
            const Version& version) const;
};

#endif // INSTALLEDPACKAGES_H
