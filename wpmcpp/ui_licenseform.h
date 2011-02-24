/********************************************************************************
** Form generated from reading UI file 'licenseform.ui'
**
** Created: Thu 24. Feb 21:45:25 2011
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LICENSEFORM_H
#define UI_LICENSEFORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFormLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LicenseForm
{
public:
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLineEdit *lineEditTitle;
    QLabel *labelURL;
    QLineEdit *lineEditInternalName;

    void setupUi(QWidget *LicenseForm)
    {
        if (LicenseForm->objectName().isEmpty())
            LicenseForm->setObjectName(QString::fromUtf8("LicenseForm"));
        LicenseForm->resize(400, 300);
        verticalLayout = new QVBoxLayout(LicenseForm);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        scrollArea = new QScrollArea(LicenseForm);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Sunken);
        scrollArea->setLineWidth(0);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 382, 282));
        formLayout = new QFormLayout(scrollAreaWidgetContents);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(scrollAreaWidgetContents);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        label_2 = new QLabel(scrollAreaWidgetContents);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_2);

        label_3 = new QLabel(scrollAreaWidgetContents);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_3);

        lineEditTitle = new QLineEdit(scrollAreaWidgetContents);
        lineEditTitle->setObjectName(QString::fromUtf8("lineEditTitle"));
        lineEditTitle->setReadOnly(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, lineEditTitle);

        labelURL = new QLabel(scrollAreaWidgetContents);
        labelURL->setObjectName(QString::fromUtf8("labelURL"));
        labelURL->setOpenExternalLinks(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, labelURL);

        lineEditInternalName = new QLineEdit(scrollAreaWidgetContents);
        lineEditInternalName->setObjectName(QString::fromUtf8("lineEditInternalName"));
        lineEditInternalName->setReadOnly(true);

        formLayout->setWidget(3, QFormLayout::FieldRole, lineEditInternalName);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(lineEditTitle);
        label_3->setBuddy(lineEditInternalName);
#endif // QT_NO_SHORTCUT

        retranslateUi(LicenseForm);

        QMetaObject::connectSlotsByName(LicenseForm);
    } // setupUi

    void retranslateUi(QWidget *LicenseForm)
    {
        LicenseForm->setWindowTitle(QApplication::translate("LicenseForm", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("LicenseForm", "Title:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("LicenseForm", "Download URL:", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("LicenseForm", "Internal Name:", 0, QApplication::UnicodeUTF8));
        labelURL->setText(QApplication::translate("LicenseForm", "TextLabel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LicenseForm: public Ui_LicenseForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LICENSEFORM_H
