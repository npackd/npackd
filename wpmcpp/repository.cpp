#include "repository.h"

Repository* Repository::def = 0;

Repository::Repository()
{
}

Repository* Repository::getDefault()
{
    if (!def) {
        def = new Repository();
    }
    return def;
}
