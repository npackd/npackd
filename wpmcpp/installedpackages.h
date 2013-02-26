#ifndef INSTALLEDPACKAGES_H
#define INSTALLEDPACKAGES_H

#include <windows.h>

#include <QMap>

#include "installedpackageversion.h"
#include "version.h"
#include "windowsregistry.h"
#include "job.h"

/**
 * @brief information about installed packages
 */
class InstalledPackages
{
private:
    static InstalledPackages def;

    QMap<QString, InstalledPackageVersion*> data;

    InstalledPackages();
    void detectOneDotNet(const WindowsRegistry& wr, const QString& keyName);

    void clearPackagesInNestedDirectories();

    void detectControlPanelPrograms();
    void detectControlPanelProgramsFrom(HKEY root,
            const QString& path, bool useWoWNode,
            QStringList *packagePaths,
            QStringList* foundDetectionInfos);
    void detectOneControlPanelProgram(const QString& registryPath,
            WindowsRegistry& k,
            const QString& keyName, QStringList *packagePaths,
            QStringList* foundDetectionInfos);

    void detectMSIProducts();
    void detectDotNet();
    void detectMicrosoftInstaller();
    void detectMSXML();
    void detectJRE(bool w64bit);
    void detectJDK(bool w64bit);
    void detectWindows();
    void setPackageVersionPathIfNotInstalled(const QString &package,
            const Version &version, const QString &directory);
public:
    /**
     * @return default instance
     */
    static InstalledPackages* getDefault();

    /**
     * @param exact if true, only exact matches to packages from current
     *     repositories recognized as existing software (e.g. something like
     *     com.mysoftware.MySoftware-2.2.3). This setting should help in rare
     *     cases when Npackd 1.14 and 1.15 are used in parallel for some time
     *     If the value is false, also
     *     packages not known in current repositories are recognized as
     *     installed.
     */
    void scanPre1_15Dir(bool exact);

    /**
     * Adds unknown in the repository, but installed packages.
     */
    void detectPre_1_15_Packages();

    /**
     * Recognizes some applications installed without Npackd. This method does
     * not scan the hard drive and is fast.
     *
     * @param job job object
     */
    void detect(Job* job);

    /**
     * @brief finds the specified installed package version
     * @param package full package name
     * @param version package version
     * @return found or newly created information
     */
    InstalledPackageVersion* findOrCreate(const QString& package,
            const Version& version);

    /**
     * @brief finds the specified installed package version
     * @param package full package name
     * @param version package version
     * @return found information or 0 if the specified package version is not
     *     installed. The returned object may still represent a not installed
     *     package version. Please check InstalledPackageVersion::getDirectory()
     */
    InstalledPackageVersion* find(const QString& package,
            const Version& version) const;

    /**
     * @brief registers an installed package version
     * @param package full package name
     * @param version package version
     * @param directory installation directory. This value cannot be empty.
     * @return error message
     */
    QString setPackageVersionPath(const QString& package, const Version& version,
            const QString& directory);

    /**
     * Reads the package statuses from the registry.
     */
    void readRegistryDatabase();

    /**
     * @return installed packages. Note all versions may be installed. It is
     *     also necessary to check InstalledPackageVersion::installed()
     */
    QList<InstalledPackageVersion*> getAll() const;

    /**
     * @brief paths to all installed package versions
     * @return list of directories
     */
    QStringList getAllInstalledPackagePaths() const;

    /**
     * Reloads the database about installed packages from the
     * registry and performs a quick detection of packages from the MSI database
     * and "Software" control panel. Checks also that the package versions
     * directories are still present.
     *
     * @param job job for this method
     */
    void refresh(Job* job);
};

#endif // INSTALLEDPACKAGES_H
