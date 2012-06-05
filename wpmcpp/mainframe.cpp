#include "mainframe.h"
#include "ui_mainframe.h"

#include "mainwindow.h"


MainFrame::MainFrame(QWidget *parent) :
    QFrame(parent), Selection(),
    ui(new Ui::MainFrame)
{
    ui->setupUi(this);
    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);

    this->ui->tableWidget->setColumnCount(6);
    this->ui->tableWidget->setColumnWidth(0, 40);
    this->ui->tableWidget->setColumnWidth(1, 150);
    this->ui->tableWidget->setColumnWidth(2, 300);
    this->ui->tableWidget->setColumnWidth(3, 100);
    this->ui->tableWidget->setColumnWidth(4, 100);
    this->ui->tableWidget->setColumnWidth(5, 100);
    this->ui->tableWidget->setIconSize(QSize(32, 32));
    this->ui->tableWidget->sortItems(1);
}

MainFrame::~MainFrame()
{
    delete ui;
}

QTableWidget * MainFrame::getTableWidget() const
{
    return this->ui->tableWidget;
}

QLineEdit* MainFrame::getFilterLineEdit() const
{
    return this->ui->lineEditText;
}

QComboBox* MainFrame::getStatusComboBox() const
{
    return this->ui->comboBoxStatus;
}

QList<void*> MainFrame::getSelected(const QString& type) const
{
    QList<void*> res;
    if (type == "Package") {
        QList<Package*> ps = this->getSelectedPackagesInTable();
        for (int i = 0; i < ps.count(); i++) {
            res.append(ps.at(i));
        }
    }
    return res;
}

Package* MainFrame::getSelectedPackageInTable()
{
    QList<QTableWidgetItem*> sel = this->ui->tableWidget->selectedItems();
    if (sel.count() > 0) {
        const QVariant v = sel.at(1)->data(Qt::UserRole);
        Package* pv = (Package*) v.value<void*>();
        return pv;
    }
    return 0;
}

QList<Package*> MainFrame::getSelectedPackagesInTable() const
{
    QList<Package*> result;
    QList<QTableWidgetItem*> sel = this->ui->tableWidget->selectedItems();
    for (int i = 0; i < sel.count(); i++) {
        QTableWidgetItem* item = sel.at(i);
        if (item->column() == 1) {
            const QVariant v = item->data(Qt::UserRole);
            Package* p = (Package*) v.value<void*>();
            result.append(p);
        }
    }
    return result;
}

void MainFrame::on_tableWidget_doubleClicked(QModelIndex index)
{
    MainWindow* mw = MainWindow::getInstance();
    QAction *a = mw->findChild<QAction *>("actionShow_Details");
    if (a)
        a->trigger();
}

void MainFrame::on_lineEditText_textChanged(QString )
{
    MainWindow* mw = MainWindow::getInstance();
    if (!mw->reloadRepositoriesThreadRunning && !mw->hardDriveScanRunning)
        mw->fillList();
}

void MainFrame::on_comboBoxStatus_currentIndexChanged(int index)
{
    MainWindow* mw = MainWindow::getInstance();
    if (!mw->reloadRepositoriesThreadRunning && !mw->hardDriveScanRunning)
        mw->fillList();
}

void MainFrame::on_tableWidget_itemSelectionChanged()
{
    MainWindow::getInstance()->updateActions();
}


void MainFrame::on_tableWidget_itemDoubleClicked(QTableWidgetItem *item)
{

}
