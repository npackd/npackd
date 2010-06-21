#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"

class PackageVersion
{
private:
    int* parts;
    int nparts;
    QString package;
public:
    PackageVersion();
    virtual ~PackageVersion();
};

#endif // PACKAGEVERSION_H
