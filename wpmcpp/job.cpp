#include <math.h>

#include "qdebug.h"
#include "qmutex.h"

#include "job.h"


Job::Job()
{
    this->progress = 0;
    this->nsteps = 1;
    this->parentJob = 0;
    this->cancelRequested = false;
    this->cancellable = false;
    this->completed = false;
}

void Job::complete()
{
    if (!this->completed) {
        this->completed = true;
        emit changed();
    }
}

void Job::setCancellable(bool v)
{
    this->cancellable = v;
    emit changed();
}

bool Job::isCancellable()
{
    return this->cancellable;
}

bool Job::isCancelled()
{
    return this->cancelRequested;
}

void Job::cancel()
{
    qDebug() << "Job::cancel";
    if (this->cancellable && !this->cancelRequested) {
        this->cancelRequested = true;
        qDebug() << "Job::cancel.2";
        if (parentJob)
            parentJob->cancel();

        emit changed();
        qDebug() << "Job::cancel.3";
    }
}

void Job::parentJobChanged()
{
    // qDebug() << "Job::parentJobChanged";
    Job* job = parentJob;
    if (job->isCancelled() && !this->isCancelled())
        this->cancel();
}

Job* Job::newSubJob(int nsteps)
{
    Job* r = new Job();
    r->parentJob = this;
    r->subJobSteps = nsteps;
    r->subJobStart = this->getProgress();
    r->parentHintStart = hint;

    bool success = connect(this, SIGNAL(changed()),
                           r, SLOT(parentJobChanged()),
                           Qt::DirectConnection);
    qDebug() << "Job::newSubJob " << success;
    return r;
}

bool Job::isCompleted()
{
    return this->completed;
}

void Job::setProgress(int progress)
{
    this->progress = progress;
    if (this->progress >= this->nsteps)
        this->completed = true;

    emit changed();

    updateParentProgress();
    updateParentHint();
}

void Job::updateParentProgress()
{
    if (parentJob) {
        double d = this->subJobStart +
                ((double) this->getProgress()) /
                this->getAmountOfWork() * this->subJobSteps;
        // Debug: qDebug() << "updateParentProgress " << this->subJobStart << " " <<
        //        lround(d) << " " << this->getProgress();
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

    emit changed();

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

    emit changed();

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

    emit changed();

    if (parentJob && parentJob->getErrorMessage().isEmpty())
        parentJob->setErrorMessage(errorMessage);
}
