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

    /**
     * Initialization with a.b
     *
     * @param a major version number
     * @param b minor version number
     */
    Version(int a, int b);

    /**
     * @param v "1.2.3"
     */
    Version(const QString& v);

    Version(const Version& v);

    Version& operator =(const Version& v);

    bool operator !=(const Version& v);

    ~Version();

    /**
     * Changes the version
     *
     * @param version "1.2.3"
     * @return true if it was a valid version. The internal value is not changed
     *     if a not-valid version was supplied
     */
    bool setVersion(const QString& version);

    /**
     * Changes the version.
     *
     * @param a first version number
     * @param b second (minor) version number part
     */
    void setVersion(int a, int b);

    /**
     * Changes the version.
     *
     * @param a first version number
     * @param b second version number part
     * @param c third (minor) version number part
     */
    void setVersion(int a, int b, int c);

    /**
     * Changes the version.
     *
     * @param a first version number
     * @param b second version number part
     * @param c third version number part
     * @param d 4th (minor) version number part
     */
    void setVersion(int a, int b, int c, int d);

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

    /**
     * @return number of parts in this version number. Returns 1 for the
     *     version "0"
     */
    int getNParts() const;
};

#endif // VERSION_H
