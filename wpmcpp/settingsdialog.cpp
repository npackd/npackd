#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "qmessagebox.h"
#include "qdesktopservices.h"

#include "repository.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

QStringList SettingsDialog::getRepositoryURLs()
{
    QString txt = this->ui->plainTextEditReps->toPlainText().trimmed();
    QStringList sl = txt.split("\n", QString::SkipEmptyParts);
    return sl;
}

QString SettingsDialog::getInstallationDirectory()
{
    return this->ui->lineEditDir->text();
}

void SettingsDialog::setInstallationDirectory(const QString& dir)
{
    this->ui->lineEditDir->setText(dir);
}

void SettingsDialog::setRepositoryURLs(const QStringList &urls)
{
    this->ui->plainTextEditReps->setPlainText(urls.join("\r\n"));
}

