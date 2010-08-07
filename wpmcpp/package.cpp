#include "package.h"

Package::Package(QString& name, QString& title)
{
    this->name = name;
    this->title = title;
}

bool Package::isValidName(QString& name)
{
    bool r = false;
    if (!name.isEmpty() && !name.contains(" ") && !name.contains("..")) {
        r = true;
    }
    return r;
}
