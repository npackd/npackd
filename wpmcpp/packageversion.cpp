#include "packageversion.h"

PackageVersion::PackageVersion()
{
    this->parts = new int[1];
    this->parts[0] = 1;
    this->nparts = 1;
}

PackageVersion::~PackageVersion()
{
    delete[] this->parts;
}

