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

QStringList SettingsDialog::getRepositoryURLs()
{
    QString txt = this->ui->textEditReps->toPlainText().trimmed();
    QStringList sl = txt.split("\r\n");
    return sl;
}

void SettingsDialog::setRepositoryURLs(const QStringList &urls)
{
    this->ui->textEditReps->setText(urls.join("\r\n"));
}
