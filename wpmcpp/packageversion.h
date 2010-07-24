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

class PackageVersion
{
private:
    int* parts;
    int nparts;

    bool unzip(QString zipfile, QString outputdir, QString* errMsg);
    bool createShortcuts(QString* errMsg);
    void deleteShortcuts();
    bool saveFiles(QString* errMsg);
public:
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

    /** 0 = zip file, 1 = one file */
    int type;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

    /**
     * @return package title
     */
    QString getPackageTitle();

    /**
     * @return only the last part of the package name (without a dot)
     */
    QString getShortPackageName();

    /**
     * Changes the version.
     *
     * @param a first version number
     * @param b second (minor) version number part
     */
    void setVersion(int a, int b);

    /**
     * Changes the version
     *
     * @param version "1.2.3"
     */
    void setVersion(QString& version);

    /**
     * @return true if this package version is installed
     */
    bool installed();

    /**
     * @return directory where this package version should be installed
     */
    QDir getDirectory();

    /**
     * @return package version as a string (like "1.2.3")
     */
    QString getVersionString();

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
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
