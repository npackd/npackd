#ifndef INSTALLEDPACKAGEVERSION_H
#define INSTALLEDPACKAGEVERSION_H

#include <QString>

/**
 * @brief information about one installed package version
 */
class InstalledPackageVersion
{
public:
    /** program location on the disk */
    QString directory;

    /**
     * @brief -
     */
    InstalledPackageVersion();
};

#endif // INSTALLEDPACKAGEVERSION_H
