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

    QString detectionInfo;

    InstalledPackageVersion(const QString& package,
            const Version& version,
            const QString& directory);

    /**
     * Loads the information about this package from the Windows registry.
     */
    void loadFromRegistry();
public:
    /** full package version. This value should not be chaged. */
    QString package;

    /** version number. This value should not be changed. */
    Version version;

    /**
     * @return true if this package version is installed
     */
    bool installed() const;

    /**
     * Changes the installation path for this package. This method should only
     * be used if the package was detected.
     *
     * @param path installation path
     */
    void setPath(const QString& path);

    /**
     * @return installation directory or "" if the package version is not
     *     installed
     */
    QString getDirectory() const;

    /**
     * @brief saves the information in the Windows registry
     * @return error message
     */
    QString save() const;

    /**
     * The value is not empty for externally installed and detected packages.
     * For example, packages detected from the MSI database have here the MSI
     * product GUID.
     *
     * MSI: "msi:{86ce85e6-dbac-3ffd-b977-e4b79f83c909}"
     * Contol Panel Programs: "control-panel:Active Script 1.0"
     * @return detection information
     */
    QString getDetectionInfo() const;

    /**
     * @brief changes the package version detection information.
     *     See getDetectionInfo() for more details
     * @param info new detection information
     * @return error message
     */
    QString setDetectionInfo(const QString& info);
};

#endif // INSTALLEDPACKAGEVERSION_H
