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
    time_t started;
public:
    /**
     * @param parent parent widget
     * @param job a job reference (not freed here)
     */
    ProgressDialog(QWidget *parent, Job* job);

    ~ProgressDialog();
private slots:
    void on_pushButtonCancel_clicked();
    void jobChanged();
};

#endif // PROGRESSDIALOG_H
