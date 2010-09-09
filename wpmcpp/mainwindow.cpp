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
#include "progressdialog.h"
#include "settingsdialog.h"

class InstallThread: public QThread
{
    PackageVersion* pv;

    // 0 = uninstall
    // 1 = install
    // 2 = update,
    // 3, 4 = recognize installed applications + load repositories
    int type;

    Job* job;
public:
    InstallThread(PackageVersion* pv, int install, Job* job);

    void run();
};

InstallThread::InstallThread(PackageVersion *pv, int type, Job* job)
{
    this->pv = pv;
    this->type = type;
    this->job = job;
}

void InstallThread::run()
{
    CoInitialize(NULL);

    qDebug() << "InstallThread::run.1";
    switch (this->type) {
    case 0:
        pv->uninstall(job);
        break;
    case 1:
        pv->install(job);
        break;
    case 2:
        pv->update(job);
        break;
    case 3:
    case 4:
        Repository::getDefault()->load(job);
        break;
    }

    CoUninitialize();

    qDebug() << "InstallThread::run.2";
}

bool MainWindow::winEvent(MSG* message, long* result)
{    
    if (message->message == WM_ICONTRAY) {
        // qDebug() << "MainWindow::winEvent " << message->lParam;
        switch (message->lParam) {
            case (LPARAM) NIN_BALLOONUSERCLICK:
                this->ui->comboBoxStatus->setCurrentIndex(3);
                this->prepare();
                ((QApplication*) QApplication::instance())->
                        setQuitOnLastWindowClosed(true);
                this->showMaximized();
                this->activateWindow();
                break;
            case (LPARAM) NIN_BALLOONTIMEOUT:
                ((QApplication*) QApplication::instance())->quit();
                break;
        }
        return true;
    }
    return false;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Windows Package Manager");

    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);

    QList<QUrl*> urls = Repository::getRepositoryURLs();
    if (urls.count() == 0) {
        urls.append(new QUrl(
                "http://windows-package-manager.googlecode.com/hg/repository/Rep.xml"));
        Repository::setRepositoryURLs(urls);
    }
    qDeleteAll(urls);
    urls.clear();

    this->on_tableWidget_itemSelectionChanged();
    this->ui->tableWidget->setColumnCount(4);
    this->ui->tableWidget->setColumnWidth(0, 150);
    this->ui->tableWidget->setColumnWidth(1, 300);
    this->ui->tableWidget->sortItems(0);
}

void MainWindow::prepare()
{
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

bool MainWindow::waitFor(Job* job, QString& title)
{
    ProgressDialog* pd;
    pd = new ProgressDialog(this, job, title);
    pd->setModal(true);

    qDebug() << "MainWindow::waitFor.1";

    pd->exec();
    delete pd;
    pd = 0;

    qDebug() << "MainWindow::waitFor.2";
    if (job->isCancelled())
        return false;
    else if (!job->getErrorMessage().isEmpty()) {
        QMessageBox::critical(this,
                "Error", job->getErrorMessage(),
                QMessageBox::Ok);
        return false;
    } else {
        return true;
    }
}

void MainWindow::onShow()
{
    recognizeAndloadRepositories();
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
    // qDebug() << "MainWindow::fillList";
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

    int statusFilter = this->ui->comboBoxStatus->currentIndex();
    QStringList textFilter =
            this->ui->lineEditText->text().toLower().simplified().split(" ");

    t->setRowCount(r->packageVersions.count());

    int n = 0;
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);

        bool installed = pv->installed();

        // filter by status (1, 2)
        if ((statusFilter == 1 && installed) ||
                (statusFilter == 2 && !installed))
            continue;

        // filter by text
        QString fullText = pv->getFullText();
        bool b = true;
        for (int i = 0; i < textFilter.count(); i++) {
            if (fullText.indexOf(textFilter.at(i)) < 0) {
                b = false;
                break;
            }
        }
        if (!b)
            continue;

        PackageVersion* newest = r->findNewestPackageVersion(pv->package);
        bool updateAvailable = newest->version.compare(pv->version) > 0;

        // filter by status (3)
        if (statusFilter == 3 && (!installed || !updateAvailable))
            continue;

        // filter by status (4)
        if (statusFilter == 4 && !(installed || !updateAvailable))
            continue;

        Package* p = r->findPackage(pv->package);

        QString packageTitle;
        if (p)
            packageTitle = p->title;
        else
            packageTitle = pv->package;
        newItem = new QTableWidgetItem(packageTitle);
        newItem->setStatusTip(pv->download.toString() + " " + pv->package);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 0, newItem);

        QString desc;
        if (p)
            desc = p->description;
        newItem = new QTableWidgetItem(desc);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 1, newItem);

        newItem = new QTableWidgetItem(pv->version.getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 2, newItem);

        newItem = new QTableWidgetItem("");
        QString status;
        if (installed) {
            if (pv->external)
                status = "installed externally";
            else
                status = "installed";
        }
        if (installed && updateAvailable) {
            newItem->setBackgroundColor(QColor(255, 0xc7, 0xc7));
            status += ", update available";
        }
        newItem->setText(status);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 3, newItem);

        n++;
    }
    t->setRowCount(n);
    t->setSortingEnabled(true);
    // qDebug() << "MainWindow::fillList.2";
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
    QStringList locked = pv->findLockedFiles();
    if (locked.size() > 0) {
        QString locked_ = locked.join(", \n");
        QString msg("The package cannot be uninstalled because "
                "the following files are in use "
                "(please close the corresponding applications): "
                "%1");
        QMessageBox::critical(this,
                "Uninstall",
                msg.arg(locked_));
    } else {
        QString msg("The directory %1 "
                "will be completely deleted. "
                "There is no way to restore the files. "
                "Are you sure?");
        msg = msg.arg(pv->getDirectory().absolutePath());
        QMessageBox::StandardButton b = QMessageBox::warning(this,
                "Uninstall",
                msg, QMessageBox::Yes | QMessageBox::No);
        if (b == QMessageBox::Yes) {
            Job* job = new Job();
            InstallThread* it = new InstallThread(pv, 0, job);
            it->start();
            it->setPriority(QThread::LowestPriority);

            QString title("Uninstalling");
            waitFor(job, title);
            it->wait();
            delete it;

            fillList();
            delete job;
        }
    }
}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    PackageVersion* pv = getSelectedPackageVersion();
    this->ui->actionInstall->setEnabled(pv && !pv->installed());
    this->ui->actionUninstall->setEnabled(pv && pv->installed() &&
            !pv->external);

    // "Update"
    if (pv && !pv->external) {
        Repository* r = Repository::getDefault();
        PackageVersion* newest = r->findNewestPackageVersion(pv->package);
        PackageVersion* newesti = r->findNewestInstalledPackageVersion(
                pv->package);
        this->ui->actionUpdate->setEnabled(
                newest != 0 && newesti != 0 &&
                newest->version.compare(newesti->version) > 0);
    } else {
        this->ui->actionUpdate->setEnabled(false);
    }

    // enable "Go To Package Page"
    Package* p;
    if (pv)
        p = Repository::getDefault()->findPackage(pv->package);
    else
        p = 0;
    this->ui->actionGotoPackageURL->setEnabled(pv && p &&
            QUrl(p->url).isValid());

    this->ui->actionTest_Download_Site->setEnabled(pv && p &&
            QUrl(p->url).isValid());
}

void MainWindow::recognizeAndloadRepositories()
{
    Job* job = new Job();
    InstallThread* it = new InstallThread(0, 3, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    QString title("Initializing");
    waitFor(job, title);
    it->wait();
    delete it;

    fillList();
    delete job;
}

void MainWindow::loadRepositories()
{
    Job* job = new Job();
    InstallThread* it = new InstallThread(0, 4, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    QString title("Loading repositories");
    waitFor(job, title);
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
    InstallThread* it = new InstallThread(pv, 1, job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    QString title("Installing");
    waitFor(job, title);
    it->wait();
    delete it;

    fillList();
    delete job;
}


void MainWindow::on_pushButtonSaveSettings_clicked()
{
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

void MainWindow::on_comboBox_activated(int index)
{
}

void MainWindow::on_comboBoxStatus_currentIndexChanged(int index)
{
    this->fillList();
}

void MainWindow::on_lineEditText_textChanged(QString )
{
    this->fillList();
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog d;

    QList<QUrl*> urls = Repository::getRepositoryURLs();
    QStringList list;
    for (int i = 0; i < urls.count(); i++) {
        list.append(urls.at(i)->toString());
    }
    d.setRepositoryURLs(list);

    qDeleteAll(urls);
    urls.clear();

    if (d.exec() == QDialog::Accepted) {
        list = d.getRepositoryURLs();
        if (list.count() == 0) {
            QMessageBox::critical(this,
                    "Error", "No repositories defined", QMessageBox::Ok);
        } else {
            QString err;
            for (int i = 0; i < list.count(); i++) {
                QUrl* url = new QUrl(list.at(i));
                urls.append(url);
                if (!url->isValid()) {
                    err = QString("%1 is not a valid repository address").arg(
                            list.at(i));
                    break;
                }
            }
            if (err.isEmpty()) {
                Repository::setRepositoryURLs(urls);
                loadRepositories();
            } else {
                QMessageBox::critical(this,
                        "Error", err, QMessageBox::Ok);
            }
            qDeleteAll(urls);
            urls.clear();
        }
    }
}

void MainWindow::on_actionUpdate_triggered()
{
    PackageVersion* pv = getSelectedPackageVersion();
    Repository* r = Repository::getDefault();
    PackageVersion* newest = r->findNewestPackageVersion(pv->package);
    PackageVersion* newesti = r->findNewestInstalledPackageVersion(pv->package);
    QString msg("This will uninstall the current version (%1) "
            "and install the newest available (%2). "
            "The directory %3 will be completely deleted. "
            "There is no way to restore the files. "
            "Are you sure?");
    msg = msg.arg(newesti->version.getVersionString()).
            arg(newest->version.getVersionString()).
            arg(newesti->getDirectory().absolutePath());

    QMessageBox::StandardButton b = QMessageBox::warning(this,
            "Update",
            msg, QMessageBox::Yes | QMessageBox::No);
    if (b == QMessageBox::Yes) {
        Job* job = new Job();
        InstallThread* it = new InstallThread(newesti, 2, job);
        it->start();
        it->setPriority(QThread::LowestPriority);

        QString title("Updating");
        waitFor(job, title);
        it->wait();
        delete it;

        fillList();
        delete job;
    }
}

void MainWindow::on_actionTest_Download_Site_triggered()
{
    PackageVersion* pv = getSelectedPackageVersion();
    if (pv) {
        QString s = "http://www.urlvoid.com/scan/" + pv->download.host();
        QUrl url(s);
        if (url.isValid()) {
            QDesktopServices::openUrl(url);
        }
    }
}
