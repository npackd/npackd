#ifndef JOB_H
#define JOB_H

#include <qstring.h>
#include <qobject.h>
#include "qmetatype.h"
#include "qmutex.h"
#include "qqueue.h"
#include "QTime"

class Job;

/**
 * This class is used to exchange data about a Job between threads.
 */
class JobState: public QObject {
    Q_OBJECT
public:
    /** job progress: 0..1 */
    double progress;

    /** job hint */
    QString hint;

    /** job error message or "" */
    QString errorMessage;

    /** true, if the job was cancelled by the user */
    bool cancelRequested;

    /** the job was completed (with or without an error) */
    bool completed;

    /** the job */
    Job* job;

    /** time of the start or 0 */
    time_t started;

    JobState();
    JobState(const JobState& s);
    void operator=(const JobState& s);

    /**
     * @return time remaining time necessary to complete this task
     */
    time_t remainingTime();
};

Q_DECLARE_METATYPE(JobState)

/**
 * A long-running task.
 *
 * A task is typically defined as a function with the following signature:
 *     void longRunning(Job* job)
 *
 * The implementation follows this pattern:
 * {
 *     for (int i = 0; i < 100; i++) {
 *        if (job->isCancelled())
 *            break;
 *        job->setHint(QString("Processing step %1").arg(i));
 *        .... do some work here
 *        job->setProgress(((double) i) / 100);
 *     }
 *     job->completed();
 * }
 *
 * The function is called like this:
 * Job* job = new Job(); // or job = mainJob->createSubJob();
 * longRunning(job);
 * if (job->isCancelled()) {
 *     ....
 * } else if (!job->getErrorMessage().isEmpty()) {
 *    ....
 * } else {
 *     ....
 * }
 */
class Job: public QObject
{
    Q_OBJECT
private:
    /** progress 0...1 */
    double progress;

    /** description of the current state */
    QString hint;

    QString errorMessage;

    Job* parentJob;

    QString parentHintStart;
    double subJobSteps;
    double subJobStart;

    /** true if the user presses the "cancel" button. */
    volatile bool cancelRequested;

    bool completed;

    void updateParentHint();
    void updateParentProgress();
    void fireChange();
public slots:
    void parentJobChanged(const JobState& s);
public:
    static QList<Job*> jobs;

    /** time when this job was started or 0 */
    time_t started;

    Job();
    ~Job();

    /**
     * @return true if this job was completed: with or without an error. If a
     *     user cancells this job and it is not yet completed it means that the
     *     cancelling is in progress.
     */
    bool isCompleted();

    /**
     * This must be called in order to complete the job regardless of
     * setProgress.
     */
    void complete();

    /**
     * Request cancelling of this job.
     */
    void cancel();

    /**
     * @return true if this job was cancelled.
     */
    bool isCancelled();

    /**
     * @param nsteps number of steps in this job for the created sub-job
     * @return child job with parent=this
     */
    Job* newSubJob(double nsteps);

    /**
     * @return progress of this job (0...1)
     */
    double getProgress() const;

    /**
     * @return current hint
     */
    QString getHint() const;

    /**
     * @param hint new hint
     */
    void setHint(const QString& hint);

    /**
     * Sets the progress. The value is only informational. You have to
     * call complete() at the end anyway.
     *
     * @param progress new progress (0...1)
     */
    void setProgress(double progress);

    /**
     * @return error message. If the error message is not empty, the
     *     job ended with an error.
     */
    QString getErrorMessage() const;

    /**
     * Sets an error message for this job. The error message does *not*
     * automatically propagate to the parent job.
     *
     * @param errorMessage new error message
     */
    void setErrorMessage(const QString &errorMessage);
signals:
    /**
     * This signal will be fired each time something in this object
     * changes (progress, hint etc.).
     */
    void changed(const JobState& s);
};

#endif // JOB_H
