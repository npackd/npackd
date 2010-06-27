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
    if (pv) {
        QString errMsg;
        if (this->install)
            pv->install(&errMsg);
        else
            pv->uninstall();
    } else {
        Repository::getDefault()->load();
    }
    qDebug() << "InstallThread::run.2";
    job->done(1);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Windows Package Manager");

    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    this->ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    this->ui->tabWidget->setTabText(0, "Packages");
    this->ui->tabWidget->setTabText(1, "Settings");

    QUrl* url = Repository::getRepositoryURL();
    if (url) {
        this->ui->lineEditRepository->setText(url->toString());
        delete url;
    } else {
        QMessageBox::critical(this,
                "Error", "The repository URL is not valid. Please change it on the settings tab.",
                QMessageBox::Ok);
    }

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
    // TODO: closing the dialog crashes the app
    pd = new QProgressDialog("Please wait...", "Cancel", 0, 100, this);
    pd->setCancelButton(0);
    pd->setModal(true);

    connect(job, SIGNAL(changed(void*)), this, SLOT(jobChanged(void*)));

    pd->exec();
    delete pd;
}

void MainWindow::jobChanged(void* job_)
{
    Job* job = (Job*) job_;
    if (job->getProgress() >= job->nsteps) {
        pd->done(0);
    } else {
        pd->setLabelText(job->getHint());
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
    qDebug() << "MainWindow::fillList";
    QTableWidget* t = this->ui->tableWidget;

    t->clearContents();

    Repository* r = Repository::getDefault();

    t->setColumnCount(3);

    QTableWidgetItem *newItem = new QTableWidgetItem("Package");
    t->setHorizontalHeaderItem(0, newItem);

    newItem = new QTableWidgetItem("Version");
    t->setHorizontalHeaderItem(1, newItem);

    newItem = new QTableWidgetItem("Status");
    t->setHorizontalHeaderItem(2, newItem);

    t->setRowCount(r->packageVersions.count());
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);

        newItem = new QTableWidgetItem(pv->package);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 0, newItem);

        newItem = new QTableWidgetItem(pv->getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 1, newItem);

        newItem = new QTableWidgetItem(pv->installed() ? "installed": "");
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 2, newItem);
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
    this->close();
}

void MainWindow::on_actionUninstall_activated()
{
    PackageVersion* pv = getSelectedPackageVersion();
    Job* job = new Job();
    InstallThread* it = new InstallThread(pv, false, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job);
    delete it;

    fillList();
    delete job;
}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    PackageVersion* pv = getSelectedPackageVersion();
    this->ui->actionInstall->setEnabled(pv && !pv->installed());
    this->ui->actionUninstall->setEnabled(pv && pv->installed());
}

void MainWindow::loadRepository()
{
    Job* job = new Job();
    InstallThread* it = new InstallThread(0, true, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job);
    delete it;

    fillList();
    delete job;
}

void MainWindow::on_actionInstall_activated()
{
    PackageVersion* pv = getSelectedPackageVersion();
    Job* job = new Job();
    InstallThread* it = new InstallThread(pv, true, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job);
    delete it;

    fillList();
    delete job;
}


void MainWindow::on_pushButtonSaveSettings_clicked()
{
    QUrl url(this->ui->lineEditRepository->text().trimmed());
    if (url.isValid()) {
        Repository::setRepositoryURL(url);
        loadRepository();
    } else {
        QMessageBox::critical(this,
                "Error", "The URL is not valid", QMessageBox::Ok);
    }
}
