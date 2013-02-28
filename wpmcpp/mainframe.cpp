#include "mainframe.h"
#include "ui_mainframe.h"

#include <QDebug>
#include <QList>

#include "mainwindow.h"
#include "package.h"
#include "packageitemmodel.h"

MainFrame::MainFrame(QWidget *parent) :
    QFrame(parent), Selection(),
    ui(new Ui::MainFrame)
{
    ui->setupUi(this);

    QTableView* t = this->ui->tableWidget;
    t->setModel(new PackageItemModel(QList<Package*>()));
    t->setEditTriggers(QTableWidget::NoEditTriggers);
    t->setSortingEnabled(false);

    t->verticalHeader()->setDefaultSectionSize(36);
    t->setColumnWidth(0, 40);
    t->setColumnWidth(1, 150);
    t->setColumnWidth(2, 300);
    t->setColumnWidth(3, 100);
    t->setColumnWidth(4, 100);
    t->setColumnWidth(5, 100);
    t->setIconSize(QSize(32, 32));

    connect(t->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this,
            SLOT(tableWidget_selectionChanged()));
}

MainFrame::~MainFrame()
{
    delete ui;
}

QTableView * MainFrame::getTableWidget() const
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
    QAbstractItemModel* m = this->ui->tableWidget->model();
    QItemSelectionModel* sm = this->ui->tableWidget->selectionModel();
    QModelIndexList sel = sm->selectedRows();
    if (sel.count() > 0) {
        QModelIndex index = m->index(sel.at(0).row(), 1);
        const QVariant v = index.data(Qt::UserRole);
        Package* p = (Package*) v.value<void*>();
        return p;
    }
    return 0;
}

QList<Package*> MainFrame::getSelectedPackagesInTable() const
{
    QList<Package*> result;
    QAbstractItemModel* m = this->ui->tableWidget->model();
    QItemSelectionModel* sm = this->ui->tableWidget->selectionModel();
    QModelIndexList sel = sm->selectedRows();
    for (int i = 0; i < sel.count(); i++) {
        QModelIndex index = m->index(sel.at(i).row(), 1);
        const QVariant v = index.data(Qt::UserRole);
        Package* p = (Package*) v.value<void*>();
        result.append(p);
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

void MainFrame::tableWidget_selectionChanged()
{
    MainWindow::getInstance()->updateActions();
}
