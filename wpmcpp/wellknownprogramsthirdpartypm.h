#ifndef WELLKNOWNPROGRAMSTHIRDPARTYPM_H
#define WELLKNOWNPROGRAMSTHIRDPARTYPM_H

#include <QList>

#include "abstractthirdpartypm.h"
#include "installedpackageversion.h"
#include "repository.h"
#include "windowsregistry.h"

/**
 * @brief detects some well-known programs like .NET or Java
 */
class WellKnownProgramsThirdPartyPM: public AbstractThirdPartyPM
{
    void scanDotNet(QList<InstalledPackageVersion*>* installed,
            Repository* rep) const;
    void detectOneDotNet(
            QList<InstalledPackageVersion *> *installed,
            Repository *rep,
            const WindowsRegistry& wr,
            const QString& keyName) const;
    void detectMSXML(QList<InstalledPackageVersion *> *installed,
            Repository *rep) const;
    void detectWindows(QList<InstalledPackageVersion *> *installed,
            Repository *rep) const;
    void detectJRE(QList<InstalledPackageVersion *> *installed,
            Repository *rep, bool w64bit) const;
    void detectJDK(QList<InstalledPackageVersion *> *installed,
            Repository *rep, bool w64bit) const;
    void detectMicrosoftInstaller(QList<InstalledPackageVersion *> *installed,
            Repository *rep) const;
public:
    void scan(QList<InstalledPackageVersion*>* installed,
              Repository* rep) const;
};

#endif // WELLKNOWNPROGRAMSTHIRDPARTYPM_H
