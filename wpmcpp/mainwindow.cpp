#include <qabstractitemview.h>
#include <qmessagebox.h>
#include <qvariant.h>
#include <qprogressdialog.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <windows.h>
#include <qtimer.h>
#include <qdebug.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "repository.h"
#include "job.h"

class InstallThread: public QThread
{
    PackageVersion* pv;
    bool install;
    Job* job;
public:
    InstallThread(PackageVersion* pv, bool install, Job* job);

    void run();
};

InstallThread::InstallThread(PackageVersion *pv, bool install, Job* job)
{
    this->pv = pv;
    this->install = install;
    this->job = job;
}

void InstallThread::run()
{
    job->nsteps = 1;
    qDebug() << "InstallThread::run.1";
    if (this->install)
        pv->install();
    else
        pv->uninstall();
    qDebug() << "InstallThread::run.2";
    job->done(1);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    this->on_tableWidget_itemSelectionChanged();
    this->ui->tableWidget->setColumnWidth(0, 400);
    fillList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::waitFor(Job* job)
{
    pd = new QProgressDialog("Please wait...", "Cancel", 0, 100, this);
    pd->setCancelButton(0);
    pd->setWindowModality(Qt::WindowModal);

    connect(job, SIGNAL(changed(void*)), this, SLOT(jobChanged(void*)));

    // TODO: connect(http, SIGNAL(dataReadProgress(int, int)),
    //        this, SLOT(updateDataReadProgress(int, int)));
    pd->show(); // TODO: pd - memory leak?
    //delete pd;
}

void MainWindow::jobChanged(void* job_)
{
    Job* job = (Job*) job_;
    if (job->getProgress() >= job->nsteps) {
        delete pd;
    } else {
        pd->setLabelText(job->hint);
    }
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
    PackageVersion* pv = getSelectedPackageVersion();
    this->ui->actionInstall->setEnabled(pv && !pv->installed());
    this->ui->actionUninstall->setEnabled(pv && pv->installed());
}

void MainWindow::on_actionInstall_activated()
{
    // TODO: getSelectedPackageVersion()->install();
    PackageVersion* pv = getSelectedPackageVersion();
    Job* job = new Job();
    InstallThread* it = new InstallThread(pv, true, job);
    it->start(); // TODO: low priority
    // TODO: memory leak: it

    waitFor(job);
    // TODO: delete job;
}
