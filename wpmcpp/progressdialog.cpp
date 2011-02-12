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

    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)),
            Qt::QueuedConnection);
    EnableMenuItem(GetSystemMenu(this->winId(), false), SC_CLOSE, MF_GRAYED);
}

void ProgressDialog::reject()
{

}

void ProgressDialog::jobChanged(const JobState& s)
{
    // Debug: qDebug() << "ProgressDialog::jobChanged";

    if (s.completed) {
        this->done(s.cancelRequested ? Rejected : Accepted);
    } else {
        time_t now;
        time(&now);
        if (now != this->modified) {
            ui->labelStep->setText(s.hint);
            this->modified = now;
        }
        if (started != 0) {
            time_t diff = difftime(now, started);

            int sec = diff % 60;
            diff /= 60;
            int min = diff % 60;
            diff /= 60;
            int h = diff;

            QTime e(h, min, sec);
            ui->labelElapsed->setText(e.toString());

            diff = difftime(now, started);
            diff = lround(diff * (1 / s.progress - 1));
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
        ui->progressBar->setValue(lround(s.progress * 10000));
        ui->pushButtonCancel->setEnabled(!s.cancelRequested);
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

