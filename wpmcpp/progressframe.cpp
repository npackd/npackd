#include <time.h>
#include <math.h>
#include <QTime>

#include "progressframe.h"
#include "ui_progressframe.h"

ProgressFrame::ProgressFrame(QWidget *parent, Job* job, const QString& title) :
    QFrame(parent),
    ui(new Ui::ProgressFrame)
{
    ui->setupUi(this);
    this->job = job;
    this->started = 0;

    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)),
            Qt::QueuedConnection);
}

ProgressFrame::~ProgressFrame()
{
    delete ui;
}

void ProgressFrame::jobChanged(const JobState& s)
{
    // Debug: qDebug() << "ProgressDialog::jobChanged";

    if (s.completed) {
        // TODO: this->done(s.cancelRequested ? Rejected : Accepted);
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

void ProgressFrame::on_pushButtonCancel_clicked()
{
    ui->pushButtonCancel->setEnabled(false);
    job->cancel();
}
