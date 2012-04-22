#include <math.h>
#include <time.h>

#include "qdebug.h"
#include "qmutex.h"

#include "job.h"

JobState::JobState()
{
    this->cancelRequested = false;
    this->completed = false;
    this->errorMessage = "";
    this->hint = "";
    this->progress = 0;
    this->job = 0;
    this->started = 0;
}

JobState::JobState(const JobState& s)
{
    this->cancelRequested = s.cancelRequested;
    this->completed = s.completed;
    this->errorMessage = s.errorMessage;
    this->hint = s.hint;
    this->progress = s.progress;
    this->job = s.job;
    this->started = s.started;
}

void JobState::operator=(const JobState& s)
{
    this->cancelRequested = s.cancelRequested;
    this->completed = s.completed;
    this->errorMessage = s.errorMessage;
    this->hint = s.hint;
    this->progress = s.progress;
    this->job = s.job;
    this->started = s.started;
}


time_t JobState::remainingTime()
{
    time_t result;
    if (started != 0) {
        time_t now;
        time(&now);

        time_t diff = difftime(now, started);

        diff = difftime(now, started);
        diff = lround(diff * (1 / this->progress - 1));
        result = diff;
    } else {
        result = 0;
    }
    return result;
}

QList<Job*> Job::jobs;

Job::Job()
{
    this->progress = 0.0;
    this->parentJob = 0;
    this->cancelRequested = false;
    this->completed = false;
    this->started = 0;
}

Job::~Job()
{
    if (parentJob) {
        parentJob->setHint(parentHintStart);
    }
}

void Job::complete()
{
    if (!this->completed) {
        this->completed = true;
        fireChange();
    }
}

bool Job::isCancelled()
{
    return this->cancelRequested;
}

void Job::cancel()
{
    // qDebug() << "Job::cancel";
    if (!this->cancelRequested) {
        this->cancelRequested = true;
        // qDebug() << "Job::cancel.2";
        if (parentJob)
            parentJob->cancel();

        fireChange();
        // qDebug() << "Job::cancel.3";
    }
}

void Job::parentJobChanged(const JobState& s)
{
    // qDebug() << "Job::parentJobChanged";
    if (s.cancelRequested && !this->isCancelled())
        this->cancel();
}

Job* Job::newSubJob(double part)
{
    Job* r = new Job();
    r->parentJob = this;
    r->subJobSteps = part;
    r->subJobStart = this->getProgress();
    r->parentHintStart = hint;

    // bool success =
    connect(this, SIGNAL(changed(const JobState&)),
            r, SLOT(parentJobChanged(const JobState&)),
            Qt::DirectConnection);
    // qDebug() << "Job::newSubJob " << success;
    return r;
}

bool Job::isCompleted()
{
    return this->completed;
}

void Job::fireChange()
{
    if (this->started == 0)
        time(&this->started);

    JobState state;
    state.job = this;
    state.cancelRequested = this->cancelRequested;
    state.completed = this->completed;
    state.errorMessage = this->errorMessage;
    state.hint = this->hint;
    state.progress = this->progress;
    state.started = this->started;
    emit changed(state);
}

void Job::setProgress(double progress)
{
    this->progress = progress;

    fireChange();

    updateParentProgress();
    updateParentHint();
}

void Job::updateParentProgress()
{
    if (parentJob) {
        double d = this->subJobStart +
                this->getProgress() * this->subJobSteps;
        parentJob->setProgress(d);
    }
}

double Job::getProgress() const
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

    fireChange();

    updateParentHint();
}

void Job::updateParentHint()
{
    if (parentJob) {
        if ((isCompleted() && getErrorMessage().isEmpty())
                || this->hint.isEmpty())
            parentJob->setHint(parentHintStart);
        else
            parentJob->setHint(parentHintStart + " / " + this->hint);
    }
}

QString Job::getErrorMessage() const
{
    return this->errorMessage;
}

bool Job::shouldProceed(const QString& hint)
{
    bool b = !this->isCancelled() && this->getErrorMessage().isEmpty();
    if (b)
        setHint(hint);
    return b;
}

void Job::setErrorMessage(const QString &errorMessage)
{
    this->errorMessage = errorMessage;

    fireChange();
}
