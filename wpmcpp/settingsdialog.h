#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    /**
     * @return repository URL
     */
    QString getRepositoryURL();

    /**
     * @param url new repository URL
     */
    void setRepositoryURL(const QString& url);
protected:
    void changeEvent(QEvent *e);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
