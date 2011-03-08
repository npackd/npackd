#include "packageversionform.h"
#include "ui_packageversionform.h"

#include "qdesktopservices.h"

#include "package.h"
#include "repository.h"
#include "mainwindow.h"
#include "license.h"
#include "licenseform.h"

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
    if (pv->isExternal())
        status = "externally installed";
    else if (pv->installed())
        status = "installed";
    else
        status = "not installed";
    this->ui->lineEditStatus->setText(status);
    this->ui->lineEditPath->setText(pv->getPath());
}

void PackageVersionForm::fillForm(PackageVersion* pv)
{
    this->pv = pv;

    this->ui->lineEditTitle->setText(pv->getPackageTitle());
    this->ui->lineEditVersion->setText(pv->version.getVersionString());
    this->ui->lineEditInternalName->setText(pv->package);

    Repository* r = Repository::getDefault();
    Package* p = r->findPackage(pv->package);

    QString licenseTitle = "unknown";
    if (p) {
        License* lic = r->findLicense(p->license);
        if (lic) {
            licenseTitle = "<a href=\"http://www.example.com\">" +
                    lic->title + "</a>";
        }
    }
    this->ui->labelLicense->setText(licenseTitle);

    if (p) {
        this->ui->textEditDescription->setText(p->description);

        QString hp;
        if (p->url.isEmpty())
            hp = "unknown";
        else{
            hp = p->url;
            hp = "<a href=\"" + hp + "\">" + hp + "</a>";
        }
        this->ui->labelHomePage->setText(hp);
    }

    updateStatus();

    QString dl;
    if (pv->download.isEmpty())
        dl = "n/a";
    else {
        dl = pv->download.toString();
        dl = "<a href=\"" + dl + "\">" + dl + "</a>";
    }
    this->ui->labelDownloadURL->setText(dl);

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

void PackageVersionForm::on_labelLicense_linkActivated(QString link)
{
    QTabWidget* tabWidget = dynamic_cast<QTabWidget*>(
            this->parentWidget()->parentWidget());
    if (!tabWidget)
        return;

    LicenseForm* f = new LicenseForm(tabWidget);

    Repository* r = Repository::getDefault();
    Package* p = r->findPackage(pv->package);
    if (!p)
        return;

    License* lic = r->findLicense(p->license);
    if (!lic)
        return;

    f->fillForm(lic);
    tabWidget->addTab(f, lic->title);
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

