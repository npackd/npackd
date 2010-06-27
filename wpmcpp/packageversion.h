#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"
#include "qmetatype.h"
#include "qdir.h"
#include "qurl.h"

class PackageVersion
{
private:
    int* parts;
    int nparts;

    bool unzip(QString zipfile, QString outputdir, QString* errMsg);
public:
    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

    /**
     * TODO:  comment
     */
    void setVersion(int a, int b);

    /**
     * TODO:  comment
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
     * @param errMsg an error message will be stored here
     * @return true if the package was installed successfully
     */
    bool install(QString* errMsg);

    /**
     * Uninstalls this package version.
     */
    void uninstall();

    /* TODO comment
      */
    bool MakezipDir( QString dirtozip ) ;

    /** TODO: comment */
    bool RemoveDirectory(QDir &aDir);
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
