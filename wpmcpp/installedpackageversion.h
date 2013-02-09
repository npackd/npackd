#ifndef INSTALLEDPACKAGEVERSION_H
#define INSTALLEDPACKAGEVERSION_H

#include <QString>

#include "version.h"

/**
 * @brief information about one installed package version
 */
class InstalledPackageVersion
{
    friend class InstalledPackages;

    QString directory;

    InstalledPackageVersion(const QString& package,
            const Version& version,
            const QString& directory);

    /**
     * Loads the information about this package from the Windows registry.
     */
    void loadFromRegistry();
public:
    /**
     * The value is not empty for externally installed and detected packages.
     * For example, packages detected from the MSI database have here the MSI
     * product GUID.
     *
     * MSI: "msi:{86ce85e6-dbac-3ffd-b977-e4b79f83c909}"
     * Contol Panel Programs: "control-panel:Active Script 1.0"
     */
    QString detectionInfo;

    /** full package version. This value should not be chaged. */
    QString package;

    /** version number. This value should not be changed. */
    Version version;

    /**
     * @return installation directory
     */
    QString getDirectory() const;

    /**
     * @brief saves the information in the Windows registry
     * @return error message
     */
    QString save() const;
};

#endif // INSTALLEDPACKAGEVERSION_H
