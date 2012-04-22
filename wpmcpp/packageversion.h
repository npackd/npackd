#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"
#include "qmetatype.h"
#include "qdir.h"
#include "qurl.h"
#include "qstringlist.h"
#include <QSemaphore>

#include "job.h"
#include "packageversionfile.h"
#include "version.h"
#include "dependency.h"
#include "installoperation.h"
#include "detectfile.h"

class InstallOperation;

/**
 * One version of a package (installed or not).
 */
class PackageVersion
{
private:    
    static QSemaphore httpConnections;
    static QSemaphore installationScripts;

    /** installation directory or "", if the package version is not installed */
    QString ipath;

    /**
     * If true, this package version is locked and cannot be
     * installed/uninstalled.
     */
    volatile bool locked;

    QString fullText;

    bool external_;

    void unzip(Job* job, QString zipfile, QString outputdir);
    bool createShortcuts(const QString& dir, QString* errMsg);
    QString saveFiles(const QDir& d);
    QString executeFile(Job* job, const QString& where,
            const QString& path, const QString& outputFile,
            const QStringList& env);
    void deleteShortcuts(const QString& dir,
            Job* job, bool menu, bool desktop, bool quickLaunch);
    /**
     * Deletes a directory. If something cannot be deleted, it waits and
     * tries to delete the directory again. Moves the directory to .Trash if
     * it cannot be move to the recycle bin.
     *
     * @param job progress for this task
     * @param dir this directory will be deleted
     */
    void removeDirectory(Job* job, const QString& dir);

    /**
     * Saves the installation information (path etc) in the registry.
     *
     * @return error message
     */
    QString saveInstallationInfo();

    void emitStatusChanged();
    void addDependencyVars(QStringList* vars);
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
     * Package detection
     */
    QList<DetectFile*> detectFiles;

    /**
     * Dependencies.
     */
    QList<Dependency*> dependencies;

    /** 0 = zip file, 1 = one file */
    int type;

    /** SHA1 for the installation file or empty if not defined */
    QString sha1;

    /**
     * .zip file for downloading
     */
    QUrl download;

    /**
     * MSI GUID like {1D2C96C3-A3F3-49E7-B839-95279DED837F} or ""
     * if not available. Should be always in lower case
     */
    QString msiGUID;

    /**
     * The value is not empty for externally installed and detected packages.
     * For example, packages detected from the MSI database have here the MSI
     * product GUID.
     *
     * MSI: "msi:{86ce85e6-dbac-3ffd-b977-e4b79f83c909}"
     * Contol Panel Programs: "control-panel:Active Script 1.0"
     */
    QString detectionInfo;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

    /**
     * Locks this package version so that it cannot be installed or removed
     * by other processes.
     */
    void lock();

    /**
     * Unlocks this package version so that it can be installed or removed
     * again.
     */
    void unlock();

    /**
     * @return true if this package version is locked and cannot be installed
     *     or removed
     */
    bool isLocked() const;

    /**
     * @return this value is true for packages not installed through WPM,
     * but detected
     * later. Those packages cannot be uninstalled, but are used for
     * dependencies.
     */
    bool isExternal() const;

    /**
     * @param e true = externally installed
     */
    void setExternal(bool e);

    /**
     * Loads the information about this package from the Windows registry.
     */
    void loadFromRegistry();

    /**
     * @return installation path or "" if the package is not installed
     */
    QString getPath();

    /**
     * Changes the installation path for this package. This method should only
     * be used if the package was detected.
     *
     * @param path installation path
     */
    void setPath(const QString& path);

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
     * Returns the extension of the package file (quessing from the URL).
     *
     * @return e.g. ".exe" or ".zip". Never returns an empty string
     */
    QString getFileExtension();

    /**
     * Downloads the package.
     *
     * @param filename target file
     */
    void downloadTo(Job* job, QString filename);

    /**
     * Plans installation of this package and all the dependencies recursively.
     *
     * @param installed list of installed packages. This list should be
     *     consulted instead of .installed() and will be updated and contains
     *     all installed package versions after the process
     * @param op necessary operations will be appended here. The existing
     *     elements will not be modified in any way.
     * @param avoid list of package versions that cannot be installed. The list
     *     will be changed by this method. Normally this is an empty list and
     *     objects will be added to it on different recursion levels.
     * @return error message or ""
     */
    QString planInstallation(QList<PackageVersion*>& installed,
            QList<InstallOperation*>& ops, QList<PackageVersion*>& avoid);

    /**
     * Plans un-installation of this package and all the dependent recursively.
     *
     * @param installed list of installed packages. This list should be
     *     consulted instead of .installed() and will be updated and contains
     *     all installed package versions after the process. The list will also
     *     be updated to reflect packages "uninstalled" by this method
     * @param op necessary operations will be added here. The existing
     *     elements will not be modified in any way.
     * @return error message or ""
     */
    QString planUninstallation(QList<PackageVersion*>& installed,
            QList<InstallOperation*>& ops);

    /**
     * @return package title
     */
    QString getPackageTitle() const;

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
    bool installed() const;

    /**
     * @return description that can be used for the full-text search in lower
     *     case
     */
    QString getFullText();

    /**
     * @return a non-existing directory where this package would normally be
     *     installed (e.g. C:\Program Files\My Prog 2.3.2)
     */
    QString getPreferredInstallationDirectory();

    /**
     * Installs this package (without dependencies).
     *
     * @param job job for this method
     * @param where a non-existing directory
     */
    void install(Job* job, const QString& where);

    /**
     * Uninstalls this package version.
     *
     * @param job job for this method
     */
    void uninstall(Job* job);

    /**
     * @return files currenly locked in this package directory
     */
    QStringList findLockedFiles();

    /**
     * @return status like "locked, installed"
     */
    QString getStatus() const;

    /**
     * @return XML representation of this package version
     */
    QString serialize() const;
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
