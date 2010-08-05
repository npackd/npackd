#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "qstringlist.h"

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
};

#endif // SETTINGSDIALOG_H
