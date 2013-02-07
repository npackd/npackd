#include "installedpackages.h"

InstalledPackages InstalledPackages::def;

InstalledPackages* InstalledPackages::getDefault()
{
    return &def;
}

InstalledPackages::InstalledPackages()
{
}

InstalledPackageVersion* InstalledPackages::find(const QString& package,
        const Version& version) const
{
    return this->data[package + "/" + version.getVersionString()];
}

