#include "dependency.h"
#include "repository.h"
#include "packageversion.h"
#include "package.h"

Dependency::Dependency()
{
    this->minIncluded = true;
    this->min.setVersion(0, 0);
    this->maxIncluded = false;
    this->max.setVersion(1, 0);
}

QString Dependency::toString()
{
    QString res;

    Repository* r = Repository::getDefault();
    Package* p = r->findPackage(this->package);
    if (p)
        res.append(p->title);
    else
        res.append(package);

    res.append(" ");

    if (minIncluded)
        res.append('[');
    else
        res.append('(');

    res.append(this->min.getVersionString());

    res.append(", ");

    res.append(this->max.getVersionString());

    if (minIncluded)
        res.append(']');
    else
        res.append(')');

    return res;
}

bool Dependency::isInstalled()
{
    Repository* r = Repository::getDefault();
    bool res = false;
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        if (pv->package == this->package && pv->installed() &&
                this->test(pv->version)) {
            res = true;
            break;
        }
    }
    return res;
}

bool Dependency::test(const Version& v)
{
    int a = v.compare(this->min);
    int b = v.compare(this->max);

    bool low;
    if (minIncluded)
        low = a >= 0;
    else
        low = a > 0;

    bool high;
    if (maxIncluded)
        high = b <= 0;
    else
        high = b < 0;

    return low && high;
}

