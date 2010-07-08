#ifndef PACKAGE_H
#define PACKAGE_H

#include "qstring.h"

/**
 * A package declaration.
 */
class Package
{
public:
    /** name of the package like "org.buggysoft.BuggyEditor" */
    QString name;

    /** short program title (1 line) like "Gimp" */
    QString title;

    Package(QString& name, QString& title);
};

#endif // PACKAGE_H
