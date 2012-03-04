#include "packageversionhandle.h"

PackageVersionHandle::PackageVersionHandle()
{
    this->package = "com.microsoft.Windows";
}

PackageVersionHandle::PackageVersionHandle(
        const QString& package, const Version& version)
{
    this->package = package;
    this->version = version;
}
