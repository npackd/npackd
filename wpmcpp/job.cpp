#include "job.h"

Job::Job()
{
    this->nsteps = 1;
}

void Job::done(int n)
{
    progress += n;

    emit changed(this);
}

int Job::getProgress() const
{
    return this->progress;
}
