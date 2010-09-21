#include <time.h>
#include <math.h>
#include <windows.h>

#include "qdebug.h"
#include "QTime"
#include "qapplication.h"
#include "qdialog.h"
#include <QCloseEvent>

#include "progressdialog.h"
#include "ui_progressdialog.h"
#include "job.h"

ProgressDialog::ProgressDialog(QWidget *parent, Job* job, const QString& title) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(title);
    this->job = job;
    this->started = 0;

    connect(job, SIGNAL(changed()), this, SLOT(jobChanged()),
            Qt::QueuedConnection);
    EnableMenuItem(GetSystemMenu(this->winId(), false), SC_CLOSE, MF_GRAYED);
}

void ProgressDialog::reject()
{

}

void ProgressDialog::jobChanged()
{
    // Debug: qDebug() << "ProgressDialog::jobChanged";

    Job* job = (Job*) QObject::sender();
    if (job->isCompleted()) {
        this->done(job->isCancelled() ? Rejected : Accepted);
    } else {
        ui->labelStep->setText(job->getHint());
        if (started != 0) {
            time_t now;
            time(&now);
            time_t diff = difftime(now, started);

            int sec = diff % 60;
            diff /= 60;
            int min = diff % 60;
            diff /= 60;
            int h = diff;

            QTime e(h, min, sec);
            ui->labelElapsed->setText(e.toString());

            diff = difftime(now, started);
            diff = lround(diff * (1 / job->getProgress() - 1));
            sec = diff % 60;
            diff /= 60;
            min = diff % 60;
            diff /= 60;
            h = diff;

            QTime r(h, min, sec);
            ui->label_remainingTime->setText(r.toString());
        } else {
            time(&this->started);
            ui->labelElapsed->setText("-");
        }
        ui->progressBar->setMaximum(10000);
        ui->progressBar->setValue(lround(job->getProgress() * 10000));
        ui->pushButtonCancel->setEnabled(job->isCancellable() &&
                !job->isCancelled());

    }
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ProgressDialog::on_pushButtonCancel_clicked()
{
    ui->pushButtonCancel->setEnabled(false);
    job->cancel();
}

