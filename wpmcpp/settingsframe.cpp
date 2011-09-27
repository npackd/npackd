#include "settingsframe.h"
#include "ui_settingsframe.h"

#include "repository.h"
#include "mainwindow.h"
#include "wpmutils.h"

SettingsFrame::SettingsFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SettingsFrame)
{
    ui->setupUi(this);
}

SettingsFrame::~SettingsFrame()
{
    delete ui;
}

QStringList SettingsFrame::getRepositoryURLs()
{
    QString txt = this->ui->plainTextEditReps->toPlainText().trimmed();
    QStringList sl = txt.split("\n", QString::SkipEmptyParts);
    return sl;
}

QString SettingsFrame::getInstallationDirectory()
{
    return this->ui->lineEditDir->text();
}

void SettingsFrame::setInstallationDirectory(const QString& dir)
{
    this->ui->lineEditDir->setText(dir);
}

void SettingsFrame::setRepositoryURLs(const QStringList &urls)
{
    this->ui->plainTextEditReps->setPlainText(urls.join("\r\n"));
}

void SettingsFrame::on_buttonBox_accepted()
{
}

void SettingsFrame::on_buttonBox_clicked(QAbstractButton *button)
{
    MainWindow* mw = MainWindow::getInstance();

    Repository* r = Repository::getDefault();

    for (int i = 0; i < r->packageVersions.size(); i++) {
        PackageVersion* pv = r->packageVersions.at(i);
        if (pv->isLocked()) {
            QString msg("Cannot change settings now. "
                    "The package %1 is locked by a "
                    "currently running installation/removal.");
            mw->addErrorMessage(msg.arg(pv->toString()));
            return;
        }
    }

    QStringList list = getRepositoryURLs();
    if (list.count() == 0) {
        QString msg("No repositories defined");
        mw->addErrorMessage(msg, msg, true, QMessageBox::Critical);
    } else if (getInstallationDirectory().isEmpty()) {
        QString msg("The installation directory cannot be empty");
        mw->addErrorMessage(msg, msg, true, QMessageBox::Critical);
    } else if (!QDir(getInstallationDirectory()).exists()) {
        QString msg("The installation directory does not exist");
        mw->addErrorMessage(msg, msg, true, QMessageBox::Critical);
    } else {
        QString err;
        QList<QUrl*> urls;
        for (int i = 0; i < list.count(); i++) {
            QUrl* url = new QUrl(list.at(i));
            urls.append(url);
            if (!url->isValid()) {
                err = QString("%1 is not a valid repository address").arg(
                        list.at(i));
                break;
            }
        }
        if (err.isEmpty()) {
            WPMUtils::setInstallationDirectory(getInstallationDirectory());
            Repository::setRepositoryURLs(urls);
            mw->closeDetailTabs();
            mw->recognizeAndLoadRepositories();
        } else {
            mw->addErrorMessage(err, err, true, QMessageBox::Critical);
        }
        qDeleteAll(urls);
        urls.clear();
    }
}
