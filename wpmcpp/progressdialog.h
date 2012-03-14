#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <time.h>

#include <QDialog>

#include "job.h"

namespace Ui {
    class ProgressDialog;
}

class ProgressDialog : public QDialog {
    Q_OBJECT
protected:
    void changeEvent(QEvent *e);
private:
    Ui::ProgressDialog *ui;
    Job* job;
    time_t started, modified;
public:
    /**
     * @param parent parent widget
     * @param job a job reference (not freed here)
     * @param title dialog title
     */
    ProgressDialog(QWidget *parent, Job* job, const QString& title);

    ~ProgressDialog();
public Q_SLOTS:
    void reject();
private slots:
    void on_pushButtonCancel_clicked();
    void jobChanged(const JobState& s);
};

#endif // PROGRESSDIALOG_H
