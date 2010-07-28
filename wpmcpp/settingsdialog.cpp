#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "qmessagebox.h"

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

QString SettingsDialog::getRepositoryURL()
{
    return this->ui->lineEditRepository->text().trimmed();
}

void SettingsDialog::setRepositoryURL(const QString &url)
{
    this->ui->lineEditRepository->setText(url);
}
