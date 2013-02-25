#include <math.h>

#include <qabstractitemview.h>
#include <qmessagebox.h>
#include <qvariant.h>
#include <qprogressdialog.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDateTime>
#include <QDesktopServices>
#include <qinputdialog.h>
#include <qfiledialog.h>
#include <QTextStream>
#include <QIODevice>
#include <QMenu>
#include <QTextEdit>
#include <QScrollArea>
#include <QPushButton>
#include <QCloseEvent>
#include <QTextBrowser>
#include <QTableWidget>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "repository.h"
#include "job.h"
#include "wpmutils.h"
#include "installoperation.h"
#include "downloader.h"
#include "packageversionform.h"
#include "uiutils.h"
#include "progressframe.h"
#include "messageframe.h"
#include "settingsframe.h"
#include "licenseform.h"
#include "packageframe.h"
#include "hrtimer.h"
#include "dbrepository.h"

extern HWND defaultPasswordWindow;

QMap<QString, QIcon> MainWindow::icons;
QIcon MainWindow::genericAppIcon;
MainWindow* MainWindow::instance = 0;

class InstallThread: public QThread
{
    PackageVersion* pv;

    // 0, 1, 2 = install/uninstall
    // 3, 4 = recognize installed applications + load repositories
    // 8 = scan hard drives
    int type;

    Job* job;

    void scanHardDrives();
public:
    // name of the log file for type=6
    // directory for type=7
    QString logFile;

    QList<InstallOperation*> install;

    /**
     * type = 3 or 4
     * true (default value) = the HTTP cache will be used
     */
    bool useCache;

    InstallThread(PackageVersion* pv, int type, Job* job);

    void run();
};

InstallThread::InstallThread(PackageVersion *pv, int type, Job* job)
{
    this->pv = pv;
    this->type = type;
    this->job = job;
    this->useCache = false;
}

void InstallThread::scanHardDrives()
{
    Repository* r = Repository::getDefault();
    r->scanHardDrive(job);
}

void InstallThread::run()
{
    CoInitialize(NULL);

    // qDebug() << "InstallThread::run.1";
    switch (this->type) {
    case 0:
    case 1:
    case 2:
        Repository::getDefault()->process(job, install);
        break;
    case 3:
    case 4: {
        DBRepository* dbr = DBRepository::getDefault();
        Repository* r = Repository::getDefault();
        if (job->shouldProceed("Downloading the remote repositories")) {
            Job* sub = job->newSubJob(0.8);
            r->reload(sub, this->useCache);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(sub->getErrorMessage());
            delete sub;

            PackageVersion* pv = r->findOrCreatePackageVersion(
                    "com.googlecode.windows-package-manager.Npackd",
                    Version(WPMUtils::NPACKD_VERSION));
            if (!pv->installed()) {
                pv->setPath(WPMUtils::getExeDir());
            }
        }

        if (job->shouldProceed("Filling the local database")) {
            Job* sub = job->newSubJob(0.19);
            dbr->reloadFrom(sub, r);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(sub->getErrorMessage());
            delete sub;
        }

        break;
    }
    case 8:
        scanHardDrives();
        break;
    }

    CoUninitialize();

    // qDebug() << "InstallThread::run.2";
}

/**
 * Scan hard drives.
 */
class ScanHardDrivesThread: public QThread
{
    Job* job;
public:
    QList<PackageVersion*> detected;

    ScanHardDrivesThread(Job* job);

    void run();
};

ScanHardDrivesThread::ScanHardDrivesThread(Job *job)
{
    this->job = job;
}

void ScanHardDrivesThread::run()
{
    Repository* r = Repository::getDefault();
    QList<PackageVersion*> s1 = r->getInstalled();
    r->scanHardDrive(job);
    QList<PackageVersion*> s2 = r->getInstalled();

    for (int i = 0; i < s2.count(); i++) {
        PackageVersion* pv = s2.at(i);
        if (!s1.contains(pv)) {
            detected.append(pv);
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    instance = this;

    ui->setupUi(this);

    this->setMenuAccelerators();
    this->setActionAccelerators(this);

    this->taskbarMessageId = 0;

    this->monitoredJobLastChanged = 0;
    this->progressContent = 0;
    this->jobsTab = 0;
    this->taskbarInterface = 0;

    this->hardDriveScanRunning = false;
    this->reloadRepositoriesThreadRunning = false;

    setWindowTitle("Npackd");

    this->genericAppIcon = QIcon(":/images/app.png");

    this->mainFrame = new MainFrame(this);

    //this->ui->formLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);

    updateActions();

    QTableWidget* t = this->mainFrame->getTableWidget();
    t->addAction(this->ui->actionInstall);
    t->addAction(this->ui->actionUninstall);
    t->addAction(this->ui->actionUpdate);
    t->addAction(this->ui->actionShow_Details);
    t->addAction(this->ui->actionGotoPackageURL);
    t->addAction(this->ui->actionTest_Download_Site);

    connect(&this->fileLoader, SIGNAL(downloaded(const FileLoaderItem&)), this,
            SLOT(iconDownloaded(const FileLoaderItem&)),
            Qt::QueuedConnection);
    this->fileLoader.start(QThread::LowestPriority);

    // copy toolTip to statusTip for all actions
    for (int i = 0; i < this->children().count(); i++) {
        QObject* ch = this->children().at(i);
        QAction* a = dynamic_cast<QAction*>(ch);
        if (a) {
            a->setStatusTip(a->toolTip());
        }
    }

    connect(QApplication::instance(),
            SIGNAL(focusChanged(QWidget*, QWidget*)), this,
            SLOT(applicationFocusChanged(QWidget*, QWidget*)));

    this->ui->tabWidget->addTab(mainFrame, "Packages");
    this->addJobsTab();
    this->mainFrame->getFilterLineEdit()->setFocus();

    //Repository* r = Repository::getDefault();
    /* TODO: connect(r, SIGNAL(statusChanged(const QString&)), this,
            SLOT(repositoryStatusChanged(const QString&)),
            Qt::QueuedConnection);*/

    defaultPasswordWindow = this->winId();

    this->taskbarMessageId = RegisterWindowMessage(L"TaskbarButtonCreated");
    // qDebug() << "id " << taskbarMessageId;

    // Npackd runs elevated and the taskbar does not. We have to allow the
    // taskbar event here.
    HINSTANCE hInstLib = LoadLibraryA("USER32.DLL");
    BOOL WINAPI (*lpfChangeWindowMessageFilterEx)
            (HWND, UINT, DWORD, void*) =
            (BOOL (WINAPI*) (HWND, UINT, DWORD, void*))
            GetProcAddress(hInstLib, "ChangeWindowMessageFilterEx");
    if (lpfChangeWindowMessageFilterEx)
        lpfChangeWindowMessageFilterEx(winId(), taskbarMessageId, 1, 0);
    FreeLibrary(hInstLib);
}

void MainWindow::applicationFocusChanged(QWidget* old, QWidget* now)
{
    updateActions();
}

bool MainWindow::winEvent(MSG* message, long* result)
{
    if (message->message == taskbarMessageId) {
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL,
                CLSCTX_INPROC_SERVER, IID_ITaskbarList3,
                reinterpret_cast<void**> (&(taskbarInterface)));

        if (SUCCEEDED(hr)) {
            hr = taskbarInterface->HrInit();

            if (FAILED(hr)) {
                taskbarInterface->Release();
                taskbarInterface = 0;
            }
        }
        return true;
    }
    return false;
}

void MainWindow::showDetails()
{
    Selection* sel = Selection::findCurrent();
    if (sel) {
        QList<void*> selected = sel->getSelected("PackageVersion");
        if (selected.count() > 0) {
            for (int i = 0; i < selected.count(); i++) {
                PackageVersion* pv = (PackageVersion*) selected.at(i);

                openPackageVersion(pv->package, pv->version, true);
            }
        } else {
            selected = sel->getSelected("Package");
            for (int i = 0; i < selected.count(); i++) {
                Package* p = (Package*) selected.at(i);

                int index = this->findPackageTab(p->name);
                if (index < 0) {
                    PackageFrame* pf = new PackageFrame(this->ui->tabWidget);
                    Package* p_ = DBRepository::getDefault()->
                            findPackage(p->name);
                    if (p_) {
                        pf->fillForm(p_);
                        QIcon icon = getPackageVersionIcon(p->name);
                        QString t = p->title;
                        if (t.isEmpty())
                            t = p->name;
                        this->ui->tabWidget->addTab(pf, icon, t);
                        index = this->ui->tabWidget->count() - 1;
                    }
                }

                if (i == selected.count() - 1)
                    this->ui->tabWidget->setCurrentIndex(index);
            }
        }
    }
}

void MainWindow::updateIcons()
{
    QTableWidget* t = this->mainFrame->getTableWidget();
    for (int i = 0; i < t->rowCount(); i++) {
        QTableWidgetItem *item = t->item(i, 0);
        const QVariant v = item->data(Qt::UserRole);
        QString icon = v.toString();

        if (!icon.isEmpty() && this->icons.contains(icon)) {
            item->setIcon(this->icons[icon]);
        }
    }

    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        if (pvf) {
            pvf->updateIcons();
            QIcon icon = getPackageVersionIcon(pvf->pv->package);
            this->ui->tabWidget->setTabIcon(i, icon);
        }
    }
}

int MainWindow::findPackageTab(const QString& package) const
{
    int r = -1;
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageFrame* pf = dynamic_cast<PackageFrame*>(w);
        if (pf) {
            if (pf->p->name == package) {
                r = i;
                break;
            }
        }
    }
    return r;
}

int MainWindow::findPackageVersionTab(const QString& package,
        const Version& version) const
{
    int r = -1;
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        if (pvf) {
            //qDebug() << pvf->pv.data()->toString() << "---" <<
            //        package << version.getVersionString();
            if (pvf->pv->package == package && pvf->pv->version == version) {
                r = i;
                break;
            }
        }
    }
    return r;
}

int MainWindow::findLicenseTab(const QString& name) const
{
    int r = -1;
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        LicenseForm* pvf = dynamic_cast<LicenseForm*>(w);
        if (pvf) {
            //qDebug() << pvf->pv.data()->toString() << "---" <<
            //        package << version.getVersionString();
            if (pvf->license->name == name) {
                r = i;
                break;
            }
        }
    }
    return r;
}

void MainWindow::updateStatusInDetailTabs()
{
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        if (pvf) {
            pvf->updateStatus();
        } else {
            PackageFrame* pf = dynamic_cast<PackageFrame*>(w);
            if (pf)
                pf->updateStatus();
        }
    }
}

QIcon MainWindow::getPackageVersionIcon(const QString& package)
{
    AbstractRepository* r = AbstractRepository::getDefault_();
    Package* p = r->findPackage_(package);

    QIcon icon = MainWindow::genericAppIcon;
    if (p) {
        if (!p->icon.isEmpty() && MainWindow::icons.contains(p->icon)) {
            icon = MainWindow::icons[p->icon];
        }
    }
    delete p;

    return icon;
}

MainWindow* MainWindow::getInstance()
{
    return instance;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    int n = this->runningJobs.count();

    if (n == 0) {
        event->accept();
    } else {
        addErrorMessage("Cannot exit while jobs are running");
        event->ignore();
    }
}

void MainWindow::repositoryStatusChanged(const QString&)
{
    // qDebug() << "MainWindow::repositoryStatusChanged" << pv->toString();

    this->updateStatusInTable();
    this->updateStatusInDetailTabs();
    this->updateActions();
}

void MainWindow::iconDownloaded(const FileLoaderItem& it)
{
    if (it.f) {
        // qDebug() << "MainWindow::iconDownloaded.2 " << it.url;
        QPixmap pm(it.f->fileName());
        delete it.f;
        if (!pm.isNull()) {
            QIcon icon(pm);
            icon.detach();
            this->icons.insert(it.url, icon);
            updateIcons();
        }
    }
}

void MainWindow::prepare()
{
    QTimer *pTimer = new QTimer(this);
    pTimer->setSingleShot(true);
    connect(pTimer, SIGNAL(timeout()), this, SLOT(onShow()));

    pTimer->start(0);
}

void MainWindow::updateStatusInTable()
{
    AbstractRepository* r = AbstractRepository::getDefault_();
    QTableWidget* t = this->mainFrame->getTableWidget();
    for (int i = 0; i < t->rowCount(); i++) {
        QTableWidgetItem* newItem = t->item(i, 4);

        const QVariant v = newItem->data(Qt::UserRole);
        Package* p = (Package *) v.value<void*>();
        PackageVersion* newestInstallable =
                r->findNewestInstallablePackageVersion_(p->name);
        PackageVersion* newestInstalled =
                r->findNewestInstalledPackageVersion_(p->name);

        if (newestInstalled && newestInstallable &&
                newestInstallable->version.compare(
                newestInstalled->version) > 0)
            newItem->setBackgroundColor(QColor(255, 0xc7, 0xc7));
        else
            newItem->setBackgroundColor(QColor(255, 255, 255));

        delete newestInstallable;
        delete newestInstalled;
    }
}

void MainWindow::updateProgressTabTitle()
{
    int n = this->runningJobStates.count();
    time_t max = 0;
    double maxProgress = 0;
    for (int i = 0; i < n; i++) {
        JobState state = this->runningJobStates.at(i);

        // state.job may be null if the corresponding task was just started
        if (state.job) {
            time_t t = state.remainingTime();
            if (t > max) {
                max = t;
                maxProgress = state.progress;
            }
        }
    }
    int maxProgress_ = lround(maxProgress * 100);
    QTime rest = WPMUtils::durationToTime(max);

    QString title;
    if (n == 0)
        title = QString("0 Jobs");
    else if (n == 1)
        title = QString("1 Job (%1%, %2)").arg(maxProgress_).
                arg(rest.toString());
    else
        title = QString("%1 Jobs (%2%, %3)").arg(n).arg(maxProgress_).
                arg(rest.toString());

    int index = this->ui->tabWidget->indexOf(this->jobsTab);
    this->ui->tabWidget->setTabText(index, title);

    if (this->taskbarInterface) {
        if (n == 0)
            taskbarInterface->SetProgressState(winId(), TBPF_NOPROGRESS);
        else {
            taskbarInterface->SetProgressState(winId(), TBPF_NORMAL);
            taskbarInterface->SetProgressValue(winId(),
                    lround(maxProgress * 10000), 10000);
        }
    }
}

MainWindow::~MainWindow()
{
    this->fileLoader.terminated = 1;
    if (!this->fileLoader.wait(1000))
        this->fileLoader.terminate();
    qDeleteAll(found);
    delete ui;
}

void MainWindow::monitoredJobChanged(const JobState& state)
{
    time_t now;
    time(&now);

    if (now != this->monitoredJobLastChanged) {
        this->monitoredJobLastChanged = now;

        int index = this->runningJobs.indexOf(state.job);
        if (index >= 0) {
            this->runningJobStates.replace(index, state);
        }

        updateProgressTabTitle();
    }
}

void MainWindow::monitor(Job* job, const QString& title, QThread* thread)
{
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(monitoredJobChanged(const JobState&)),
            Qt::QueuedConnection);

    this->runningJobs.append(job);
    this->runningJobStates.append(JobState());

    updateProgressTabTitle();

    ProgressFrame* pf = new ProgressFrame(progressContent, job, title,
            thread);
    pf->resize(100, 100);
    QVBoxLayout* layout = (QVBoxLayout*) progressContent->layout();
    layout->insertWidget(0, pf);

    progressContent->resize(500, 500);
}

void MainWindow::onShow()
{
    DBRepository* dbr = DBRepository::getDefault();
    QString err = dbr->open();
    if (err.isEmpty())
        recognizeAndLoadRepositories(false);
    else
        this->addErrorMessage(err, err, true, QMessageBox::Critical);
}

void MainWindow::selectPackages(QList<Package*> ps)
{
    QTableWidget* t = this->mainFrame->getTableWidget();
    t->clearSelection();
    for (int i = 0; i < t->rowCount(); i++) {
        const QVariant v = t->item(i, 1)->
                data(Qt::UserRole);
        Package* f = (Package*) v.value<void*>();
        if (ps.contains(f)) {
            //topLeft = t->selectionModel()->selection().
            QModelIndex topLeft = t->model()->index(i, 0);
            // QModelIndex bottomRight = t->model()->index(i, t->columnCount() - 1);
            t->selectionModel()->select(topLeft, QItemSelectionModel::Rows |
                    QItemSelectionModel::Select);
        }
    }
}

class QCITableWidgetItem: public QTableWidgetItem {
public:
    explicit QCITableWidgetItem(const QString &text, int type = Type);
    virtual bool operator<(const QTableWidgetItem &other) const;
};

QCITableWidgetItem::QCITableWidgetItem(const QString &text, int type)
    : QTableWidgetItem(text, type)
{
}

bool QCITableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    QString a = this->text();
    QString b = other.text();

    return a.compare(b, Qt::CaseInsensitive) <= 0;
}

void MainWindow::fillList()
{
    // Columns and data types:
    // 0: icon QString
    // 1: package name Package*
    // 2: package description Package*
    // 3: available version Package*
    // 4: installed versions Package*
    // 5: license Package*

    // qDebug() << "MainWindow::fillList";
    QTableWidget* t = this->mainFrame->getTableWidget();

    t->clearContents();
    t->setUpdatesEnabled(false);
    t->setSortingEnabled(false);

    AbstractRepository* r = AbstractRepository::getDefault_();

    t->setColumnCount(6);

    QTableWidgetItem *newItem = new QTableWidgetItem("Icon");
    t->setHorizontalHeaderItem(0, newItem);

    newItem = new QTableWidgetItem("Title");
    t->setHorizontalHeaderItem(1, newItem);

    newItem = new QTableWidgetItem("Description");
    t->setHorizontalHeaderItem(2, newItem);

    newItem = new QTableWidgetItem("Available");
    t->setHorizontalHeaderItem(3, newItem);

    newItem = new QTableWidgetItem("Installed");
    t->setHorizontalHeaderItem(4, newItem);

    newItem = new QTableWidgetItem("License");
    t->setHorizontalHeaderItem(5, newItem);

    int statusFilter = this->mainFrame->getStatusComboBox()->currentIndex();
    QStringList textFilter =
            this->mainFrame->getFilterLineEdit()->text().
            toLower().simplified().split(" ");

    QSet<QString> requestedIcons;

    t->setRowCount(0);
    DBRepository* dbr = DBRepository::getDefault();
    qDeleteAll(found);
    found = dbr->findPackages(textFilter);
    t->setRowCount(found.count());

    for (int i = 0; i < found.count(); i++) {
        Package* p = found.at(i);
        if (!p->icon.isEmpty()) {
            if (!icons.contains(p->icon)) {
                FileLoaderItem it;
                it.url = p->icon;
                // qDebug() << "MainWindow::loadRepository " << it.url;
                this->fileLoader.work.append(it);
            }
        }
    }
    // qDebug() << "MainWindow::loadRepository";

    int n = 0;
    for (int i = 0; i < found.count(); i++) {
        Package* p = found.at(i);

        QString err;
        // TODO: error is not handled
        QList<PackageVersion*> pvs = dbr->getPackageVersions(p->name, &err);
        if (pvs.count() == 0)
            continue;

        QString installed;
        bool atLeastOneInstalled = false;
        for (int j = pvs.count() - 1; j >= 0; j--) {
            PackageVersion* pv = pvs.at(j);
            if (pv->installed()) {
                atLeastOneInstalled = true;
                if (!installed.isEmpty())
                    installed.append(", ");
                installed.append(pv->version.getVersionString());
            }
        }
        qDeleteAll(pvs);
        pvs.clear();

        bool updateEnabled = isUpdateEnabled(p->name);

        QSharedPointer<PackageVersion> newestInstallable(
                r->findNewestInstallablePackageVersion_(p->name));

        if (!atLeastOneInstalled && !newestInstallable)
            continue;

        bool statusOK;
        switch (statusFilter) {
            case 0:
                // all
                statusOK = true;
                break;
            case 1:
                // not installed
                statusOK = !atLeastOneInstalled;
                break;
            case 2:
                // installed
                statusOK = atLeastOneInstalled;
                break;
            case 3:
                // installed, updateable
                statusOK = atLeastOneInstalled && updateEnabled;
                break;
            default:
                statusOK = true;
                break;
        }
        if (!statusOK)
            continue;

        if (!p->icon.isEmpty() && !requestedIcons.contains(p->icon)) {
            FileLoaderItem it;
            it.url = p->icon;
            // qDebug() << "MainWindow::loadRepository " << it.url;
            this->fileLoader.work.append(it);
            requestedIcons += p->icon;
        }

        newItem = new QTableWidgetItem("");
        newItem->setData(Qt::UserRole, qVariantFromValue(p->icon));
        if (p) {
            if (!p->icon.isEmpty() && this->icons.contains(p->icon)) {
                QIcon icon = this->icons[p->icon];
                newItem->setIcon(icon);
            } else {
                newItem->setIcon(MainWindow::genericAppIcon);
            }
        } else {
            newItem->setIcon(MainWindow::genericAppIcon);
        }
        t->setItem(n, 0, newItem);

        newItem = new QCITableWidgetItem(p->title);
        newItem->setStatusTip(p->name);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) p));
        t->setItem(n, 1, newItem);

        newItem = new QCITableWidgetItem(p->description);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) p));
        t->setItem(n, 2, newItem);

        newItem = new QTableWidgetItem("");
        if (newestInstallable)
            newItem->setText(newestInstallable->version.getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) p));
        t->setItem(n, 3, newItem);

        newItem = new QCITableWidgetItem(installed);
        QSharedPointer<PackageVersion> newestInstalled(
                r->findNewestInstalledPackageVersion_(p->name));
        if (newestInstalled && newestInstallable &&
                newestInstallable->version.compare(
                newestInstalled->version) > 0)
            newItem->setBackgroundColor(QColor(255, 0xc7, 0xc7));
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) p));
        t->setItem(n, 4, newItem);

        newItem = new QCITableWidgetItem("");
        QString licenseTitle;
        if (p) {
            QSharedPointer<License> lic(r->findLicense_(p->license));
            if (lic)
                licenseTitle = lic->title;
        }
        newItem->setText(licenseTitle);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) p));
        t->setItem(n, 5, newItem);

        n++;
    }

    t->setRowCount(n);
    t->setSortingEnabled(true);
    t->setUpdatesEnabled(true);
    // qDebug() << "MainWindow::fillList.2";
}

void MainWindow::process(QList<InstallOperation*> &install)
{
    // reoder the operations if a package is updated. In this case it is better
    // to uninstall the old first and then install the new one.
    if (install.size() == 2) {
        InstallOperation* first = install.at(0);
        InstallOperation* second = install.at(1);
        if (first->package == second->package &&
                first->install && !second->install) {
            install.insert(0, second);
            install.removeAt(2);
        }
    }

    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        // TODO: op->findPackageVersion() may return 0
        QScopedPointer<PackageVersion> pv(op->findPackageVersion());
        if (pv->isLocked()) {
            QString msg("The package %1 is locked by a "
                    "currently running installation/removal.");
            this->addErrorMessage(msg.arg(pv->toString()),
                    msg.arg(pv->toString()), true, QMessageBox::Critical);
            qDeleteAll(install);
            install.clear();
            return;
        }
    }

    QString err = Repository::checkLockedFilesForUninstall(install);
    if (!err.isEmpty()) {
        addErrorMessage(err, err, true, QMessageBox::Critical);
        qDeleteAll(install);
        install.clear();
        return;
    }

    QString names;
    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (!op->install) {
            // TODO: op->findPackageVersion() may return 0
            QScopedPointer<PackageVersion> pv(op->findPackageVersion());
            if (!names.isEmpty())
                names.append(", ");
            names.append(pv->toString());
        }
    }
    QString installNames;
    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (op->install) {
            // TODO: op->findPackageVersion() may return 0
            QScopedPointer<PackageVersion> pv(op->findPackageVersion());
            if (!installNames.isEmpty())
                installNames.append(", ");
            installNames.append(pv->toString());
        }
    }

    int installCount = 0, uninstallCount = 0;
    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (op->install)
            installCount++;
        else
            uninstallCount++;
    }

    bool b;
    QString msg;
    QString title;
    if (installCount == 1 && uninstallCount == 0) {
        b = true;
        title = "Installing";
    } else if (installCount == 0 && uninstallCount == 1) {
        title = "Uninstalling";
        // TODO: install.at(0)->findPackageVersion() may return 0
        QScopedPointer<PackageVersion> pv(install.at(0)->findPackageVersion());
        msg = QString("The package %1 will be uninstalled. "
                "The corresponding directory %2 "
                "will be completely deleted. "
                "There is no way to restore the files.").
                arg(pv->toString()).
                arg(pv->getPath());
        b = UIUtils::confirm(this, "Uninstall", msg);
    } else if (installCount > 0 && uninstallCount == 0) {
        title = QString("Installing %1 packages").arg(
                installCount);
        msg = QString("%1 package(s) will be installed: %2").
                arg(installCount).arg(installNames);
        b = UIUtils::confirm(this, "Install", msg);
    } else if (installCount == 0 && uninstallCount > 0) {
        title = QString("Uninstalling %1 packages").arg(
                uninstallCount);
        msg = QString("%1 package(s) will be uninstalled: %2. "
                "The corresponding directories "
                "will be completely deleted. "
                "There is no way to restore the files.").
                arg(uninstallCount).arg(names);
        b = UIUtils::confirm(this, "Uninstall", msg);
    } else {
        title = QString("Installing %1 packages, uninstalling %2 packages").arg(
                installCount).arg(uninstallCount);
        msg = QString("%1 package(s) will be uninstalled: %2 ("
                "the corresponding directories "
                "will be completely deleted; "
                "there is no way to restore the files) "
                "and %3 package(s) will be installed: %4.").arg(uninstallCount).
                arg(names).
                arg(installCount).
                arg(installNames);
        b = UIUtils::confirm(this, "Install/Uninstall", msg);
    }

    if (b) {
        Job* job = new Job();
        InstallThread* it = new InstallThread(0, 1, job);
        it->install = install;
        install.clear();

        connect(it, SIGNAL(finished()), this,
                SLOT(processThreadFinished()),
                Qt::QueuedConnection);

        monitor(job, title, it);
    } else {
        qDeleteAll(install);
        install.clear();
    }
}

void MainWindow::processThreadFinished()
{
    QTableWidget* t = this->mainFrame->getTableWidget();
    QList<Package*> sel = mainFrame->getSelectedPackagesInTable();
    int col = t->currentColumn();
    int row = t->currentRow();
    fillList();
    updateStatusInDetailTabs();
    t->setCurrentCell(row, col);
    selectPackages(sel);
    updateActions();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    case QEvent::ActivationChange:
        // qDebug() << "QEvent::ActivationChange";
        QTimer::singleShot(0, this, SLOT(updateActionsSlot()));
        break;
    default:
        break;
    }
}

void MainWindow::on_actionExit_triggered()
{
    int n = this->runningJobs.count();

    if (n > 0)
        addErrorMessage("Cannot exit while jobs are running");
    else
        this->close();
}

void MainWindow::unregisterJob(Job *job)
{
    int index = this->runningJobs.indexOf(job);
    if (index >= 0) {
        this->runningJobs.removeAt(index);
        this->runningJobStates.removeAt(index);
    }

    this->updateProgressTabTitle();
}

void MainWindow::on_actionUninstall_activated()
{
}

bool MainWindow::isUpdateEnabled(const QString& package)
{
    bool res = false;
    AbstractRepository* r = AbstractRepository::getDefault_();
    PackageVersion* newest = r->findNewestInstallablePackageVersion_(
            package);
    PackageVersion* newesti = r->findNewestInstalledPackageVersion_(
            package);
    if (newest != 0 && newesti != 0) {
        // qDebug() << newest->version.getVersionString() << " " <<
                newesti->version.getVersionString();
        bool canInstall = !newest->isLocked() && !newest->installed() &&
                newest->download.isValid();
        bool canUninstall = !newesti->isLocked() &&
                !newesti->isInWindowsDir();

        // qDebug() << canInstall << " " << canUninstall;

        res = canInstall && canUninstall &&
                newest->version.compare(newesti->version) > 0;
    }
    delete newest;
    delete newesti;

    return res;
}

void MainWindow::updateActions()
{
    updateInstallAction();
    updateUninstallAction();
    updateUpdateAction();
    updateGotoPackageURLAction();
    updateTestDownloadSiteAction();
    updateActionShowDetailsAction();
    updateCloseTabAction();
    updateReloadRepositoriesAction();
    updateScanHardDrivesAction();
}

void MainWindow::updateInstallAction()
{
    Selection* selection = Selection::findCurrent();
    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");
        if (selected.count() > 0) {
            enabled = selected.count() > 0 &&
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                PackageVersion* pv = (PackageVersion*) selected.at(i);

                enabled = enabled &&
                        pv && !pv->isLocked() &&
                        !pv->installed() &&
                        pv->download.isValid();
            }
        } else {
            AbstractRepository* r = AbstractRepository::getDefault_();
            selected = selection->getSelected("Package");
            enabled = selected.count() > 0 &&
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                Package* p = (Package*) selected.at(i);
                PackageVersion* pv  = r->findNewestInstallablePackageVersion_(
                        p->name);

                enabled = enabled &&
                        pv && !pv->isLocked() &&
                        !pv->installed() &&
                        pv->download.isValid();

                delete pv;
            }
        }
    }

    this->ui->actionInstall->setEnabled(enabled);
}

void MainWindow::updateUninstallAction()
{
    // qDebug() << "MainWindow::updateUninstallAction start";

    Selection* selection = Selection::findCurrent();

    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");
        if (selected.count() > 0) {
            enabled = selected.count() > 0 &&
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                PackageVersion* pv = (PackageVersion*) selected.at(i);

                enabled = enabled &&
                        pv && !pv->isLocked() &&
                        pv->installed() && !pv->isInWindowsDir();
            }
            // qDebug() << "MainWindow::updateUninstallAction 2:" << selected.count();
        } else {
            AbstractRepository* r = AbstractRepository::getDefault_();
            QList<void*> selected = selection->getSelected("Package");
            enabled = selected.count() > 0 &&
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                Package* p = (Package*) selected.at(i);
                PackageVersion* pv = r->findNewestInstalledPackageVersion_(
                        p->name);

                enabled = enabled &&
                        pv && !pv->isLocked() &&
                        pv->installed() && !pv->isInWindowsDir();

                delete pv;
            }
        }
    }
    this->ui->actionUninstall->setEnabled(enabled);
    // qDebug() << "MainWindow::updateUninstallAction end " << enabled;
}

void MainWindow::updateUpdateAction()
{
    Selection* selection = Selection::findCurrent();

    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("Package");
        if (selected.count() > 0) {
            enabled =
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                Package* p = (Package*) selected.at(i);

                enabled = enabled && isUpdateEnabled(p->name);
            }
        } else {
            selected = selection->getSelected("PackageVersion");
            enabled = selected.count() >= 1 &&
                    !hardDriveScanRunning && !reloadRepositoriesThreadRunning;
            for (int i = 0; i < selected.count(); i++) {
                if (!enabled)
                    break;

                PackageVersion* pv = (PackageVersion*) selected.at(i);

                enabled = enabled && isUpdateEnabled(pv->package);
            }
        }
    }
    this->ui->actionUpdate->setEnabled(enabled);
}

void MainWindow::updateScanHardDrivesAction()
{
    this->ui->actionScan_Hard_Drives->setEnabled(
            !hardDriveScanRunning && !reloadRepositoriesThreadRunning);
}

void MainWindow::updateReloadRepositoriesAction()
{
    this->ui->actionReload_Repositories->setEnabled(
            !hardDriveScanRunning && !reloadRepositoriesThreadRunning);
}

void MainWindow::updateCloseTabAction()
{
    QWidget* w = this->ui->tabWidget->currentWidget();
    this->ui->actionClose_Tab->setEnabled(
            w != this->mainFrame && w != this->jobsTab);
}

void MainWindow::updateActionShowDetailsAction()
{
    Selection* selection = Selection::findCurrent();
    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");
        if (selected.count() > 0)
            enabled = !reloadRepositoriesThreadRunning;
        else {
            selected = selection->getSelected("Package");
            enabled = !reloadRepositoriesThreadRunning && selected.count() > 0;
        }
    }

    this->ui->actionShow_Details->setEnabled(enabled);
}

void MainWindow::updateTestDownloadSiteAction()
{
    Selection* selection = Selection::findCurrent();

    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");
        if (selected.count() > 0) {
            if (!reloadRepositoriesThreadRunning) {
                for (int i = 0; i < selected.count(); i++) {
                    PackageVersion* pv = (PackageVersion*) selected.at(i);
                    if (pv->download.isValid()) {
                        enabled = true;
                        break;
                    }
                }
            }
        } else {
            AbstractRepository* r = AbstractRepository::getDefault_();
            selected = selection->getSelected("Package");
            if (!reloadRepositoriesThreadRunning) {
                for (int i = 0; i < selected.count(); i++) {
                    Package* p = (Package*) selected.at(i);
                    PackageVersion* pv = r->findNewestInstallablePackageVersion_(
                            p->name);
                    if (pv && pv->download.isValid()) {
                        enabled = true;
                        delete pv;
                        break;
                    }
                    delete pv;
                }
            }
        }
    }

    this->ui->actionTest_Download_Site->setEnabled(enabled);
}

void MainWindow::updateGotoPackageURLAction()
{
    Selection* selection = Selection::findCurrent();
    bool enabled = false;
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");
        if (selected.count() > 0) {
            AbstractRepository* r = Repository::getDefault_();
            if (!reloadRepositoriesThreadRunning) {
                for (int i = 0; i < selected.count(); i++) {
                    PackageVersion* pv = (PackageVersion*) selected.at(i);

                    Package* p = r->findPackage_(pv->package);
                    if (p) {
                        QUrl url(p->url);
                        delete p;

                        if (url.isValid()) {
                            enabled = true;
                            break;
                        }
                    }
                }
            }
        } else {
            selected = selection->getSelected("Package");
            if (!reloadRepositoriesThreadRunning) {
                for (int i = 0; i < selected.count(); i++) {
                    Package* p = (Package*) selected.at(i);

                    if (p) {
                        QUrl url(p->url);
                        if (url.isValid()) {
                            enabled = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    this->ui->actionGotoPackageURL->setEnabled(enabled);
}

void MainWindow::closeDetailTabs()
{
    for (int i = 0; i < this->ui->tabWidget->count(); ) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        PackageFrame* pf = dynamic_cast<PackageFrame*>(w);
        LicenseForm* lf = dynamic_cast<LicenseForm*>(w);
        if (pvf != 0 || lf != 0 || pf != 0) {
            this->ui->tabWidget->removeTab(i);
        } else {
            i++;
        }
    }
}

void MainWindow::recognizeAndLoadRepositories(bool useCache)
{
    QTableWidget* t = this->mainFrame->getTableWidget();
    t->clearContents();
    t->setRowCount(0);

    Job* job = new Job();
    InstallThread* it = new InstallThread(0, 3, job);
    it->useCache = useCache;

    connect(it, SIGNAL(finished()), this,
            SLOT(recognizeAndLoadRepositoriesThreadFinished()),
            Qt::QueuedConnection);

    this->reloadRepositoriesThreadRunning = true;
    updateActions();

    monitor(job, "Initializing", it);
}

void MainWindow::setMenuAccelerators(){
    QMenuBar* mb = this->menuBar();
    QList<QChar> used;
    QStringList titles;
    for (int i = 0; i < mb->children().count(); i++) {
        QMenu* m = dynamic_cast<QMenu*>(mb->children().at(i));
        if (m) {
            titles.append(m->title());
        }
    }
    chooseAccelerators(&titles);
    for (int i = 0, j = 0; i < mb->children().count(); i++) {
        QMenu* m = dynamic_cast<QMenu*>(mb->children().at(i));
        if (m) {
            m->setTitle(titles.at(j));
            j++;
        }
    }
}

void MainWindow::chooseAccelerators(QStringList* titles)
{
    QList<QChar> used;
    for (int i = 0; i < titles->count(); i++) {
        QString title = titles->at(i);

        if (title.contains('&')) {
            int pos = title.indexOf('&');
            if (pos + 1 < title.length()) {
                QChar c = title.at(pos + 1);
                if (c.isLetter()) {
                    if (!used.contains(c))
                        used.append(c);
                    else
                        title.remove(pos, 1);
                }
            }
        }

        if (!title.contains('&')) {
            QString s = title.toUpper();
            int pos = -1;
            for (int j = 0; j < s.length(); j++) {
                QChar c = s.at(j);
                if (c.isLetter() && !used.contains(c)) {
                    pos = j;
                    break;
                }
            }

            if (pos >= 0) {
                QChar c = s.at(pos);
                used.append(c);
                title.insert(pos, '&');
            }
        }

        titles->replace(i, title);
    }
}

void MainWindow::setActionAccelerators(QWidget* w) {
    QStringList titles;
    for (int i = 0; i < w->children().count(); i++) {
        QAction* m = dynamic_cast<QAction*>(w->children().at(i));
        if (m) {
            titles.append(m->text());
        }
    }
    chooseAccelerators(&titles);
    for (int i = 0, j = 0; i < w->children().count(); i++) {
        QAction* m = dynamic_cast<QAction*>(w->children().at(i));
        if (m) {
            m->setText(titles.at(j));
            j++;
        }
    }
}

void MainWindow::recognizeAndLoadRepositoriesThreadFinished()
{
    fillList();

    this->reloadRepositoriesThreadRunning = false;
    updateActions();
}

QList<void*> MainWindow::getSelected(const QString& type) const
{
    QWidget* w = this->ui->tabWidget->currentWidget();
    QList<void*> r;
    if (w) {
        Selection* sel = dynamic_cast<Selection*>(w);
        if (sel)
            r = sel->getSelected(type);
    }
    return r;
}

void MainWindow::openLicense(const QString& name, bool select)
{
    int index = this->findLicenseTab(name);
    if (index < 0) {
        LicenseForm* f = new LicenseForm(this->ui->tabWidget);
        License* lic =
                DBRepository::getDefault()->
                findLicense(name);
        f->fillForm(lic);
        this->ui->tabWidget->addTab(f, lic->title);
        index = this->ui->tabWidget->count() - 1;
    }
    if (select)
        this->ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::openPackageVersion(const QString& package,
        const Version& version, bool select)
{
    int index = this->findPackageVersionTab(package, version);
    if (index < 0) {
        PackageVersionForm* pvf = new PackageVersionForm(
                this->ui->tabWidget);
        PackageVersion* pv_ =
                DBRepository::getDefault()->
                findPackageVersion(package, version);
        if (pv_) {
            pvf->fillForm(pv_);
            QIcon icon = getPackageVersionIcon(package);
            this->ui->tabWidget->addTab(pvf, icon, pv_->toString());
            index = this->ui->tabWidget->count() - 1;
        }
    }
    if (select)
        this->ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::addTab(QWidget* w, const QIcon& icon, const QString& title)
{
    this->ui->tabWidget->addTab(w, icon, title);
    this->ui->tabWidget->setCurrentIndex(this->ui->tabWidget->count() - 1);
}

void MainWindow::on_actionInstall_activated()
{
}

void MainWindow::on_actionGotoPackageURL_triggered()
{
    QSet<QUrl> urls;

    Selection* selection = Selection::findCurrent();
    QList<void*> selected;
    if (selection) {
        selected = selection->getSelected("Package");
        if (selected.count() != 0) {
            for (int i = 0; i < selected.count(); i++) {
                Package* p = (Package*) selected.at(i);
                QUrl url(p->url);
                if (url.isValid())
                    urls.insert(url);
            }
        } else {
            AbstractRepository* r = AbstractRepository::getDefault_();
            selected = selection->getSelected("PackageVersion");
            for (int i = 0; i < selected.count(); i++) {
                PackageVersion* pv = (PackageVersion*) selected.at(i);
                QScopedPointer<Package> p(r->findPackage_(pv->package));
                if (p) {
                    QUrl url(p->url);
                    if (url.isValid())
                        urls.insert(url);
                }
            }
        }
    }

    for (QSet<QUrl>::const_iterator it = urls.begin();
            it != urls.end(); it++) {
        QDesktopServices::openUrl(*it);
    }
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsFrame* d = 0;
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        d = dynamic_cast<SettingsFrame*>(w);
        if (d) {
            break;
        }
    }
    if (d) {
        this->ui->tabWidget->setCurrentWidget(d);
    } else {
        d = new SettingsFrame(this->ui->tabWidget);

        QString err;
        QList<QUrl*> urls = Repository::getRepositoryURLs(&err);
        QStringList list;
        for (int i = 0; i < urls.count(); i++) {
            list.append(urls.at(i)->toString());
        }
        d->setRepositoryURLs(list);
        qDeleteAll(urls);
        urls.clear();

        d->setInstallationDirectory(WPMUtils::getInstallationDirectory());

        this->ui->tabWidget->addTab(d, "Settings");
        this->ui->tabWidget->setCurrentIndex(this->ui->tabWidget->count() - 1);
    }
}

void MainWindow::on_actionUpdate_triggered()
{
    Selection* sel = Selection::findCurrent();
    if (sel) {
        Repository* r = Repository::getDefault();
        QList<void*> selected = sel->getSelected("PackageVersion");
        QList<Package*> packages;
        if (selected.count() > 0) {
            for (int i = 0; i < selected.count(); i++) {
                PackageVersion* pv = (PackageVersion*) selected.at(i);
                Package* p = r->findPackage(pv->package);

                // multiple versions of the same package could be selected in the table,
                // but only one should be updated
                if (p != 0 && !packages.contains(p)) {
                    packages.append(p);
                }
            }
        } else {
            selected = sel->getSelected("Package");
            for (int i = 0; i < selected.count(); i++) {
                Package* p = (Package*) selected.at(i);
                packages.append(p);
            }
        }

        QList<InstallOperation*> ops;
        QString err = r->planUpdates(packages, ops);

        if (err.isEmpty()) {
            process(ops);
        } else
            addErrorMessage(err, err, true, QMessageBox::Critical);
    }
}

void MainWindow::on_actionTest_Download_Site_triggered()
{
    Selection* sel = Selection::findCurrent();
    if (sel) {
        QSet<QString> urls;

        QList<void*> selected = sel->getSelected("PackageVersion");
        if (selected.count() > 0) {
            for (int i = 0; i < selected.count(); i++) {
                PackageVersion* pv = (PackageVersion*) selected.at(i);
                urls.insert(pv->download.host());
            }
        } else {
            AbstractRepository* r = AbstractRepository::getDefault_();
            selected = sel->getSelected("Package");
            for (int i = 0; i < selected.count(); i++) {
                Package* p = (Package*) selected.at(i);
                QScopedPointer<PackageVersion> pv(
                        r->findNewestInstallablePackageVersion_(p->name));
                if (pv) {
                    urls.insert(pv->download.host());
                }
            }
        }

        for (QSet<QString>::const_iterator it = urls.begin();
                it != urls.end(); it++) {
            QString s = "http://www.urlvoid.com/scan/" + *it;
            QUrl url(s);
            if (url.isValid())
                QDesktopServices::openUrl(url);
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    addTextTab("About", QString(
            "<html><body>Npackd %1 - software package manager for Windows (R)<br>"
            "<a href='http://code.google.com/p/windows-package-manager'>"
            "http://code.google.com/p/windows-package-manager</a></body></html>").
            arg(WPMUtils::NPACKD_VERSION), true);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    QWidget* w = this->ui->tabWidget->widget(index);
    if (w != this->mainFrame && w != this->jobsTab) {
        this->ui->tabWidget->removeTab(index);
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    updateActions();
}

void MainWindow::addTextTab(const QString& title, const QString& text,
        bool html)
{
    QWidget* w;
    if (html) {
        QTextBrowser* te = new QTextBrowser(this->ui->tabWidget);
        te->setReadOnly(true);
        te->setHtml(text);
        te->setOpenExternalLinks(true);
        w = te;
    } else {
        QTextEdit* te = new QTextEdit(this->ui->tabWidget);
        te->setReadOnly(true);
        te->setText(text);
        w = te;
    }
    this->ui->tabWidget->addTab(w, title);
    this->ui->tabWidget->setCurrentIndex(this->ui->tabWidget->count() - 1);
}

void MainWindow::addJobsTab()
{
    QScrollArea* jobsScrollArea = new QScrollArea(this->ui->tabWidget);
    jobsScrollArea->setFrameStyle(0);

    progressContent = new QFrame(jobsScrollArea);
    QVBoxLayout* layout = new QVBoxLayout();

    QSizePolicy sp;
    sp.setVerticalPolicy(QSizePolicy::Preferred);
    sp.setHorizontalPolicy(QSizePolicy::Ignored);
    sp.setHorizontalStretch(100);
    progressContent->setSizePolicy(sp);

    layout->addStretch(100);
    progressContent->setLayout(layout);
    jobsScrollArea->setWidget(progressContent);
    jobsScrollArea->setWidgetResizable(true);

    int index = this->ui->tabWidget->addTab(jobsScrollArea, "Jobs");
    this->jobsTab = this->ui->tabWidget->widget(index);
    updateProgressTabTitle();
}

void MainWindow::on_actionShow_Details_triggered()
{
    showDetails();
}

void MainWindow::on_actionScan_Hard_Drives_triggered()
{
    PackageVersion* locked = PackageVersion::findLockedPackageVersion();
    if (locked) {
        QString msg("Cannot start the scan now. "
                "The package %1 is locked by a "
                "currently running installation/removal.");
        this->addErrorMessage(msg.arg(locked->toString()));
        delete locked;
        return;
    }

    Job* job = new Job();
    ScanHardDrivesThread* it = new ScanHardDrivesThread(job);

    connect(it, SIGNAL(finished()), this,
            SLOT(hardDriveScanThreadFinished()),
            Qt::QueuedConnection);

    this->hardDriveScanRunning = true;
    this->updateActions();

    monitor(job, "Install/Uninstall", it);
}

void MainWindow::hardDriveScanThreadFinished()
{
    QStringList detected;
    ScanHardDrivesThread* it = (ScanHardDrivesThread*) this->sender();
    for (int i = 0; i < it->detected.count(); i++) {
        PackageVersion* pv = it->detected.at(i);
        detected.append(pv->toString());
    }

    detected.append("____________________");
    detected.append(QString("%1 package(s) detected").
            arg(it->detected.count()));

    fillList();

    addTextTab("Package detection status", detected.join("\n"));

    this->hardDriveScanRunning = false;
    this->updateActions();
}

void MainWindow::addErrorMessage(const QString& msg, const QString& details,
        bool autoHide, QMessageBox::Icon icon)
{
    MessageFrame* label = new MessageFrame(this->centralWidget(), msg,
            details, autoHide ? 30 : 0, icon);
    QVBoxLayout* layout = (QVBoxLayout*) this->centralWidget()->layout();
    layout->insertWidget(0, label);
}

void MainWindow::on_actionReload_Repositories_triggered()
{
    PackageVersion* locked = PackageVersion::findLockedPackageVersion();
    if (locked) {
        QString msg("Cannot reload the repositories now. "
                "The package %1 is locked by a "
                "currently running installation/removal.");
        this->addErrorMessage(msg.arg(locked->toString()));
        delete locked;
    } else {
        closeDetailTabs();
        recognizeAndLoadRepositories(false);
    }
}

void MainWindow::on_actionClose_Tab_triggered()
{
    QWidget* w = this->ui->tabWidget->currentWidget();
    if (w != this->mainFrame && w != this->jobsTab) {
        this->ui->tabWidget->removeTab(this->ui->tabWidget->currentIndex());
    }
}

void MainWindow::updateActionsSlot()
{
    updateActions();
}

void MainWindow::on_actionFile_an_Issue_triggered()
{
    QDesktopServices::openUrl(QUrl(
            "http://code.google.com/p/windows-package-manager/issues/entry?template=Defect%20report%20from%20user"));
}

void MainWindow::on_actionInstall_triggered()
{
    Selection* selection = Selection::findCurrent();
    if (selection) {
        QList<void*> selected = selection->getSelected("PackageVersion");

        if (selected.count() == 0) {
            Repository* r = Repository::getDefault();
            selected = selection->getSelected("Package");
            for (int i = 0; i < selected.count(); ) {
                Package* p = (Package*) selected.at(i);
                PackageVersion* pv = r->findNewestInstallablePackageVersion(
                        p->name);
                if (pv) {
                    selected.replace(i, pv);
                    i++;
                } else
                    selected.removeAt(i);
            }
        }

        QList<InstallOperation*> ops;
        QList<PackageVersion*> installed =
                Repository::getDefault()->getInstalled();
        QList<PackageVersion*> avoid;

        QString err;
        for (int i = 0; i < selected.count(); i++) {
            PackageVersion* pv = (PackageVersion*) selected.at(i);

            avoid.clear();
            err = pv->planInstallation(installed, ops, avoid);
            if (!err.isEmpty())
                break;
        }

        if (err.isEmpty())
            process(ops);
        else
            addErrorMessage(err, err, true, QMessageBox::Critical);
    }
}

void MainWindow::on_actionUninstall_triggered()
{
    Selection* selection = Selection::findCurrent();
    QList<void*> selected;
    if (selection)
        selected = selection->getSelected("PackageVersion");

    if (selected.count() == 0) {
        Repository* r = Repository::getDefault();
        selected = selection->getSelected("Package");
        for (int i = 0; i < selected.count(); ) {
            Package* p = (Package*) selected.at(i);
            PackageVersion* pv = r->findNewestInstalledPackageVersion(p->name);
            if (pv) {
                selected.replace(i, pv);
                i++;
            } else
                selected.removeAt(i);
        }
    }

    QList<InstallOperation*> ops;
    QList<PackageVersion*> installed = Repository::getDefault()->getInstalled();

    QString err;
    for (int i = 0; i < selected.count(); i++) {
        PackageVersion* pv = (PackageVersion*) selected.at(i);
        err = pv->planUninstallation(installed, ops);
        if (!err.isEmpty())
            break;
    }

    if (err.isEmpty())
        process(ops);
    else
        addErrorMessage(err, err, true, QMessageBox::Critical);
}
