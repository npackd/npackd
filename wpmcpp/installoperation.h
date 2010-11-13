#ifndef INSTALLOPERATION_H
#define INSTALLOPERATION_H

#include "packageversion.h"

/**
 * Installation operation.
 */
class InstallOperation
{
public:
    /** this will be changed. The object is not owned by this one. */
    PackageVersion* packageVersion;

    /** true = install, false = uninstall */
    bool install;

    InstallOperation();
};

#endif // INSTALLOPERATION_H
