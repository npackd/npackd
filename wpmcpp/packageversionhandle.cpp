#include "packageversionhandle.h"

PackageVersionHandle::PackageVersionHandle(
        const QString& package, const Version& version)
{
    this->package = package;
    this->version = version;
}
