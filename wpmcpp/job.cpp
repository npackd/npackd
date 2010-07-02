#include <math.h>

#include "qdebug.h"

#include "job.h"


Job::Job()
{
    this->progress = 0;
    this->nsteps = 1;
    this->parentJob = 0;
}

Job* Job::newSubJob(int nsteps)
{
    Job* r = new Job();
    r->parentJob = this;
    r->subJobSteps = nsteps;
    r->subJobStart = this->getProgress();
    r->parentHintStart = hint;
    return r;
}

void Job::setProgress(int progress)
{
    this->progress = progress;

    emit changed(this);

    updateParentProgress();
    updateParentHint();
}

void Job::updateParentProgress()
{
    if (parentJob) {
        double d = this->subJobStart +
                ((double) this->getProgress()) /
                this->getAmountOfWork() * this->subJobSteps;
        qDebug() << "updateParentProgress " << this->subJobStart << " " <<
                lround(d) << " " << this->getProgress();
        parentJob->setProgress(lround(d));
    }
}

void Job::done(int n)
{
    if (n < 0)
        setProgress(nsteps);
    else
        setProgress(getProgress() + n);
}

int Job::getAmountOfWork()
{
    return nsteps;
}

void Job::setAmountOfWork(int n)
{
    this->nsteps = n;

    emit changed(this);

    updateParentProgress();
    updateParentHint();
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

    updateParentHint();
}

void Job::updateParentHint()
{
    if (parentJob) {
        if (getProgress() >= getAmountOfWork())
            parentJob->setHint(parentHintStart);
        else
            parentJob->setHint(parentHintStart + " / " + this->hint);
    }
}

QString Job::getErrorMessage() const
{
    return this->errorMessage;
}

void Job::setErrorMessage(const QString &errorMessage)
{
    this->errorMessage = errorMessage;

    emit changed(this);
}
