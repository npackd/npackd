#include "repository.h"

Repository* Repository::def = 0;

Repository::Repository()
{
    // TODO: remove later
    PackageVersion a(QString("net.sourceforge.NotepadPlusPlus"));

    this->packageVersions.append(a);
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}
