#include "packageframe.h"
#include "ui_packageframe.h"

#include "packageversionform.h"
#include "ui_packageversionform.h"

#include "qdesktopservices.h"

#include "package.h"
#include "repository.h"
#include "mainwindow.h"
#include "license.h"
#include "licenseform.h"
#include "packageversionform.h"

PackageFrame::PackageFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PackageFrame)
{
    ui->setupUi(this);
    this->p = 0;

    MainWindow* mw = MainWindow::getInstance();

    QTableWidget* t = this->ui->tableWidgetVersions;
    t->addAction(mw->findChild<QAction*>("actionInstall"));
    t->addAction(mw->findChild<QAction*>("actionUninstall"));
    t->addAction(mw->findChild<QAction*>("actionUpdate"));
    t->addAction(mw->findChild<QAction*>("actionShow_Details"));
    t->addAction(mw->findChild<QAction*>("actionTest_Download_Site"));
}

PackageFrame::~PackageFrame()
{
    delete ui;
}

void PackageFrame::updateIcons()
{
    QIcon icon = MainWindow::getPackageVersionIcon(p->name);
    QPixmap pixmap = icon.pixmap(32, 32, QIcon::Normal, QIcon::On);
    this->ui->labelIcon->setPixmap(pixmap);
}

void PackageFrame::updateStatus()
{
    for (int i = 0; i < this->ui->tableWidgetVersions->rowCount(); i++) {
        QTableWidgetItem* item = this->ui->tableWidgetVersions->item(i, 1);
        const QVariant v = item->data(Qt::UserRole);
        PackageVersion* pv = (PackageVersion*) v.value<void*>();
        item->setText(pv->getPath());
    }
}

void PackageFrame::fillForm(Package* p)
{
    this->p = p;

    Repository* r = Repository::getDefault();

    this->ui->lineEditTitle->setText(p->title);
    this->ui->lineEditInternalName->setText(p->name);

    QString licenseTitle = "unknown";
    if (p) {
        License* lic = r->findLicense(p->license);
        if (lic) {
            licenseTitle = "<a href=\"http://www.example.com\">" +
                    Qt::escape(lic->title) + "</a>";
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
            hp = "<a href=\"" + Qt::escape(hp) + "\">" + Qt::escape(hp) +
                    "</a>";
        }
        this->ui->labelHomePage->setText(hp);
    }

    updateIcons();

    QTableWidgetItem *newItem;
    QTableWidget* t = this->ui->tableWidgetVersions;
    t->clear();
    t->setColumnCount(2);
    t->setColumnWidth(1, 400);
    newItem = new QTableWidgetItem("Version");
    t->setHorizontalHeaderItem(0, newItem);
    newItem = new QTableWidgetItem("Installation path");
    t->setHorizontalHeaderItem(1, newItem);

    QList<PackageVersion*> pvs = r->getPackageVersions(p->name);
    t->setRowCount(pvs.size());
    for (int i = pvs.count() - 1; i >= 0; i--) {
        PackageVersion* pv = pvs.at(i);

        newItem = new QTableWidgetItem(pv->version.getVersionString());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(pvs.count() - i - 1, 0, newItem);

        newItem = new QTableWidgetItem("");
        newItem->setText(pv->getPath());
        newItem->setData(Qt::UserRole, qVariantFromValue((void*) pv));
        t->setItem(pvs.count() - i - 1, 1, newItem);
    }
}

void PackageFrame::changeEvent(QEvent *e)
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

void PackageFrame::on_labelLicense_linkActivated(const QString &link)
{
    QTabWidget* tabWidget = dynamic_cast<QTabWidget*>(
            this->parentWidget()->parentWidget());
    if (!tabWidget)
        return;

    LicenseForm* f = new LicenseForm(tabWidget);

    Repository* r = Repository::getDefault();

    License* lic = r->findLicense(p->license);
    if (!lic)
        return;

    f->fillForm(lic);
    tabWidget->addTab(f, lic->title);
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void PackageFrame::showDetails()
{
    MainWindow* mw = MainWindow::getInstance();
    QList<QTableWidgetItem*> sel = this->ui->tableWidgetVersions->selectedItems();
    for (int i = 0; i < sel.count(); i++) {
        QTableWidgetItem* item = sel.at(i);
        if (item->column() == 0) {
            const QVariant v = item->data(Qt::UserRole);

            PackageVersion* pv = (PackageVersion*) v.value<void*>();
            PackageVersionForm* pvf = new PackageVersionForm(0);
            pvf->fillForm(pv);
            QIcon icon = mw->getPackageVersionIcon(pv->package);
            mw->addTab(pvf, icon, p->title + " " +
                    pv->version.getVersionString());
        }
    }
}

QList<void*> PackageFrame::getSelected(const QString& type) const
{
    QList<void*> res;
    if (type == "Package" && this->p) {
        res.append(this->p);
    } else if (type == "PackageVersion") {
        if (this->ui->tableWidgetVersions->hasFocus()) {
            QList<QTableWidgetItem*> sel =
                    this->ui->tableWidgetVersions->selectedItems();
            for (int i = 0; i < sel.count(); i++) {
                QTableWidgetItem* item = sel.at(i);
                if (item->column() == 0) {
                    const QVariant v = item->data(Qt::UserRole);

                    PackageVersion* pv = (PackageVersion*) v.value<void*>();
                    res.append(pv);
                }
            }
        }
    }
    return res;
}

void PackageFrame::on_tableWidgetVersions_doubleClicked(const QModelIndex &index)
{
    showDetails();
}

void PackageFrame::on_tableWidgetVersions_itemSelectionChanged()
{
    MainWindow::getInstance()->updateActions();
}
