#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"
#include "qmetatype.h"
#include "qdir.h"
#include "qurl.h"
#include "qstringlist.h"

#include "job.h"
#include "packageversionfile.h"
#include "version.h"
#include "dependency.h"
#include "digraph.h"
#include "installoperation.h"

class InstallOperation;

class PackageVersion
{
private:
    bool unzip(QString zipfile, QString outputdir, QString* errMsg);
    bool createShortcuts(QString* errMsg);
    void deleteShortcuts(QDir& d);
    bool saveFiles(QString* errMsg);
    bool executeFile(QString& path, QString* errMsg, int timeout);
    void deleteShortcuts(Job* job, bool menu, bool desktop, bool quickLaunch);
    QString fullText;
public:
    /** package version */
    Version version;

    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    /** important files (shortcuts for these will be created in the menu) */
    QStringList importantFiles;

    /** titles for the important files */
    QStringList importantFilesTitles;

    /**
     * Packages.
     */
    QList<PackageVersionFile*> files;

    /**
     * Dependencies.
     */
    QList<Dependency*> dependencies;

    /** 0 = zip file, 1 = one file */
    int type;

    /** SHA1 for the installation file or empty if not defined */
    QString sha1;

    /**
     * this value is true for packages not installed through WPM, but detected
     * later. Those packages cannot be uninstalled, but are used for
     * dependencies.
     */
    bool external;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

    /**
     * Renames the directory for this package to a temporary name and then
     * renames it back.
     *
     * @return true if the renaming was OK (the directory is not locked)
     */
    bool isDirectoryLocked();

    /**
     * Downloads the package and computes its SHA1.
     *
     * @return SHA1
     */
    QString downloadAndComputeSHA1(Job* job);

    /**
     * Installs necessary dependencies.
     *
     * @param job job
     */
    void installDeps(Job* job);

    /**
     * Uninstalls dependant packages.
     *
     * @param state current state of the system (installed package versions)
     * @param job job
     */
    void uninstallDeps(Job* job);

    /**
     * @param res list of packages that should be uninstalled before this one is
     *     uninstalled (will be filled by this method). The packages should
     *     be uninstalled in the order they are in r.
     */
    void getUninstallFirstPackages(QList<PackageVersion*>& res);

    /**
     * @param r list of packages that should be installed before this one is
     *     installed (will be filled by this method)
     * @param unsatisfiedDeps this dependencies cannot be installed (are not
     *     available)
     */
    void getInstallFirstPackages(QList<PackageVersion*>& r,
            QList<Dependency*>& unsatisfiedDeps);

    /**
     * Plans installation of this package and all the dependencies recursively.
     *
     * @param installed list of installed packages. This list should be
     *     consulted instead of .installed() and will be updated and contains
     *     all installed package versions after the process
     * @param op necessary operations should be added here
     * @return error message or ""
     */
    QString planInstallation(QList<PackageVersion*>& installed,
            QList<InstallOperation*>& ops);

    /**
     * Plans un-installation of this package and all the dependent recursively.
     *
     * @param installed list of installed packages. This list should be
     *     consulted instead of .installed() and will be updated and contains
     *     all installed package versions after the process
     * @param op necessary operations should be added here
     * @return error message or ""
     */
    QString planUninstallation(QList<PackageVersion*>& installed,
            QList<InstallOperation*>& ops);

    /**
     * @return first unsatisfied dependency or 0
     */
    Dependency* findFirstUnsatisfiedDependency();

    /**
     * @return package title
     */
    QString getPackageTitle();

    /**
     * @return only the last part of the package name (without a dot)
     */
    QString getShortPackageName();

    /**
     * @return human readable title for this package version
     */
    QString toString();

    /**
     * @return true if this package version is installed
     */
    bool installed();

    /**
     * @return directory where this package version should be installed
     */
    QDir getDirectory();

    /**
     * @return description that can be used for the full-text search in lower
     *     case
     */
    QString getFullText();

    /**
     * .zip file for downloading
     */
    QUrl download;

    /**
     * Installs this application.
     *
     * @param job job for this method
     */
    void install(Job* job);

    /**
     * Uninstalls this package version.
     *
     * @param job job for this method
     */
    void uninstall(Job* job);

    /**
     * Updates this version to the newest available.
     *
     * @param job job for this method
     */
    void update(Job* job);

    /**
     * @return files currenly locked in this package directory
     */
    QStringList findLockedFiles();
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
