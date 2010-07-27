#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qprogressdialog.h>
#include <qtimer.h>

#include "packageversion.h"
#include "job.h"
#include "progressdialog.h"

namespace Ui {
    class MainWindow;
}

/**
 * Main window.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /**
     * Blocks until the job is completed. Shows an error
     * message dialog if the job was completed with an
     * error.
     *
     * @param job a job
     * @return true if the job has completed successfully
     *     (no error and not cancelled)
     */
    bool waitFor(Job* job);

    void loadRepository();
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_comboBoxStatus_currentIndexChanged(int index);
    void on_comboBox_activated(int index);
    void on_actionGotoPackageURL_triggered();
    void on_pushButtonSaveSettings_clicked();
    void onShow();
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
private slots:
    void on_actionInstall_activated();
    void on_tableWidget_itemSelectionChanged();
    void on_actionUninstall_activated();
    void on_actionExit_triggered();
};

#endif // MAINWINDOW_H
