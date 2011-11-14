#include "licenseform.h"
#include "ui_licenseform.h"

LicenseForm::LicenseForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LicenseForm)
{
    ui->setupUi(this);
}

LicenseForm::~LicenseForm()
{
    delete ui;
}

void LicenseForm::fillForm(License* license)
{
    this->ui->lineEditTitle->setText(license->title);

    QString dl;
    if (license->url.isEmpty())
        dl = "n/a";
    else {
        dl = license->url;
        dl = "<a href=\"" + dl + "\">" + dl + "</a>";
    }
    this->ui->labelURL->setText(dl);

    this->ui->lineEditInternalName->setText(license->name);
}

void LicenseForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
