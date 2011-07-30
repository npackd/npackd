#include "messageframe.h"
#include "ui_messageframe.h"

#include <QTimer>

MessageFrame::MessageFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::MessageFrame)
{
    ui->setupUi(this);

    QTimer* t = new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    t->setSingleShot(true);
    t->start(10000);
}


MessageFrame::~MessageFrame()
{
    delete ui;
}

void MessageFrame::setMessage(const QString& msg)
{
    this->ui->label->setText(msg);
}

void MessageFrame::timerTimeout()
{
    this->deleteLater();
}

void MessageFrame::on_pushButton_clicked()
{
    this->deleteLater();
}
