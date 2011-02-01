#include "packageversionform.h"
#include "ui_packageversionform.h"

#include "package.h"
#include "repository.h"
#include "mainwindow.h"
#include "license.h"

PackageVersionForm::PackageVersionForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PackageVersionForm)
{
    ui->setupUi(this);
    this->pv = 0;
}

void PackageVersionForm::updateIcons()
{
    QIcon icon = MainWindow::getPackageVersionIcon(pv);
    QPixmap pixmap = icon.pixmap(32, 32, QIcon::Normal, QIcon::On);
    this->ui->labelIcon->setPixmap(pixmap);
}

void PackageVersionForm::updateStatus()
{
    QString status;
    if (pv->external)
        status = "externally installed";
    else if (pv->installed())
        status = "installed";
    else
        status = "not installed";
    this->ui->lineEditStatus->setText(status);
}

void PackageVersionForm::fillForm(PackageVersion* pv)
{
    this->pv = pv;

    this->ui->lineEditTitle->setText(pv->getPackageTitle());
    this->ui->lineEditVersion->setText(pv->version.getVersionString());
    this->ui->lineEditInternalName->setText(pv->package);

    Repository* r = Repository::getDefault();
    Package* p = r->findPackage(pv->package);

    QString licenseTitle;
    if (p) {
        License* lic = r->findLicense(p->license);
        if (lic)
            licenseTitle = lic->title;
    }
    this->ui->lineEditLicense->setText(licenseTitle);

    if (p) {
        this->ui->textEditDescription->setText(p->description);
    }

    updateStatus();

    QString dl;
    if (pv->download.isEmpty())
        dl = "n/a";
    else
        dl = pv->download.toString();
    this->ui->lineEditDownload->setText(dl);

    QString sha1;
    if (pv->sha1.isEmpty())
        sha1 = "n/a";
    else
        sha1 = pv->sha1;
    this->ui->lineEditSHA1->setText(sha1);

    this->ui->lineEditType->setText(pv->type == 0 ? "zip" : "one-file");

    QString details;
    for (int i = 0; i < pv->importantFiles.count(); i++) {
        details.append(pv->importantFilesTitles.at(i));
        details.append(" (");
        details.append(pv->importantFiles.at(i));
        details.append(")\n");
    }
    this->ui->textEditImportantFiles->setText(details);

    details = "";
    for (int i = 0; i < pv->dependencies.count(); i++) {
        Dependency* d = pv->dependencies.at(i);
        details.append(d->toString());
        details.append("\n");
    }
    this->ui->textEditDependencies->setText(details);

    updateIcons();
}

PackageVersionForm::~PackageVersionForm()
{
    delete ui;
}

void PackageVersionForm::changeEvent(QEvent *e)
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
