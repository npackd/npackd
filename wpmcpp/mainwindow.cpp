#include <qabstractitemview.h>
#include <qmessagebox.h>
#include <qvariant.h>
#include <qprogressdialog.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <windows.h>
#include <qtimer.h>
#include <qdebug.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include "qdesktopservices.h"

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
    CoInitialize(NULL);

    qDebug() << "InstallThread::run.1";
    QString errMsg;
    if (pv) {
        if (this->install) {
            pv->install(job);
        } else {
            pv->uninstall(job);
        }
    } else {
        Repository::getDefault()->load(job);
    }

    CoUninitialize();

    qDebug() << "InstallThread::run.2";
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Windows Package Manager");

    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);

    this->ui->tabWidget->setTabText(0, "Packages");
    this->ui->tabWidget->setTabText(1, "Settings");

    QUrl* url = Repository::getRepositoryURL();
    if (!url) {
        url = new QUrl(
                "http://windows-package-manager.googlecode.com/hg/repository/Rep.xml");
        Repository::setRepositoryURL(*url);
    }
    this->ui->lineEditRepository->setText(url->toString());
    delete url;

    this->on_tableWidget_itemSelectionChanged();
    this->ui->tableWidget->setColumnCount(4);
    this->ui->tableWidget->setColumnWidth(0, 150);
    this->ui->tableWidget->setColumnWidth(1, 300);
    this->ui->tableWidget->sortItems(0);

    this->pd = 0;

    QTimer *pTimer = new QTimer(this);
    pTimer->setSingleShot(true);
    connect(pTimer, SIGNAL(timeout()), this,
            SLOT(onShow()));

    pTimer->start(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::waitFor(Job* job)
{
    pd = new QProgressDialog("Please wait...", "Cancel", 0, 100, this);
    pd->setCancelButton(0);
    pd->setModal(true);

    connect(job, SIGNAL(changed(void*)), this, SLOT(jobChanged(void*)));

    qDebug() << "MainWindow::waitFor.1";

    pd->exec();
    delete pd;
    pd = 0;

    qDebug() << "MainWindow::waitFor.2";
    if (!job->getErrorMessage().isEmpty()) {
        qDebug() << "MainWindow::waitFor.3";
        QMessageBox::critical(this,
                "Error", job->getErrorMessage(),
                QMessageBox::Ok);
        qDebug() << "MainWindow::waitFor.4";
        return false;
    } else {
        return true;
    }
}

void MainWindow::onShow()
{
    loadRepository();
}

void MainWindow::jobChanged(void* job_)
{
    qDebug() << "MainWindow::jobChanged";

    Job* job = (Job*) job_;
    if (pd) {
        if (job->getProgress() >= job->getAmountOfWork() ||
                !job->getErrorMessage().isEmpty()) {
            pd->done(0);
        } else {
            pd->setLabelText(job->getHint());
        }
        pd->setMaximum(job->getAmountOfWork());
        pd->setValue(job->getProgress());
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
    t->setSortingEnabled(false);

    Repository* r = Repository::getDefault();

    t->setColumnCount(4);

    QTableWidgetItem *newItem = new QTableWidgetItem("Title");
    t->setHorizontalHeaderItem(0, newItem);

    newItem = new QTableWidgetItem("Description");
    t->setHorizontalHeaderItem(1, newItem);

    newItem = new QTableWidgetItem("Version");
    t->setHorizontalHeaderItem(2, newItem);

    newItem = new QTableWidgetItem("Status");
    t->setHorizontalHeaderItem(3, newItem);

    t->setRowCount(r->packageVersions.count());
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        Package* p = r->findPackage(pv->package);

        QString packageTitle;
        if (p)
            packageTitle = p->title;
        else
            packageTitle = pv->package;
        newItem = new QTableWidgetItem(packageTitle);
        newItem->setStatusTip(pv->download.toString() + " " + pv->package);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 0, newItem);

        QString desc;
        if (p)
            desc = p->description;
        if (desc.isEmpty())
            desc = pv->package;
        newItem = new QTableWidgetItem(desc);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 1, newItem);

        newItem = new QTableWidgetItem(pv->getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 2, newItem);

        newItem = new QTableWidgetItem(pv->installed() ? "installed": "");
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(i, 3, newItem);
    }
    t->setSortingEnabled(true);
    qDebug() << "MainWindow::fillList.2";
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
    it->wait();
    delete it;

    fillList();
    delete job;
}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    PackageVersion* pv = getSelectedPackageVersion();
    this->ui->actionInstall->setEnabled(pv && !pv->installed());
    this->ui->actionUninstall->setEnabled(pv && pv->installed());
    Package* p;
    if (pv)
        p = Repository::getDefault()->findPackage(pv->package);
    else
        p = 0;
    this->ui->actionGotoPackageURL->setEnabled(pv && p &&
            QUrl(p->url).isValid());
}

void MainWindow::loadRepository()
{
    Job* job = new Job();
    InstallThread* it = new InstallThread(0, true, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job);
    it->wait();
    delete it;

    fillList();
    delete job;

    qDebug() << "MainWindow::loadRepository";
}

void MainWindow::on_actionInstall_activated()
{
    PackageVersion* pv = getSelectedPackageVersion();
    Job* job = new Job();
    InstallThread* it = new InstallThread(pv, true, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job);
    it->wait();
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
    qDebug() << "MainWindow::on_pushButtonSaveSettings_clicked";
}

void MainWindow::on_actionGotoPackageURL_triggered()
{
    PackageVersion* pv = getSelectedPackageVersion();
    if (pv) {
        Package* p = Repository::getDefault()->findPackage(pv->package);
        if (p) {
            QUrl url(p->url);
            if (url.isValid()) {
                QDesktopServices::openUrl(url);
            }
        }
    }
}
