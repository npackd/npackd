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

    /** web page associated with this piece of software. May be empty. */
    QString url;

    /** description. May contain multiple lines and paragraphs. */
    QString description;

    Package(QString& name, QString& title);
};

#endif // PACKAGE_H
