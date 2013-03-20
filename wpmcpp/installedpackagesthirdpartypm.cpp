#include <QList>
#include <QSet>
#include <QString>

#include "installedpackagesthirdpartypm.h"
#include "installedpackages.h"

InstalledPackagesThirdPartyPM::InstalledPackagesThirdPartyPM()
{
}

void InstalledPackagesThirdPartyPM::scan(
        QList<InstalledPackageVersion *> *installed, Repository *rep) const
{
    InstalledPackages* ip = InstalledPackages::getDefault();
    QList<InstalledPackageVersion*> ipvs = ip->getAll();
    QSet<QString> used;
    for (int i = 0; i < ipvs.count(); ++i) {
        InstalledPackageVersion* ipv = ipvs.at(i);
        if (!used.contains(ipv->package)) {
            QString package = ipv->package;
            int pos = package.lastIndexOf('.');
            if (pos > 1)
                package = package.right(package.count() - pos - 1);
            Package* p = new Package(package, ipv->package);
            rep->packages.append(p);
            used.insert(ipv->package);
        }
        PackageVersion* pv = new PackageVersion(ipv->package, ipv->version);
        rep->packageVersions.append(pv);
        rep->package2versions.insert(ipv->package, pv);
    }
    qDeleteAll(ipvs);
}
