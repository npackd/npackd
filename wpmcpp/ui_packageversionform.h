/********************************************************************************
** Form generated from reading UI file 'packageversionform.ui'
**
** Created: Sat 15. Jan 17:01:56 2011
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
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PackageVersionForm
{
public:
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
    QLineEdit *lineEditDownload;
    QLabel *label_10;
    QLineEdit *lineEditType;
    QLabel *label_9;
    QLineEdit *lineEditSHA1;
    QLabel *label_11;
    QTextEdit *textEditImportantFiles;
    QLabel *label_12;
    QTextEdit *textEditDependencies;

    void setupUi(QWidget *PackageVersionForm)
    {
        if (PackageVersionForm->objectName().isEmpty())
            PackageVersionForm->setObjectName(QString::fromUtf8("PackageVersionForm"));
        PackageVersionForm->resize(456, 421);
        formLayout = new QFormLayout(PackageVersionForm);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label_6 = new QLabel(PackageVersionForm);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_6);

        labelIcon = new QLabel(PackageVersionForm);
        labelIcon->setObjectName(QString::fromUtf8("labelIcon"));

        formLayout->setWidget(0, QFormLayout::FieldRole, labelIcon);

        label = new QLabel(PackageVersionForm);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label);

        lineEditTitle = new QLineEdit(PackageVersionForm);
        lineEditTitle->setObjectName(QString::fromUtf8("lineEditTitle"));
        lineEditTitle->setReadOnly(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, lineEditTitle);

        label_2 = new QLabel(PackageVersionForm);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_2);

        lineEditVersion = new QLineEdit(PackageVersionForm);
        lineEditVersion->setObjectName(QString::fromUtf8("lineEditVersion"));
        lineEditVersion->setReadOnly(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, lineEditVersion);

        label_5 = new QLabel(PackageVersionForm);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_5);

        textEditDescription = new QTextEdit(PackageVersionForm);
        textEditDescription->setObjectName(QString::fromUtf8("textEditDescription"));
        textEditDescription->setReadOnly(true);

        formLayout->setWidget(3, QFormLayout::FieldRole, textEditDescription);

        label_3 = new QLabel(PackageVersionForm);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_3);

        lineEditInternalName = new QLineEdit(PackageVersionForm);
        lineEditInternalName->setObjectName(QString::fromUtf8("lineEditInternalName"));
        lineEditInternalName->setReadOnly(true);

        formLayout->setWidget(5, QFormLayout::FieldRole, lineEditInternalName);

        label_7 = new QLabel(PackageVersionForm);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_7);

        lineEditStatus = new QLineEdit(PackageVersionForm);
        lineEditStatus->setObjectName(QString::fromUtf8("lineEditStatus"));
        lineEditStatus->setReadOnly(true);

        formLayout->setWidget(6, QFormLayout::FieldRole, lineEditStatus);

        label_8 = new QLabel(PackageVersionForm);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_8);

        lineEditDownload = new QLineEdit(PackageVersionForm);
        lineEditDownload->setObjectName(QString::fromUtf8("lineEditDownload"));
        lineEditDownload->setReadOnly(true);

        formLayout->setWidget(7, QFormLayout::FieldRole, lineEditDownload);

        label_10 = new QLabel(PackageVersionForm);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout->setWidget(8, QFormLayout::LabelRole, label_10);

        lineEditType = new QLineEdit(PackageVersionForm);
        lineEditType->setObjectName(QString::fromUtf8("lineEditType"));
        lineEditType->setReadOnly(true);

        formLayout->setWidget(8, QFormLayout::FieldRole, lineEditType);

        label_9 = new QLabel(PackageVersionForm);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        formLayout->setWidget(9, QFormLayout::LabelRole, label_9);

        lineEditSHA1 = new QLineEdit(PackageVersionForm);
        lineEditSHA1->setObjectName(QString::fromUtf8("lineEditSHA1"));
        lineEditSHA1->setReadOnly(true);

        formLayout->setWidget(9, QFormLayout::FieldRole, lineEditSHA1);

        label_11 = new QLabel(PackageVersionForm);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        formLayout->setWidget(10, QFormLayout::LabelRole, label_11);

        textEditImportantFiles = new QTextEdit(PackageVersionForm);
        textEditImportantFiles->setObjectName(QString::fromUtf8("textEditImportantFiles"));
        textEditImportantFiles->setReadOnly(true);

        formLayout->setWidget(10, QFormLayout::FieldRole, textEditImportantFiles);

        label_12 = new QLabel(PackageVersionForm);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        formLayout->setWidget(11, QFormLayout::LabelRole, label_12);

        textEditDependencies = new QTextEdit(PackageVersionForm);
        textEditDependencies->setObjectName(QString::fromUtf8("textEditDependencies"));
        textEditDependencies->setReadOnly(true);

        formLayout->setWidget(11, QFormLayout::FieldRole, textEditDependencies);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(lineEditTitle);
        label_2->setBuddy(lineEditVersion);
        label_5->setBuddy(textEditDescription);
        label_3->setBuddy(lineEditInternalName);
        label_7->setBuddy(lineEditStatus);
        label_8->setBuddy(lineEditDownload);
        label_10->setBuddy(lineEditType);
        label_9->setBuddy(lineEditSHA1);
        label_11->setBuddy(textEditImportantFiles);
        label_12->setBuddy(textEditDependencies);
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
    } // retranslateUi

};

namespace Ui {
    class PackageVersionForm: public Ui_PackageVersionForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PACKAGEVERSIONFORM_H
