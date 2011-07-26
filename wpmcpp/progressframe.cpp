#include "progressframe.h"
#include "ui_progressframe.h"

ProgressFrame::ProgressFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ProgressFrame)
{
    ui->setupUi(this);
}

ProgressFrame::~ProgressFrame()
{
    delete ui;
}
