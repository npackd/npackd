#include "qstringlist.h"

#include "version.h"

Version::Version()
{
    this->parts = new int[2];
    this->parts[0] = 1;
    this->parts[1] = 0;
    this->nparts = 2;
}

Version::Version(const Version &v)
{
    this->parts = new int[v.nparts];
    this->nparts = v.nparts;
    memcpy(parts, v.parts, sizeof(parts[0]) * nparts);
}

Version::~Version()
{
    delete[] this->parts;
}

void Version::setVersion(int a, int b)
{
    delete[] this->parts;
    this->parts = new int[2];
    this->parts[0] = a;
    this->parts[1] = b;
    this->nparts = 2;
}

void Version::setVersion(QString& v)
{
    delete[] this->parts;
    QStringList sl = v.split(".", QString::KeepEmptyParts);

    this->parts = new int[sl.count()];
    this->nparts = sl.count();
    for (int i = 0; i < sl.count(); i++) {
        this->parts[i] = sl.at(i).toInt();
    }
}

QString Version::getVersionString()
{
    QString r;
    for (int i = 0; i < this->nparts; i++) {
        if (i != 0)
            r.append(".");
        r.append(QString("%1").arg(this->parts[i]));
    }
    return r;
}

int Version::compare(const Version &other) const
{
    int nmax = nparts;
    if (other.nparts > nmax)
        nmax = other.nparts;

    int r = 0;
    for (int i = 0; i < nmax; i++) {
        int a;
        if (i < nparts)
            a = this->parts[i];
        else
            a = 0;
        int b;
        if (i < nparts)
            b = other.parts[i];
        else
            b = 0;

        if (a < b) {
            r = -1;
            break;
        } else if (a > b) {
            r = 1;
            break;
        }
    }
    return r;
}
