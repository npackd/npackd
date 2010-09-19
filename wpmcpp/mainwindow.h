#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>

#include <QMainWindow>
#include <qprogressdialog.h>
#include <qtimer.h>

#include "packageversion.h"
#include "job.h"
#include "progressdialog.h"

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
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool winEvent(MSG* message, long* result);

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
    bool waitFor(Job* job, QString& title);

    /**
     * Load the content of all defined repositories and updates the UI.
     */
    void loadRepositories();

    void recognizeAndloadRepositories();
protected:
    void changeEvent(QEvent *e);
private slots:
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
private:
    Ui::MainWindow *ui;

    /**
     * Fills the table with known package versions.
     */
    void fillList();

    /**
     * @return selected package version or null.
     */
    PackageVersion* getSelectedPackageVersion();

    /**
     * @param pv a version or 0
     */
    void selectPackageVersion(PackageVersion* pv);
};

#endif // MAINWINDOW_H
