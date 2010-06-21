#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include "qxml.h"
#include "qstring.h"
#include "qmetatype.h"

class PackageVersion
{
private:
    int* parts;
    int nparts;
public:
    /** complete package name like net.sourceforge.NotepadPlusPlus */
    QString package;

    PackageVersion();
    PackageVersion(const QString& package);
    virtual ~PackageVersion();
};

Q_DECLARE_METATYPE(PackageVersion);

#endif // PACKAGEVERSION_H
