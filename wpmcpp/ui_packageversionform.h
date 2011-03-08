/********************************************************************************
** Form generated from reading UI file 'packageversionform.ui'
**
** Created: Tue 8. Mar 22:11:58 2011
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PACKAGEVERSIONFORM_H
#define UI_PACKAGEVERSIONFORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFormLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QScrollArea>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PackageVersionForm
{
public:
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QFormLayout *formLayout;
    QLabel *label_6;
    QLabel *labelIcon;
    QLabel *label;
    QLineEdit *lineEditTitle;
    QLabel *label_2;
    QLineEdit *lineEditVersion;
    QLabel *label_5;
    QTextEdit *textEditDescription;
    QLabel *label_3;
    QLineEdit *lineEditInternalName;
    QLabel *label_7;
    QLineEdit *lineEditStatus;
    QLabel *label_8;
    QLabel *label_10;
    QLineEdit *lineEditType;
    QLabel *label_9;
    QLineEdit *lineEditSHA1;
    QLabel *label_11;
    QTextEdit *textEditImportantFiles;
    QLabel *label_12;
    QTextEdit *textEditDependencies;
    QLabel *label_4;
    QLabel *labelDownloadURL;
    QLabel *labelLicense;
    QLabel *label_13;
    QLineEdit *lineEditPath;
    QLabel *label_14;
    QLabel *labelHomePage;

    void setupUi(QWidget *PackageVersionForm)
    {
        if (PackageVersionForm->objectName().isEmpty())
            PackageVersionForm->setObjectName(QString::fromUtf8("PackageVersionForm"));
        PackageVersionForm->resize(456, 450);
        verticalLayout = new QVBoxLayout(PackageVersionForm);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        scrollArea = new QScrollArea(PackageVersionForm);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 421, 507));
        formLayout = new QFormLayout(scrollAreaWidgetContents);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label_6 = new QLabel(scrollAreaWidgetContents);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_6);

        labelIcon = new QLabel(scrollAreaWidgetContents);
        labelIcon->setObjectName(QString::fromUtf8("labelIcon"));

        formLayout->setWidget(0, QFormLayout::FieldRole, labelIcon);

        label = new QLabel(scrollAreaWidgetContents);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label);

        lineEditTitle = new QLineEdit(scrollAreaWidgetContents);
        lineEditTitle->setObjectName(QString::fromUtf8("lineEditTitle"));
        lineEditTitle->setReadOnly(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, lineEditTitle);

        label_2 = new QLabel(scrollAreaWidgetContents);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_2);

        lineEditVersion = new QLineEdit(scrollAreaWidgetContents);
        lineEditVersion->setObjectName(QString::fromUtf8("lineEditVersion"));
        lineEditVersion->setReadOnly(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, lineEditVersion);

        label_5 = new QLabel(scrollAreaWidgetContents);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_5);

        textEditDescription = new QTextEdit(scrollAreaWidgetContents);
        textEditDescription->setObjectName(QString::fromUtf8("textEditDescription"));
        textEditDescription->setReadOnly(true);

        formLayout->setWidget(3, QFormLayout::FieldRole, textEditDescription);

        label_3 = new QLabel(scrollAreaWidgetContents);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_3);

        lineEditInternalName = new QLineEdit(scrollAreaWidgetContents);
        lineEditInternalName->setObjectName(QString::fromUtf8("lineEditInternalName"));
        lineEditInternalName->setReadOnly(true);

        formLayout->setWidget(6, QFormLayout::FieldRole, lineEditInternalName);

        label_7 = new QLabel(scrollAreaWidgetContents);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_7);

        lineEditStatus = new QLineEdit(scrollAreaWidgetContents);
        lineEditStatus->setObjectName(QString::fromUtf8("lineEditStatus"));
        lineEditStatus->setReadOnly(true);

        formLayout->setWidget(7, QFormLayout::FieldRole, lineEditStatus);

        label_8 = new QLabel(scrollAreaWidgetContents);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout->setWidget(9, QFormLayout::LabelRole, label_8);

        label_10 = new QLabel(scrollAreaWidgetContents);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout->setWidget(11, QFormLayout::LabelRole, label_10);

        lineEditType = new QLineEdit(scrollAreaWidgetContents);
        lineEditType->setObjectName(QString::fromUtf8("lineEditType"));
        lineEditType->setReadOnly(true);

        formLayout->setWidget(11, QFormLayout::FieldRole, lineEditType);

        label_9 = new QLabel(scrollAreaWidgetContents);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        formLayout->setWidget(12, QFormLayout::LabelRole, label_9);

        lineEditSHA1 = new QLineEdit(scrollAreaWidgetContents);
        lineEditSHA1->setObjectName(QString::fromUtf8("lineEditSHA1"));
        lineEditSHA1->setReadOnly(true);

        formLayout->setWidget(12, QFormLayout::FieldRole, lineEditSHA1);

        label_11 = new QLabel(scrollAreaWidgetContents);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        formLayout->setWidget(13, QFormLayout::LabelRole, label_11);

        textEditImportantFiles = new QTextEdit(scrollAreaWidgetContents);
        textEditImportantFiles->setObjectName(QString::fromUtf8("textEditImportantFiles"));
        textEditImportantFiles->setReadOnly(true);

        formLayout->setWidget(13, QFormLayout::FieldRole, textEditImportantFiles);

        label_12 = new QLabel(scrollAreaWidgetContents);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        formLayout->setWidget(14, QFormLayout::LabelRole, label_12);

        textEditDependencies = new QTextEdit(scrollAreaWidgetContents);
        textEditDependencies->setObjectName(QString::fromUtf8("textEditDependencies"));
        textEditDependencies->setReadOnly(true);

        formLayout->setWidget(14, QFormLayout::FieldRole, textEditDependencies);

        label_4 = new QLabel(scrollAreaWidgetContents);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_4);

        labelDownloadURL = new QLabel(scrollAreaWidgetContents);
        labelDownloadURL->setObjectName(QString::fromUtf8("labelDownloadURL"));
        labelDownloadURL->setOpenExternalLinks(true);

        formLayout->setWidget(9, QFormLayout::FieldRole, labelDownloadURL);

        labelLicense = new QLabel(scrollAreaWidgetContents);
        labelLicense->setObjectName(QString::fromUtf8("labelLicense"));

        formLayout->setWidget(4, QFormLayout::FieldRole, labelLicense);

        label_13 = new QLabel(scrollAreaWidgetContents);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_13);

        lineEditPath = new QLineEdit(scrollAreaWidgetContents);
        lineEditPath->setObjectName(QString::fromUtf8("lineEditPath"));
        lineEditPath->setReadOnly(true);

        formLayout->setWidget(5, QFormLayout::FieldRole, lineEditPath);

        label_14 = new QLabel(scrollAreaWidgetContents);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        formLayout->setWidget(10, QFormLayout::LabelRole, label_14);

        labelHomePage = new QLabel(scrollAreaWidgetContents);
        labelHomePage->setObjectName(QString::fromUtf8("labelHomePage"));
        labelHomePage->setMouseTracking(true);
        labelHomePage->setOpenExternalLinks(true);

        formLayout->setWidget(10, QFormLayout::FieldRole, labelHomePage);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(lineEditTitle);
        label_2->setBuddy(lineEditVersion);
        label_5->setBuddy(textEditDescription);
        label_3->setBuddy(lineEditInternalName);
        label_7->setBuddy(lineEditStatus);
        label_10->setBuddy(lineEditType);
        label_9->setBuddy(lineEditSHA1);
        label_11->setBuddy(textEditImportantFiles);
        label_12->setBuddy(textEditDependencies);
        label_13->setBuddy(lineEditPath);
#endif // QT_NO_SHORTCUT

        retranslateUi(PackageVersionForm);

        QMetaObject::connectSlotsByName(PackageVersionForm);
    } // setupUi

    void retranslateUi(QWidget *PackageVersionForm)
    {
        PackageVersionForm->setWindowTitle(QApplication::translate("PackageVersionForm", "Form", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("PackageVersionForm", "Icon:", 0, QApplication::UnicodeUTF8));
        labelIcon->setText(QString());
        label->setText(QApplication::translate("PackageVersionForm", "Title:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("PackageVersionForm", "Version:", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("PackageVersionForm", "Description:", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("PackageVersionForm", "Internal Name:", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("PackageVersionForm", "Status:", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("PackageVersionForm", "Download URL:", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("PackageVersionForm", "Type:", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("PackageVersionForm", "SHA1:", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("PackageVersionForm", "Important Files:", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("PackageVersionForm", "Dependencies:", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("PackageVersionForm", "License:", 0, QApplication::UnicodeUTF8));
        labelDownloadURL->setText(QApplication::translate("PackageVersionForm", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://www.test.de\"><span style=\" text-decoration: underline; color:#0000ff;\">TextLabel</span></a></p></body></html>", 0, QApplication::UnicodeUTF8));
        labelLicense->setText(QApplication::translate("PackageVersionForm", "TextLabel", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("PackageVersionForm", "Installation Path:", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("PackageVersionForm", "Package Home Page:", 0, QApplication::UnicodeUTF8));
        labelHomePage->setText(QApplication::translate("PackageVersionForm", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://www.test.de\"><span style=\" text-decoration: underline; color:#0000ff;\">TextLabel</span></a></p></body></html>", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PackageVersionForm: public Ui_PackageVersionForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PACKAGEVERSIONFORM_H
