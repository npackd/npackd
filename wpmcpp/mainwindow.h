#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>

#include <QMainWindow>
#include <QProgressDialog>
#include <QTimer>
#include <QMap>
#include <QModelIndex>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <QStringList>

#include "packageversion.h"
#include "job.h"
#include "progressdialog.h"
#include "fileloader.h"
#include "taskbar.h"

namespace Ui {
    class MainWindow;
}

const UINT WM_ICONTRAY = WM_USER + 1;

const UINT NIN_BALLOONSHOW = WM_USER + 2;
const UINT NIN_BALLOONHIDE = WM_USER + 3;
const UINT NIN_BALLOONTIMEOUT = WM_USER + 4;
const UINT NIN_BALLOONUSERCLICK = WM_USER + 5;
const UINT NIN_SELECT = WM_USER + 0;
const UINT NINF_KEY = 1;
const UINT NIN_KEYSELECT = NIN_SELECT or NINF_KEY;

/**
 * Main window.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    static MainWindow* instance;

    time_t monitoredJobLastChanged;
    QList<Job*> runningJobs;
    QList<JobState> runningJobStates;

    Ui::MainWindow *ui;

    FileLoader fileLoader;
    QFrame* progressContent;
    QWidget* jobsTab;

    UINT taskbarMessageId;
    ITaskbarList3* taskbarInterface;

    void addJobsTab();
    void showDetails();
    void updateIcons();
    void updateActions();
    bool isUpdateEnabled(PackageVersion* pv);
    void setMenuAccelerators();
    void setActionAccelerators(QWidget* w);
    void chooseAccelerators(QStringList* titles);

    /**
     * Fills the table with known package versions.
     */
    void fillList();

    /**
     * This method returns a non-null PackageVersion* if something is selected
     * in the list or package details are shown in the current tab.
     *
     * @return selected package version or 0.
     */
    PackageVersion* getSelectedPackageVersion();

    /**
     * This method returns a all current PackageVersion* if
     * something is selected
     * in the list or package details are shown in the current tab.
     *
     * @return selected package versions
     */
    QList<PackageVersion*> getSelectedPackageVersions();

    /**
     * This method returns a non-null PackageVersion* if something is selected
     * in the list.
     *
     * @return selected package version or 0.
     */
    PackageVersion* getSelectedPackageVersionInTable();

    /**
     * This method returns all selected PackageVersion* items
     *
     * @return selected package versions
     */
    QList<PackageVersion*> getSelectedPackageVersionsInTable();

    /**
     * @param pv a version or 0
     */
    void selectPackageVersion(PackageVersion* pv);

    /**
     * Adds an entry in the "Progress" tab and monitors a task.
     *
     * @param title job title
     * @param job this job will be monitored. The object will be destroyed after
     *     the thread completion
     * @param thread the job itself. The object will be destroyed after the
     *     completion. The thread will be started in this method.
     */
    void monitor(Job* job, const QString& title, QThread* thread);

    void updateStatusInDetailTabs();
    void updateProgressTabTitle();
    void updateStatusInTable();

    virtual void closeEvent(QCloseEvent *event);
public:
    static QMap<QString, QIcon> icons;

    /**
     * @param pv a package versioin
     * @return icon for the specified package
     */
    static QIcon getPackageVersionIcon(PackageVersion* pv);

    /**
     * @return the only instance of this class
     */
    static MainWindow* getInstance();

    /**
     * This icon is used if a package does not define an icon.
     */
    static QIcon genericAppIcon;

    /** true if the hard drive scan is runnig */
    bool hardDriveScanRunning;

    /** true if the repositories are being reloaded */
    bool reloadRepositoriesThreadRunning;

    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /**
     * Adds an error message panel.
     *
     * @param msg short error message
     * @param details error details
     * @param autoHide true = automatically hide the message after some time
     * @param icon message icon
     */
    void addErrorMessage(const QString& msg, const QString& details="",
            bool autoHide=true, QMessageBox::Icon icon=QMessageBox::NoIcon);

    /**
     * Adds a new tab with the specified text
     *
     * @param title tab title
     * @param text the text
     * @param html true = HTML, false = plain text
     */
    void addTextTab(const QString& title, const QString& text, bool html=false);

    virtual bool winEvent(MSG* message, long* result);

    /**
     * Prepares the UI after the constructor was called.
     */
    void prepare();

    /**
     * Blocks until the job is completed. Shows an error
     * message dialog if the job was completed with an
     * error.
     *
     * @param job a job
     * @return true if the job has completed successfully
     *     (no error and not cancelled)
     * @param title dialog title
     */
    bool waitFor(Job* job, const QString& title);

    /**
     * Close all detail tabs (versions and licenses).
     */
    void closeDetailTabs();

    /**
     * Reloads the content of all repositories and recognizes all installed
     * packages again.
     */
    void recognizeAndLoadRepositories();

    /**
     * Unregistered a currently running monitored job.
     *
     * @param job a currently running and monitored job
     */
    void unregisterJob(Job* job);
protected:
    void changeEvent(QEvent *e);

    /**
     * @param install the objects will be destroyed
     */
    void process(QList<InstallOperation*>& install);
private slots:
    void processThreadFinished();
    void hardDriveScanThreadFinished();
    void recognizeAndLoadRepositoriesThreadFinished();
    void on_actionScan_Hard_Drives_triggered();
    void on_actionShow_Details_triggered();
    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabCloseRequested(int index);
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_actionAbout_triggered();
    void on_actionTest_Download_Site_triggered();
    void on_actionUpdate_triggered();
    void on_actionSettings_triggered();
    void on_lineEditText_textChanged(QString );
    void on_comboBoxStatus_currentIndexChanged(int index);
    void on_actionGotoPackageURL_triggered();
    void onShow();
    void on_actionInstall_activated();
    void on_tableWidget_itemSelectionChanged();
    void on_actionUninstall_activated();
    void on_actionExit_triggered();
    void iconDownloaded(const FileLoaderItem& it);
    void on_actionReload_Repositories_triggered();
    void on_actionClose_Tab_triggered();
    void repositoryStatusChanged(PackageVersion* pv);
    void monitoredJobChanged(const JobState& state);
};

#endif // MAINWINDOW_H
