#include <math.h>

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
#include <qdatetime.h>
#include "qdesktopservices.h"
#include <qinputdialog.h>
#include <qfiledialog.h>
#include <qtextstream.h>
#include <qiodevice.h>
#include <qmenu.h>
#include <qtextedit.h>
#include <qscrollarea.h>
#include <QPushButton>
#include <QCloseEvent>
#include <QTextBrowser>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "repository.h"
#include "job.h"
#include "progressdialog.h"
#include "settingsdialog.h"
#include "wpmutils.h"
#include "installoperation.h"
#include "downloader.h"
#include "packageversionform.h"
#include "uiutils.h"
#include "progressframe.h"
#include "messageframe.h"

extern HWND defaultPasswordWindow;

QMap<QString, QIcon> MainWindow::icons;
QIcon MainWindow::genericAppIcon;
MainWindow* MainWindow::instance = 0;

class InstallThread: public QThread
{
    PackageVersion* pv;

    // 0, 1, 2 = install/uninstall
    // 3, 4 = recognize installed applications + load repositories
    // 7 = download all files
    // 8 = scan hard drives
    int type;

    Job* job;

    void testRepositories();
    void downloadAllFiles();
    void testOnePackage(Job* job, PackageVersion* pv);
    void scanHardDrives();
public:
    // name of the log file for type=6
    // directory for type=7
    QString logFile;

    QList<InstallOperation*> install;

    InstallThread(PackageVersion* pv, int type, Job* job);

    void run();
};

InstallThread::InstallThread(PackageVersion *pv, int type, Job* job)
{
    this->pv = pv;
    this->type = type;
    this->job = job;
}

void InstallThread::downloadAllFiles()
{
    QFile f(this->logFile + "\\Download.log");
    QTextStream ts(&f);
    Repository* r = Repository::getDefault();
    double step = 1.0 / r->packageVersions.count();
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);

        if (job->isCancelled() || !job->getErrorMessage().isEmpty())
            break;

        if (pv->download.isValid()) {
            job->setHint(QString("Downloading %1").arg(pv->toString()));
            Job* stepJob = job->newSubJob(step);
            QString to = this->logFile + "\\" + pv->package + "-" +
                         pv->version.getVersionString() + pv->getFileExtension();
            pv->downloadTo(stepJob, to);
            if (!stepJob->getErrorMessage().isEmpty())
                ts << QString("Error downloading %1: %2").
                        arg(pv->toString()).
                        arg(stepJob->getErrorMessage()) << endl;
            delete stepJob;
        }
    }
    f.close();
    job->complete();
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
        Repository* r = Repository::getDefault();
        r->reload(job);
        PackageVersion* pv = r->findOrCreatePackageVersion(
                "com.googlecode.windows-package-manager.Npackd",
                Version(WPMUtils::NPACKD_VERSION));
        if (!pv->installed()) {
            pv->setPath(WPMUtils::getExeDir());
            pv->setExternal(true);
        }
        break;
    }
    case 7:
        downloadAllFiles();
        break;
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
    PackageVersion* pv = getSelectedPackageVersion();
    if (pv) {
        PackageVersionForm* pvf = new PackageVersionForm(this->ui->tabWidget);
        pvf->fillForm(pv);
        QIcon icon = getPackageVersionIcon(pv);
        this->ui->tabWidget->addTab(pvf, icon, pv->toString());
        this->ui->tabWidget->setCurrentIndex(this->ui->tabWidget->count() - 1);
    }
}

void MainWindow::updateIcons()
{
    Repository* r = Repository::getDefault();
    for (int i = 0; i < this->ui->tableWidget->rowCount(); i++) {
        QTableWidgetItem *item = ui->tableWidget->item(i, 0);
        const QVariant v = item->data(Qt::UserRole);
        PackageVersion* pv = (PackageVersion *) v.value<void*>();
        Package* p = r->findPackage(pv->package);

        if (p) {
            if (!p->icon.isEmpty() && this->icons.contains(p->icon)) {
                QIcon icon = this->icons[p->icon];
                item->setIcon(icon);
            }
        }
    }

    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        if (pvf) {
            pvf->updateIcons();
            QIcon icon = getPackageVersionIcon(pvf->pv);
            this->ui->tabWidget->setTabIcon(i, icon);
        }
    }
}

void MainWindow::updateStatusInDetailTabs()
{
    for (int i = 0; i < this->ui->tabWidget->count(); i++) {
        QWidget* w = this->ui->tabWidget->widget(i);
        PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
        if (pvf) {
            pvf->updateStatus();
        }
    }
}

QIcon MainWindow::getPackageVersionIcon(PackageVersion *pv)
{
    Repository* r = Repository::getDefault();
    Package* p = r->findPackage(pv->package);

    QIcon icon = MainWindow::genericAppIcon;
    if (p) {
        if (!p->icon.isEmpty() && MainWindow::icons.contains(p->icon)) {
            icon = MainWindow::icons[p->icon];
        }
    }
    return icon;
}

MainWindow* MainWindow::getInstance()
{
    return instance;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->runningJobsMutex.lock();
    int n = this->runningJobs.count();
    this->runningJobsMutex.unlock();

    if (n == 0) {
        event->accept();
    } else {
        addErrorMessage("Cannot exit while jobs are running");
        event->ignore();
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    instance = this;

    ui->setupUi(this);

    this->taskbarMessageId = 0;

    this->monitoredJobLastChanged = 0;
    this->progressContent = 0;
    this->jobsTab = 0;
    this->taskbarInterface = 0;

    setWindowTitle("Npackd");

    this->genericAppIcon = QIcon(":/images/app.png");

    this->ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);

    QList<QUrl*> urls = Repository::getRepositoryURLs();
    if (urls.count() == 0) {
        urls.append(new QUrl(
                "https://windows-package-manager.googlecode.com/hg/repository/Rep.xml"));
        if (WPMUtils::is64BitWindows())
            urls.append(new QUrl(
                    "https://windows-package-manager.googlecode.com/hg/repository/Rep64.xml"));
        Repository::setRepositoryURLs(urls);
    }
    qDeleteAll(urls);
    urls.clear();

    //this->ui->formLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);

    this->ui->tabWidget->setTabText(0, "Packages");

    this->on_tableWidget_itemSelectionChanged();
    this->ui->tableWidget->setColumnCount(6);
    this->ui->tableWidget->setColumnWidth(0, 40);
    this->ui->tableWidget->setColumnWidth(1, 150);
    this->ui->tableWidget->setColumnWidth(2, 300);
    this->ui->tableWidget->setColumnWidth(3, 100);
    this->ui->tableWidget->setColumnWidth(4, 100);
    this->ui->tableWidget->setColumnWidth(5, 100);
    this->ui->tableWidget->setIconSize(QSize(32, 32));
    this->ui->tableWidget->sortItems(1);
    this->ui->tableWidget->addAction(this->ui->actionInstall);
    this->ui->tableWidget->addAction(this->ui->actionUninstall);
    this->ui->tableWidget->addAction(this->ui->actionUpdate);
    this->ui->tableWidget->addAction(this->ui->actionShow_Details);
    this->ui->tableWidget->addAction(this->ui->actionGotoPackageURL);
    this->ui->tableWidget->addAction(this->ui->actionTest_Download_Site);

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

    this->ui->lineEditText->setFocus();

    this->addJobsTab();

    Repository* r = Repository::getDefault();
    connect(r, SIGNAL(statusChanged(PackageVersion*)), this,
            SLOT(repositoryStatusChanged(PackageVersion*)),
            Qt::QueuedConnection);

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

void MainWindow::repositoryStatusChanged(PackageVersion* pv)
{
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
    for (int i = 0; i < this->ui->tableWidget->rowCount(); i++) {
        QTableWidgetItem* newItem = this->ui->tableWidget->item(i, 4);

        const QVariant v = newItem->data(Qt::UserRole);
        PackageVersion* pv = (PackageVersion *) v.value<void*>();

        QString status = pv->getStatus();
        newItem->setText(status);
        if (status.contains("obsolete") || status.contains("updateable"))
            newItem->setBackgroundColor(QColor(255, 0xc7, 0xc7));
        else
            newItem->setBackgroundColor(QColor(255, 255, 255));
    }
}

void MainWindow::updateProgressTabTitle()
{
    this->runningJobsMutex.lock();
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
    QTime rest = WPMUtils::durationToTime(max);

    this->runningJobsMutex.unlock();

    QString title;
    if (n == 0)
        title = QString("0 Jobs");
    else if (n == 1)
        title = QString("1 Job (%2)").arg(rest.toString());
    else
        title = QString("%1 Jobs (%2)").arg(n).arg(rest.toString());

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
    delete ui;
}

void MainWindow::monitoredJobChanged(const JobState& state)
{
    time_t now;
    time(&now);

    if (now != this->monitoredJobLastChanged) {
        this->monitoredJobLastChanged = now;

        this->runningJobsMutex.lock();
        int index = this->runningJobs.indexOf(state.job);
        if (index >= 0) {
            this->runningJobStates.replace(index, state);
        }
        this->runningJobsMutex.unlock();

        updateProgressTabTitle();
    }
}

void MainWindow::monitor(Job* job, const QString& title, QThread* thread)
{
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(monitoredJobChanged(const JobState&)),
            Qt::QueuedConnection);

    this->runningJobsMutex.lock();
    this->runningJobs.append(job);
    this->runningJobStates.append(JobState());
    this->runningJobsMutex.unlock();

    updateProgressTabTitle();

    ProgressFrame* pf = new ProgressFrame(progressContent, job, title,
            thread);
    pf->resize(100, 100);
    QVBoxLayout* layout = (QVBoxLayout*) progressContent->layout();
    layout->insertWidget(0, pf);

    progressContent->resize(500, 500);
}

bool MainWindow::waitFor(Job* job, const QString& title)
{
    ProgressDialog* pd;
    pd = new ProgressDialog(this, job, title);
    pd->setModal(true);

    defaultPasswordWindow = pd->winId();
    // qDebug() << "MainWindow::waitFor.1";

    pd->exec();
    delete pd;
    defaultPasswordWindow = this->winId();

    // qDebug() << "MainWindow::waitFor.2";
    bool r = false;
    if (job->isCancelled())
        r = false;
    else if (!job->getErrorMessage().isEmpty()) {
        QString first = job->getErrorMessage().trimmed();
        int ind = first.indexOf("\n");
        if (ind >= 0)
            first = first.left(ind);
        ind = first.indexOf("\r");
        if (ind >= 0)
            first = first.left(ind);

        addErrorMessage(QString("%1: %2").arg(job->getHint()).arg(first),
                job->getErrorMessage());

        r = false;
    } else {
        r = true;
    }


    return r;
}

void MainWindow::onShow()
{
    recognizeAndLoadRepositories();
}

void MainWindow::selectPackageVersion(PackageVersion* pv)
{
    for (int i = 0; i < this->ui->tableWidget->rowCount(); i++) {
        const QVariant v = this->ui->tableWidget->item(i, 0)->
                data(Qt::UserRole);
        PackageVersion* f = (PackageVersion *) v.value<void*>();
        if (f == pv) {
            this->ui->tableWidget->selectRow(i);
            break;
        }
    }
}

PackageVersion* MainWindow::getSelectedPackageVersionInTable()
{
    QList<QTableWidgetItem*> sel = this->ui->tableWidget->selectedItems();
    if (sel.count() > 0) {
        const QVariant v = sel.at(0)->data(Qt::UserRole);
        PackageVersion* pv = (PackageVersion *) v.value<void*>();
        return pv;
    }
    return 0;
}

PackageVersion* MainWindow::getSelectedPackageVersion()
{
    QWidget* w = this->ui->tabWidget->widget(this->ui->tabWidget->
            currentIndex());
    PackageVersionForm* pvf = dynamic_cast<PackageVersionForm*>(w);
    if (pvf) {
        return pvf->pv;
    } else if (w == this->ui->tab){
        return getSelectedPackageVersionInTable();
    } else {
        return 0;
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
    // qDebug() << "MainWindow::fillList";
    QTableWidget* t = this->ui->tableWidget;

    t->clearContents();
    t->setSortingEnabled(false);

    Repository* r = Repository::getDefault();

    t->setColumnCount(6);

    QTableWidgetItem *newItem = new QTableWidgetItem("Icon");
    t->setHorizontalHeaderItem(0, newItem);

    newItem = new QTableWidgetItem("Title");
    t->setHorizontalHeaderItem(1, newItem);

    newItem = new QTableWidgetItem("Description");
    t->setHorizontalHeaderItem(2, newItem);

    newItem = new QTableWidgetItem("Version");
    t->setHorizontalHeaderItem(3, newItem);

    newItem = new QTableWidgetItem("Status");
    t->setHorizontalHeaderItem(4, newItem);

    newItem = new QTableWidgetItem("License");
    t->setHorizontalHeaderItem(5, newItem);

    int statusFilter = this->ui->comboBoxStatus->currentIndex();
    QStringList textFilter =
            this->ui->lineEditText->text().toLower().simplified().split(" ");

    t->setRowCount(r->packageVersions.count());

    int n = 0;
    for (int i = 0; i < r->packageVersions.count(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);

        // filter by text
        if (textFilter.count() > 0) {
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
        }

        bool installed = pv->installed();
        bool updateEnabled = isUpdateEnabled(pv);
        PackageVersion* newest = r->findNewestInstallablePackageVersion(
                pv->package);
        bool statusOK;
        switch (statusFilter) {
            case 0:
                // all
                statusOK = true;
                break;
            case 1:
                // not installed
                statusOK = !installed;
                break;
            case 2:
                // installed
                statusOK = installed;
                break;
            case 3:
                // installed, updateable
                statusOK = installed && updateEnabled;
                break;
            case 4:
                // newest or installed
                statusOK = installed || pv == newest;
                break;
            default:
                statusOK = true;
                break;
        }
        if (!statusOK)
            continue;

        Package* p = r->findPackage(pv->package);

        newItem = new QTableWidgetItem("");
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
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

        QString packageTitle;
        if (p)
            packageTitle = p->title;
        else
            packageTitle = pv->package;
        newItem = new QCITableWidgetItem(packageTitle);
        newItem->setStatusTip(pv->download.toString() + " " + pv->package +
                " " + pv->sha1);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 1, newItem);

        QString desc;
        if (p)
            desc = p->description;
        newItem = new QCITableWidgetItem(desc);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 2, newItem);

        newItem = new QTableWidgetItem(pv->version.getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 3, newItem);

        newItem = new QCITableWidgetItem("");
        QString status = pv->getStatus();
        newItem->setText(status);
        if (status.contains("obsolete") || status.contains("updateable"))
            newItem->setBackgroundColor(QColor(255, 0xc7, 0xc7));
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 4, newItem);

        newItem = new QCITableWidgetItem("");
        QString licenseTitle;
        if (p) {
            License* lic = r->findLicense(p->license);
            if (lic)
                licenseTitle = lic->title;
        }
        newItem->setText(licenseTitle);
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(n, 5, newItem);

        n++;
    }
    t->setRowCount(n);
    t->setSortingEnabled(true);
    // qDebug() << "MainWindow::fillList.2";
}

void MainWindow::process(QList<InstallOperation*> &install)
{
    // reoder the operations if a package is updated. In this case it is better
    // to uninstall the old first and then install the new one.
    if (install.size() == 2) {
        InstallOperation* first = install.at(0);
        InstallOperation* second = install.at(1);
        if (first->packageVersion->package == second->packageVersion->package &&
                first->install && !second->install) {
            install.insert(0, second);
            install.removeAt(2);
        }
    }

    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        PackageVersion* pv = op->packageVersion;
        if (pv->isLocked()) {
            QString msg("The package %1 is locked by a "
                    "currently running installation/removal.");
            this->addErrorMessage(msg.arg(pv->toString()),
                    msg.arg(pv->toString()), true, QMessageBox::Critical);
            return;
        }
    }

    QStringList locked = WPMUtils::getProcessFiles();
    QStringList lockedUninstall;
    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        if (!op->install) {
            PackageVersion* pv = op->packageVersion;
            QString path = pv->getPath();
            for (int i = 0; i < locked.size(); i++) {
                if (WPMUtils::isUnder(locked.at(i), path)) {
                    lockedUninstall.append(locked.at(i));
                }
            }
        }
    }

    if (lockedUninstall.size() > 0) {
        QString locked_ = lockedUninstall.join(", \n");
        QString msg = QString("The package(s) cannot be uninstalled because "
                "the following files are in use "
                "(please close the corresponding applications): "
                "%1").arg(locked_);
        addErrorMessage(msg, msg, true, QMessageBox::Critical);
        return;
    }

    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (!op->install) {
            PackageVersion* pv = op->packageVersion;
            if (pv->isDirectoryLocked()) {
                QString msg = QString("The package %1 cannot be uninstalled because "
                        "some files or directories under %2 are in use.").
                        arg(pv->toString()).
                        arg(pv->getPath());
                addErrorMessage(msg, msg, true, QMessageBox::Critical);
                return;
            }
        }
    }

    QString names;
    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (!op->install) {
            PackageVersion* pv = op->packageVersion;
            if (!names.isEmpty())
                names.append(", ");
            names.append(pv->toString());
        }
    }
    QString installNames;
    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        if (op->install) {
            PackageVersion* pv = op->packageVersion;
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
    if (installCount == 1 && uninstallCount == 0) {
        b = true;
    } else if (installCount == 0 && uninstallCount == 1) {
        msg = QString("The package %1 will be uninstalled. "
                "The corresponding directory %2 "
                "will be completely deleted. "
                "There is no way to restore the files.").
                arg(install.at(0)->packageVersion->toString()).
                arg(install.at(0)->packageVersion->getPath());
        b = UIUtils::confirm(this, "Uninstall", msg);
    } else if (installCount > 0 && uninstallCount == 0) {
        msg = QString("%1 package(s) will be installed: %2").
                arg(installCount).arg(installNames);
        b = UIUtils::confirm(this, "Install", msg);
    } else if (installCount == 0 && uninstallCount > 0) {
        msg = QString("%1 package(s) will be uninstalled: %2. "
                "The corresponding directories "
                "will be completely deleted. "
                "There is no way to restore the files.").
                arg(uninstallCount).arg(names);
        b = UIUtils::confirm(this, "Uninstall", msg);
    } else {
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

        connect(it, SIGNAL(finished()), this,
                SLOT(processThreadFinished()),
                Qt::QueuedConnection);

        monitor(job, "Install/Uninstall", it);
    }
}

void MainWindow::processThreadFinished()
{
    PackageVersion* sel = getSelectedPackageVersionInTable();
    fillList();
    updateStatusInDetailTabs();
    selectPackageVersion(sel);
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
    this->runningJobsMutex.lock();
    int n = this->runningJobs.count();
    this->runningJobsMutex.unlock();

    if (n > 0)
        addErrorMessage("Cannot exit while jobs are running");
    else
        this->close();
}

void MainWindow::unregisterJob(Job *job)
{
    this->runningJobsMutex.lock();
    int index = this->runningJobs.indexOf(job);
    if (index >= 0) {
        this->runningJobs.removeAt(index);
        this->runningJobStates.removeAt(index);
    }
    this->runningJobsMutex.unlock();

    this->updateProgressTabTitle();
}

void MainWindow::on_actionUninstall_activated()
{
    PackageVersion* pv = getSelectedPackageVersion();
    QList<InstallOperation*> ops;
    QList<PackageVersion*> installed = Repository::getDefault()->getInstalled();
    QString err = pv->planUninstallation(installed, ops);
    if (err.isEmpty())
        process(ops);
    else
        addErrorMessage(err, err, true, QMessageBox::Critical);
}

bool MainWindow::isUpdateEnabled(PackageVersion* pv)
{
    if (pv) {
        Repository* r = Repository::getDefault();
        PackageVersion* newest = r->findNewestInstallablePackageVersion(
                pv->package);
        PackageVersion* newesti = r->findNewestInstalledPackageVersion(
                pv->package);
        if (newest != 0 && newesti != 0) {
            bool canInstall = !newest->isLocked() && !newest->installed() &&
                    newest->download.isValid();
            bool canUninstall = !newesti->isLocked() && !newesti->isExternal();

            return canInstall && canUninstall &&
                    newest->version.compare(newesti->version) > 0;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void MainWindow::updateActions()
{
    PackageVersion* pv = getSelectedPackageVersion();

    this->ui->actionInstall->setEnabled(pv && !pv->isLocked() &&
            !pv->installed() &&
            pv->download.isValid());
    this->ui->actionUninstall->setEnabled(pv && !pv->isLocked() &&
            pv->installed() &&
            !pv->isExternal());

    // "Update"
    this->ui->actionUpdate->setEnabled(isUpdateEnabled(pv));

    // enable "Go To Package Page"
    Package* p;
    if (pv)
        p = Repository::getDefault()->findPackage(pv->package);
    else
        p = 0;
    this->ui->actionGotoPackageURL->setEnabled(pv && p &&
            QUrl(p->url).isValid());

    this->ui->actionTest_Download_Site->setEnabled(pv &&
            pv->download.isValid());

    this->ui->actionShow_Details->setEnabled(pv);

    QWidget* w = this->ui->tabWidget->currentWidget();
    this->ui->actionClose_Tab->setEnabled(
            w != this->ui->tab && w != this->jobsTab);
}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    updateActions();
}

void MainWindow::closeDetailTabs()
{
    for (int i = 0; i < this->ui->tabWidget->count(); ) {
        QWidget* w = this->ui->tabWidget->widget(i);
        if (w != this->ui->tab && w != this->jobsTab) {
            this->ui->tabWidget->removeTab(i);
        } else {
            i++;
        }
    }
}

void MainWindow::recognizeAndLoadRepositories()
{
    QTableWidget* t = this->ui->tableWidget;
    t->clearContents();
    t->setRowCount(0);

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

    Repository* r = Repository::getDefault();
    for (int i = 0; i < r->packages.count(); i++) {
        Package* p = r->packages.at(i);
        if (!p->icon.isEmpty()) {
            FileLoaderItem it;
            it.url = p->icon;
            // qDebug() << "MainWindow::loadRepository " << it.url;
            this->fileLoader.work.append(it);
        }
    }
    // qDebug() << "MainWindow::loadRepository";
}

void MainWindow::on_actionInstall_activated()
{
    PackageVersion* pv = getSelectedPackageVersion();

    QList<PackageVersion*> r;
    QList<InstallOperation*> ops;
    QList<PackageVersion*> installed =
            Repository::getDefault()->getInstalled();
    QList<PackageVersion*> avoid;
    QString err = pv->planInstallation(installed, ops, avoid);
    if (err.isEmpty())
        process(ops);
    else
        addErrorMessage(err, err, true, QMessageBox::Critical);
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
    Repository* r = Repository::getDefault();

    for (int i = 0; i < r->packageVersions.size(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        if (pv->isLocked()) {
            QString msg("Cannot change settings now. "
                    "The package %1 is locked by a "
                    "currently running installation/removal.");
            this->addErrorMessage(msg.arg(pv->toString()));
            return;
        }
    }

    SettingsDialog d;

    QList<QUrl*> urls = Repository::getRepositoryURLs();
    QStringList list;
    for (int i = 0; i < urls.count(); i++) {
        list.append(urls.at(i)->toString());
    }
    d.setRepositoryURLs(list);
    qDeleteAll(urls);
    urls.clear();

    d.setInstallationDirectory(WPMUtils::getInstallationDirectory());

    if (d.exec() == QDialog::Accepted) {
        list = d.getRepositoryURLs();
        if (list.count() == 0) {
            QString msg("No repositories defined");
            addErrorMessage(msg, msg, true, QMessageBox::Critical);
        } else if (d.getInstallationDirectory().isEmpty()) {
            QString msg("The installation directory cannot be empty");
            addErrorMessage(msg, msg, true, QMessageBox::Critical);
        } else if (!QDir(d.getInstallationDirectory()).exists()) {
            QString msg("The installation directory does not exist");
            addErrorMessage(msg, msg, true, QMessageBox::Critical);
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
                WPMUtils::setInstallationDirectory(d.getInstallationDirectory());
                Repository::setRepositoryURLs(urls);
                closeDetailTabs();
                recognizeAndLoadRepositories();
            } else {
                addErrorMessage(err, err, true, QMessageBox::Critical);
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
    PackageVersion* newest = r->findNewestInstallablePackageVersion(pv->package);
    PackageVersion* newesti = r->findNewestInstalledPackageVersion(pv->package);

    QList<InstallOperation*> ops;
    QList<PackageVersion*> installed =
            Repository::getDefault()->getInstalled();
    QList<PackageVersion*> avoid;
    QString err = newest->planInstallation(installed, ops, avoid);
    if (err.isEmpty())
        err = newesti->planUninstallation(installed, ops);

    if (err.isEmpty()) {
        InstallOperation::simplify(ops);
        process(ops);
    } else
        addErrorMessage(err, err, true, QMessageBox::Critical);
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

void MainWindow::on_actionAbout_triggered()
{
    addTextTab("About", QString(
            "<html><body>Npackd %1 - software package manager for Windows (R)<br>"
            "<a href='http://code.google.com/p/windows-package-manager'>"
            "http://code.google.com/p/windows-package-manager</a></body></html>").
            arg(WPMUtils::NPACKD_VERSION), true);
}

void MainWindow::on_tableWidget_doubleClicked(QModelIndex index)
{
    showDetails();
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    QWidget* w = this->ui->tabWidget->widget(index);
    if (w != this->ui->tab && w != this->jobsTab) {
        this->ui->tabWidget->removeTab(index);
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    updateActions();
}

void MainWindow::on_actionList_Installed_MSI_Products_triggered()
{
    QStringList sl = WPMUtils::findInstalledMSIProductNames();

    addTextTab("Installed MSI Products", sl.join("\n"));
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

void MainWindow::on_actionDownload_All_Files_triggered()
{
    QString fn = QFileDialog::getExistingDirectory(this,
            "Download All Files", ".", QFileDialog::ShowDirsOnly);
    if (!fn.isEmpty()) {
        QDir d(fn);
        QFileInfoList entries = d.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::AllEntries | QDir::System);
        if (entries.size() != 0) {
            QString msg("The chosen directory is not empty");
            addErrorMessage(msg, msg, true, QMessageBox::Critical);
        } else {
            Job* job = new Job();
            InstallThread* it = new InstallThread(0, 7, job);
            it->logFile = fn;
            it->start();
            it->setPriority(QThread::LowestPriority);

            waitFor(job, "Download All Files");
            it->wait();
            delete it;
            delete job;
        }
    }
}


void MainWindow::on_actionShow_Details_triggered()
{
    showDetails();
}

void MainWindow::on_actionScan_Hard_Drives_triggered()
{
    Repository* r = Repository::getDefault();

    for (int i = 0; i < r->packageVersions.size(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        if (pv->isLocked()) {
            QString msg("Cannot start the scan now. "
                    "The package %1 is locked by a "
                    "currently running installation/removal.");
            this->addErrorMessage(msg.arg(pv->toString()));
            return;
        }
    }

    Job* job = new Job();
    ScanHardDrivesThread* it = new ScanHardDrivesThread(job);
    it->start();
    it->setPriority(QThread::LowestPriority);

    waitFor(job, "Scan Hard Drives");
    it->wait();
    QStringList detected;
    for (int i = 0; i < it->detected.count(); i++) {
        PackageVersion* pv = it->detected.at(i);
        detected.append(pv->toString());
    }
    detected.append("____________________");
    detected.append(QString("%1 package(s) detected").
            arg(it->detected.count()));
    delete it;
    delete job;

    fillList();

    addTextTab("Package detection status", detected.join("\n"));
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
    Repository* r = Repository::getDefault();
    PackageVersion* locked = r->findLockedPackageVersion();
    if (locked) {
        QString msg("Cannot reload the repositories now. "
                "The package %1 is locked by a "
                "currently running installation/removal.");
        this->addErrorMessage(msg.arg(locked->toString()));
    } else {
        closeDetailTabs();
        recognizeAndLoadRepositories();
    }
}

void MainWindow::on_actionClose_Tab_triggered()
{
    QWidget* w = this->ui->tabWidget->currentWidget();
    if (w != this->ui->tab && w != this->jobsTab) {
        this->ui->tabWidget->removeTab(this->ui->tabWidget->currentIndex());
    }
}
