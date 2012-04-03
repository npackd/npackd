#include "hrtimer.h"

#include <windows.h>

#include <QDebug>

HRTimer::HRTimer(int size)
{
    this->cur = 0;
    this->size = size;
    this->lastMeasurement = -1;

    this->durations = new LONGLONG[size];
    memset(this->durations, 0, sizeof(durations[0]) * size);

    LARGE_INTEGER liFrequency;
    QueryPerformanceFrequency(&liFrequency);
    frequency = liFrequency.QuadPart;
}

void HRTimer::time(int point)
{
    if (point != cur)
        qDebug() << "HRTimer: " << point << " != " << cur;

    LARGE_INTEGER v;
    QueryPerformanceCounter(&v);

    if (lastMeasurement >= 0) {
        this->durations[cur] += v.QuadPart - lastMeasurement;
    }
    lastMeasurement = v.QuadPart;
    cur++;
    if (cur == this->size)
        cur = 0;
}

void HRTimer::dump() const
{
    for (int i = 0; i < this->size; i++) {
        qDebug() << i << ": " <<
                (this->durations[i] * 1000 / this->frequency) << " ms";
    }
}

HRTimer::~HRTimer()
{
    delete[] this->durations;
}
