#include "messageframe.h"
#include "ui_messageframe.h"

#include <QTimer>

#include "mainwindow.h"

MessageFrame::MessageFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::MessageFrame)
{
    ui->setupUi(this);

    this->autoHide = true;

    QTimer* t = new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    t->setSingleShot(true);
    t->start(10000);

    this->setDetails("");
}


MessageFrame::~MessageFrame()
{
    delete ui;
}

void MessageFrame::setMessage(const QString& msg)
{
    this->ui->label->setText(msg);
}

void MessageFrame::setAutoHide(bool b)
{
    this->autoHide = b;
}

void MessageFrame::setDetails(const QString& msg)
{
    this->details = msg;
    this->ui->pushButtonDetails->setEnabled(!details.isEmpty());
}

void MessageFrame::timerTimeout()
{
    if (autoHide) {
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
