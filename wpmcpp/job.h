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
public:
    /** description of the current state */
    QString hint;

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
signals:
    // TODO: comments
    void changed(void* job);
};

#endif // JOB_H
