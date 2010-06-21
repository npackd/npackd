#include "packageversion.h"

PackageVersion::PackageVersion(const QString& package)
{
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
    this->package = package;
}

PackageVersion::~PackageVersion()
{
    delete[] this->parts;
}

