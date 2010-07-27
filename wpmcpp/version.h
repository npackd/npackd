#ifndef VERSION_H
#define VERSION_H

#include "qstring.h"

class Version
{
private:
    int* parts;
    int nparts;
public:
    /**
     * Initialization with 1.0
     */
    Version();

    Version(const Version& v);

    ~Version();

    /**
     * Changes the version
     *
     * @param version "1.2.3"
     */
    void setVersion(QString& version);

    /**
     * Changes the version.
     *
     * @param a first version number
     * @param b second (minor) version number part
     */
    void setVersion(int a, int b);

    /**
     * @return package version as a string (like "1.2.3")
     */
    QString getVersionString();

    /**
     * Compares this version with another one.
     *
     * @param other other version
     * @return <0, 0 or >0
     */
    int compare(const Version& other) const;
};

#endif // VERSION_H
