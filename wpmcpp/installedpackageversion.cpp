#include "installedpackageversion.h"

InstalledPackageVersion::InstalledPackageVersion(Package* package,
        const Version& version,
        const QString& ipath, bool external)
{
    this->package_ = package;
    this->version = version;
    this->ipath = ipath;
    this->external_ = external;
}
