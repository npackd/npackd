#include "dependency.h"

Dependency::Dependency()
{
    this->minIncluded = true;
    this->min.setVersion(0, 0);
    this->maxIncluded = false;
    this->max.setVersion(1, 0);
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

