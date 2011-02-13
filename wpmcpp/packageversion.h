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
#include "detectfile.h"

class InstallOperation;

/**
 * One version of a package (installed or not).
 */
class PackageVersion
{
private:    
    /** installation directory or "", if the package version is not installed */
    QString ipath;

    bool unzip(QString zipfile, QString outputdir, QString* errMsg);
    bool createShortcuts(const QString& dir, QString* errMsg);
    bool saveFiles(const QDir& d, QString* errMsg);
    void executeFile(Job* job, const QString& where,
            const QString& path, const QString& outputFile,
            const QStringList& env);
    void deleteShortcuts(const QString& dir,
            Job* job, bool menu, bool desktop, bool quickLaunch);
    QString fullText;

    /**
     * Deletes a directory. If something cannot be deleted, it waits and
     * tries to delete the directory again. Moves the directory to .Trash if
     * it cannot be move to the recycle bin.
     *
     * @param job progress for this task
     * @param dir this directory will be deleted
     */
    void removeDirectory(Job* job, const QString& dir);
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
     * this value is true for packages not installed through WPM, but detected
     * later. Those packages cannot be uninstalled, but are used for
     * dependencies.
     */
    bool external;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

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
     * @return description that can be used for the full-text search in lower
     *     case
     */
    QString getFullText();

    /**
     * Saves the installation information (path etc) in the registry.
     *
     * @return error message
     */
    QString saveInstallationInfo();

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
     * @return files currenly locked in this package directory
     */
    QStringList findLockedFiles();
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
