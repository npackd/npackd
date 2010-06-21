#include <qabstractitemview.h>
#include <qmessagebox.h>
#include <qvariant.h>

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

PackageVersion* MainWindow::getSelectedPackageVersion()
{
    QList<QTableWidgetItem*> sel = this->ui->tableWidget->selectedItems();
    if (sel.count() > 0) {
        const QVariant v = sel.at(0)->data(Qt::UserRole);
        PackageVersion* pv = (PackageVersion *) v.value<void*>();
        return pv;
    }
    return 0;
}


void MainWindow::fillList()
{
    QTableWidget* t = this->ui->tableWidget;

    Repository* r = Repository::getDefault();

    t->setColumnCount(1);
    t->setRowCount(r->packageVersions.count());
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        QTableWidgetItem *newItem = new QTableWidgetItem(pv->package);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
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

void MainWindow::on_MainWindow_destroyed()
{

}

void MainWindow::on_actionUninstall_activated()
{
    QMessageBox::about(this, "Windows Package Manager", "AHA");
}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    this->ui->actionInstall->setEnabled(true);
    this->ui->actionUninstall->setEnabled(true);
}
