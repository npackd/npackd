#include "messageframe.h"
#include "ui_messageframe.h"

#include <QTimer>

#include "mainwindow.h"

MessageFrame::MessageFrame(QWidget *parent, const QString& msg,
        const QString& details, int seconds) :
    QFrame(parent),
    ui(new Ui::MessageFrame)
{
    ui->setupUi(this);

    this->ui->pushButtonDismiss->setText(QString("Dismiss (%1)").arg(seconds));
    this->seconds = seconds;
    this->ui->label->setText(msg);
    this->details = msg;
    this->ui->pushButtonDetails->setEnabled(!details.isEmpty());

    if (seconds > 0) {
        QTimer* t = new QTimer(this);
        connect(t, SIGNAL(timeout()), this, SLOT(timerTimeout()));
        t->start(1000);
    }
}


MessageFrame::~MessageFrame()
{
    delete ui;
}

void MessageFrame::timerTimeout()
{
    this->seconds--;
    this->ui->pushButtonDismiss->setText(QString("Dismiss (%1)").arg(seconds));
    if (this->seconds <= 0) {
        this->deleteLater();
    }
}

void MessageFrame::on_pushButtonDetails_clicked()
{
    QString title = this->ui->label->text();
    if (title.length() > 20)
        title = title.left(20) + "...";
    MainWindow::getInstance()->addTextTab(title,
            this->details);
    this->deleteLater();
}

void MessageFrame::on_pushButtonDismiss_clicked()
{
    this->deleteLater();
}
