/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created: Fri Sep 3 22:13:18 2010
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QFormLayout>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QTableWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionExit;
    QAction *actionInstall;
    QAction *actionUninstall;
    QAction *actionGotoPackageURL;
    QAction *actionSettings;
    QAction *actionUpdate;
    QAction *actionTest_Download_Site;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_3;
    QFrame *frame;
    QVBoxLayout *verticalLayout;
    QTableWidget *tableWidget;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QDockWidget *dockWidget;
    QWidget *dockWidgetContents;
    QFormLayout *formLayout;
    QLabel *label_2;
    QComboBox *comboBoxStatus;
    QLabel *label_3;
    QLineEdit *lineEditText;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(620, 406);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionInstall = new QAction(MainWindow);
        actionInstall->setObjectName(QString::fromUtf8("actionInstall"));
        QIcon icon;
        icon.addFile(QString::fromUtf8("edit_add.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionInstall->setIcon(icon);
        actionUninstall = new QAction(MainWindow);
        actionUninstall->setObjectName(QString::fromUtf8("actionUninstall"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8("stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionUninstall->setIcon(icon1);
        actionGotoPackageURL = new QAction(MainWindow);
        actionGotoPackageURL->setObjectName(QString::fromUtf8("actionGotoPackageURL"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8("internet&networking.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionGotoPackageURL->setIcon(icon2);
        actionSettings = new QAction(MainWindow);
        actionSettings->setObjectName(QString::fromUtf8("actionSettings"));
        actionUpdate = new QAction(MainWindow);
        actionUpdate->setObjectName(QString::fromUtf8("actionUpdate"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8("agt_reload.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionUpdate->setIcon(icon3);
        actionTest_Download_Site = new QAction(MainWindow);
        actionTest_Download_Site->setObjectName(QString::fromUtf8("actionTest_Download_Site"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout_3 = new QVBoxLayout(centralWidget);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        frame = new QFrame(centralWidget);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 1);
        tableWidget = new QTableWidget(frame);
        if (tableWidget->columnCount() < 1)
            tableWidget->setColumnCount(1);
        if (tableWidget->rowCount() < 1)
            tableWidget->setRowCount(1);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
        tableWidget->setEnabled(true);
        tableWidget->setMouseTracking(true);
        tableWidget->setFrameShape(QFrame::StyledPanel);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSortingEnabled(true);
        tableWidget->setRowCount(1);
        tableWidget->setColumnCount(1);
        tableWidget->verticalHeader()->setDefaultSectionSize(35);

        verticalLayout->addWidget(tableWidget);


        verticalLayout_3->addWidget(frame);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 620, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        mainToolBar->setIconSize(QSize(32, 32));
        mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);
        dockWidget = new QDockWidget(MainWindow);
        dockWidget->setObjectName(QString::fromUtf8("dockWidget"));
        dockWidget->setFloating(false);
        dockWidget->setFeatures(QDockWidget::DockWidgetFloatable);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        formLayout = new QFormLayout(dockWidgetContents);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label_2 = new QLabel(dockWidgetContents);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_2);

        comboBoxStatus = new QComboBox(dockWidgetContents);
        comboBoxStatus->setObjectName(QString::fromUtf8("comboBoxStatus"));

        formLayout->setWidget(0, QFormLayout::FieldRole, comboBoxStatus);

        label_3 = new QLabel(dockWidgetContents);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_3);

        lineEditText = new QLineEdit(dockWidgetContents);
        lineEditText->setObjectName(QString::fromUtf8("lineEditText"));

        formLayout->setWidget(1, QFormLayout::FieldRole, lineEditText);

        dockWidget->setWidget(dockWidgetContents);
        MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(1), dockWidget);

        menuBar->addAction(menuFile->menuAction());
        menuFile->addAction(actionInstall);
        menuFile->addAction(actionUninstall);
        menuFile->addAction(actionUpdate);
        menuFile->addSeparator();
        menuFile->addAction(actionGotoPackageURL);
        menuFile->addAction(actionTest_Download_Site);
        menuFile->addSeparator();
        menuFile->addAction(actionSettings);
        menuFile->addAction(actionExit);
        mainToolBar->addAction(actionInstall);
        mainToolBar->addAction(actionUninstall);
        mainToolBar->addAction(actionUpdate);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionGotoPackageURL);

        retranslateUi(MainWindow);

        comboBoxStatus->setCurrentIndex(4);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("MainWindow", "Exit", 0, QApplication::UnicodeUTF8));
        actionInstall->setText(QApplication::translate("MainWindow", "Install", 0, QApplication::UnicodeUTF8));
        actionUninstall->setText(QApplication::translate("MainWindow", "Uninstall", 0, QApplication::UnicodeUTF8));
        actionGotoPackageURL->setText(QApplication::translate("MainWindow", "Go To Package Page", 0, QApplication::UnicodeUTF8));
        actionGotoPackageURL->setShortcut(QApplication::translate("MainWindow", "Ctrl+G", 0, QApplication::UnicodeUTF8));
        actionSettings->setText(QApplication::translate("MainWindow", "Settings", 0, QApplication::UnicodeUTF8));
        actionUpdate->setText(QApplication::translate("MainWindow", "Update", 0, QApplication::UnicodeUTF8));
        actionTest_Download_Site->setText(QApplication::translate("MainWindow", "Test Download Site", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
        dockWidget->setWindowTitle(QApplication::translate("MainWindow", "Filter", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "Status:", 0, QApplication::UnicodeUTF8));
        comboBoxStatus->clear();
        comboBoxStatus->insertItems(0, QStringList()
         << QApplication::translate("MainWindow", "All", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindow", "Not Installed", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindow", "Installed", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindow", "Installed, Update Available", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("MainWindow", "Newest or Installed", 0, QApplication::UnicodeUTF8)
        );
        label_3->setText(QApplication::translate("MainWindow", "Text:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        lineEditText->setToolTip(QApplication::translate("MainWindow", "Enter here your search text. You can enter multiple words if a package should contain all of them. The search is case insensitive. No special characters are filtered out.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
