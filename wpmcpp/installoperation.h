#ifndef INSTALLOPERATION_H
#define INSTALLOPERATION_H

#include "qlist.h"

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

    /**
     * Simplifies a list of operations.
     *
     * @param ops a list of operations. The list will be modified and
     *     unnecessary operation removed and the objects destroyed.
     */
    static void simplify(QList<InstallOperation*> ops);
};

#endif // INSTALLOPERATION_H
