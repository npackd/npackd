#ifndef UPDATESEARCHER_H
#define UPDATESEARCHER_H

#include <QString>

#include "..\wpmcpp\job.h"
#include "..\wpmcpp\packageversion.h"

class UpdateSearcher
{
private:
    void setDownload(Job* job, PackageVersion* pv);
    QString findTextInPage(Job* job, const QString& url,
            const QString& regex);
    PackageVersion* findUpdate(Job* job, const QString& package,
            const QString& versionPage,
            const QString& versionRE);
    PackageVersion* findGraphicsMagickUpdates(Job* job);
    PackageVersion* findGTKPlusBundleUpdates(Job* job);
    PackageVersion* findH2Updates(Job* job);
    PackageVersion* findHandBrakeUpdates(Job* job);
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
