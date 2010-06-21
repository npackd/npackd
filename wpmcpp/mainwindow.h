#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "packageversion.h"

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

protected:
    void changeEvent(QEvent *e);

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
    void on_MainWindow_destroyed();
    void on_actionExit_triggered();
};

#endif // MAINWINDOW_H
