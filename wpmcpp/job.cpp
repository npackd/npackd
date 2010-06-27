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

QString Job::getHint() const
{
    return this->hint;
}

void Job::setHint(const QString &hint)
{
    this->hint = hint;

    emit changed(this);
}
