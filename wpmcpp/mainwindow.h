#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qprogressdialog.h>
#include <qtimer.h>

#include "packageversion.h"
#include "job.h"

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
     * Blocks until the job is completed.
     *
     * @param job a job
     */
    void waitFor(Job* job);
protected:
    void changeEvent(QEvent *e);
private slots:
    void jobChanged(void* job);
private:
    Ui::MainWindow *ui;
    QProgressDialog* pd;

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
    void on_MainWindow_destroyed();
    void on_actionExit_triggered();
};

#endif // MAINWINDOW_H
