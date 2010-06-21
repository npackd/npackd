#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"

class PackageVersion
{
private:
    int* parts;
    int nparts;
public:
    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    PackageVersion(const QString& package);
    virtual ~PackageVersion();
};

#endif // PACKAGEVERSION_H
