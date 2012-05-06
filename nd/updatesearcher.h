#ifndef UPDATESEARCHER_H
#define UPDATESEARCHER_H

#include <QString>
#include <QStringList>

#include "..\wpmcpp\job.h"
#include "..\wpmcpp\packageversion.h"
#include "..\wpmcpp\repository.h"

class UpdateSearcher
{
private:
    void setDownload(Job* job, PackageVersion* pv);

    QString findTextInPage(Job* job, const QString& url,
            const QString& regex);
    QStringList findTextsInPage(Job* job, const QString& url,
            const QString& regex);

    /**
     * @param versionRE regular expression for the version number. Only valid
     *     version numbers or valid version numbers with a one-letter suffix
     *     are allowed. Examples: "1.2", "1.2.3b"
     */
    PackageVersion* findUpdate(Job* job, const QString& package,
            const QString& versionPage,
            const QString& versionRE, QString* realVersion=0,
            bool searchForMaxVersion=false);

    PackageVersion* findGraphicsMagickUpdates(Job* job);
    PackageVersion* findGTKPlusBundleUpdates(Job* job);
    PackageVersion* findH2Updates(Job* job);
    PackageVersion* findHandBrakeUpdates(Job* job);
    PackageVersion* findImgBurnUpdates(Job* job);
    PackageVersion* findIrfanViewUpdates(Job* job);
    PackageVersion* findFirefoxUpdates(Job* job);
    PackageVersion* findAC3FilterUpdates(Job* job);
    PackageVersion* findAdobeReaderUpdates(Job* job);
    PackageVersion* findSharpDevelopUpdates(Job* job);
    PackageVersion* findXULRunnerUpdates(Job* job);    
    PackageVersion* findClementineUpdates(Job* job);
    PackageVersion* findAdvancedInstallerFreewareUpdates(Job* job,
            Repository* templ);
    PackageVersion* findAria2Updates(Job* job,
            Repository* templ);
    PackageVersion* findBlatUpdates(Job* job,
            Repository* templ);
    PackageVersion* findAria2_64Updates(Job* job,
            Repository* templ);
public:
    /**
     * -
     */
    UpdateSearcher();

    /**
     * Searches for updates.
     *
     * @param job Job description
     */
    void findUpdates(Job* job);
};

#endif // UPDATESEARCHER_H
