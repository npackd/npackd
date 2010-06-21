#include <qabstractitemview.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "repository.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    fillList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillList()
{
    QTableWidget* t = this->ui->tableWidget;

    Repository* r = Repository::getDefault();

    t->setColumnCount(1);
    t->setRowCount(r->packageVersions.count());
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion pv = r->packageVersions.at(i);
        QTableWidgetItem *newItem = new QTableWidgetItem(pv.package);
        t->setItem(i, 0, newItem);
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_actionExit_triggered()
{
    
}
