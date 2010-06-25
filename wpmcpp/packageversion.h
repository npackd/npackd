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
public:
    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();

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
     */
    void install();

    /**
     * Uninstalls this package version.
     */
    void uninstall();

    /* TODO comment
      */
    bool MakezipDir( QString dirtozip ) ;

    /* TODO comment
      */
    bool  UnzipTo( QString zipfile, QString outputdir ) ;

    /** TODO: comment */
    bool RemoveDirectory(QDir &aDir);
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
