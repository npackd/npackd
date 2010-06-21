#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <qlist.h>
#include "packageversion.h"

class Repository
{
private:
    static Repository* def;

    QList<PackageVersion> pvs;
public:
    Repository();

    static Repository* getDefault();
};

#endif // REPOSITORY_H
