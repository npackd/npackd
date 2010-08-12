#ifndef JOB_H
#define JOB_H

#include <qstring.h>
#include <qobject.h>
#include "qmetatype.h"
#include "qmutex.h"

/**
 * A long-running task.
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
    bool cancelRequested;

    /** if true, this job can be cancelled */
    bool cancellable;

    bool completed;

    void updateParentHint();
    void updateParentProgress();
public slots:
    void parentJobChanged();
public:
    Job();

    /**
     * @return true if this job was completed: with or without an error. If a
     *     user cancells this job and it is not yet completed it means that the
     *     cancelling is in progress.
     */
    bool isCompleted();

    /**
     * This must be called in order to complete the job. done(0) completes
     * the job automatically.
     */
    void complete();

    /**
     * @param true if this task can be cancelled
     */
    void setCancellable(bool v);

    /**
     * @return true if this task can be cancelled. This value can change during
     *     a job lifecycle
     */
    bool isCancellable();

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
     * @param errorMessage new error message
     */
    void setErrorMessage(const QString &errorMessage);
signals:
    /**
     * This signal will be fired each time something in this object
     * changes (progress, hint etc.).
     */
    void changed();
};

#endif // JOB_H
