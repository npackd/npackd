#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"
#include "qmetatype.h"
#include "qdir.h"
#include "qurl.h"
#include "qstringlist.h"

#include "job.h"

class PackageVersion
{
private:
    int* parts;
    int nparts;

    bool unzip(QString zipfile, QString outputdir, QString* errMsg);
    bool removeDirectory(QDir &aDir, QString* errMsg);
    bool createShortcuts(QString* errMsg);
public:
    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    /** important files (shortcuts for these will be created in the menu) */
    QStringList importantFiles;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

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
     * @param errMsg an error message will be stored here
     * @return true if the package was installed successfully
     */
    bool install(Job* job, QString* errMsg);

    /**
     * Uninstalls this package version.
     *
     * @param errMsg error message
     * @return true if the application was uninstalled successfully
     */
    bool uninstall(QString* errMsg);

    /**
     * Creates a zip file
     */
    bool MakezipDir(QString dirtozip) ;
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
