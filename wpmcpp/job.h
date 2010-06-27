#ifndef JOB_H
#define JOB_H

#include <qstring.h>
#include <qobject.h>
#include "qmetatype.h"

/**
 * A long-running task.
 */
class Job: public QObject
{
    Q_OBJECT
private:
    /** progress */
    int progress;

    /** description of the current state */
    QString hint;

    QString errorMessage;
public:
    /** number of steps. Initialized with 1. */
    int nsteps;

    Job();

    /**
     * Changes the progress.
     *
     * @param n number of steps done (additionally)
     */
    void done(int n);

    /**
     * @return progress of this job
     */
    int getProgress() const;

    /**
     * @return current hint
     */
    QString getHint() const;

    /**
     * @param hint new hint
     */
    void setHint(const QString& hint);

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
     *
     * @param job *Job = this
     */
    void changed(void* job);
};

#endif // JOB_H
