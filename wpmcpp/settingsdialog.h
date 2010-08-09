#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "qstringlist.h"
#include "qurl.h"

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    /**
     * @return repository URLs
     */
    QStringList getRepositoryURLs();

    /**
     * @param urls new repository URL
     */
    void setRepositoryURLs(const QStringList& urls);
protected:
    void changeEvent(QEvent *e);

private:
    Ui::SettingsDialog *ui;

private slots:
    void on_textBrowser_anchorClicked(QUrl url);
};

#endif // SETTINGSDIALOG_H
