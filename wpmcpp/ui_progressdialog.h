/********************************************************************************
** Form generated from reading UI file 'progressdialog.ui'
**
** Created: Fri 21. Jan 08:20:23 2011
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROGRESSDIALOG_H
#define UI_PROGRESSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ProgressDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *labelStep;
    QLabel *label_2;
    QLabel *labelElapsed;
    QLabel *label_3;
    QLabel *label_remainingTime;
    QProgressBar *progressBar;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButtonCancel;

    void setupUi(QDialog *ProgressDialog)
    {
        if (ProgressDialog->objectName().isEmpty())
            ProgressDialog->setObjectName(QString::fromUtf8("ProgressDialog"));
        ProgressDialog->resize(396, 164);
        ProgressDialog->setAutoFillBackground(false);
        verticalLayout = new QVBoxLayout(ProgressDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(ProgressDialog);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        labelStep = new QLabel(ProgressDialog);
        labelStep->setObjectName(QString::fromUtf8("labelStep"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(labelStep->sizePolicy().hasHeightForWidth());
        labelStep->setSizePolicy(sizePolicy);
        labelStep->setWordWrap(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, labelStep);

        label_2 = new QLabel(ProgressDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        labelElapsed = new QLabel(ProgressDialog);
        labelElapsed->setObjectName(QString::fromUtf8("labelElapsed"));

        formLayout->setWidget(1, QFormLayout::FieldRole, labelElapsed);

        label_3 = new QLabel(ProgressDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        label_remainingTime = new QLabel(ProgressDialog);
        label_remainingTime->setObjectName(QString::fromUtf8("label_remainingTime"));

        formLayout->setWidget(2, QFormLayout::FieldRole, label_remainingTime);


        verticalLayout->addLayout(formLayout);

        progressBar = new QProgressBar(ProgressDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);

        verticalLayout->addWidget(progressBar);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        pushButtonCancel = new QPushButton(ProgressDialog);
        pushButtonCancel->setObjectName(QString::fromUtf8("pushButtonCancel"));
        pushButtonCancel->setDefault(true);

        horizontalLayout->addWidget(pushButtonCancel);


        verticalLayout->addLayout(horizontalLayout);

        verticalLayout->setStretch(0, 1);

        retranslateUi(ProgressDialog);

        QMetaObject::connectSlotsByName(ProgressDialog);
    } // setupUi

    void retranslateUi(QDialog *ProgressDialog)
    {
        ProgressDialog->setWindowTitle(QApplication::translate("ProgressDialog", "Progress", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("ProgressDialog", "Step:", 0, QApplication::UnicodeUTF8));
        labelStep->setText(QApplication::translate("ProgressDialog", "-", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("ProgressDialog", "Elapsed Time:", 0, QApplication::UnicodeUTF8));
        labelElapsed->setText(QApplication::translate("ProgressDialog", "-", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("ProgressDialog", "Remaining Time:", 0, QApplication::UnicodeUTF8));
        label_remainingTime->setText(QApplication::translate("ProgressDialog", "-", 0, QApplication::UnicodeUTF8));
        pushButtonCancel->setText(QApplication::translate("ProgressDialog", "Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ProgressDialog: public Ui_ProgressDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROGRESSDIALOG_H
