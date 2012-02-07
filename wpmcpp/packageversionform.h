#ifndef PACKAGEVERSIONFORM_H
#define PACKAGEVERSIONFORM_H

#include <QWidget>

#include "packageversion.h"

namespace Ui {
    class PackageVersionForm;
}

class PackageVersionForm : public QWidget {
    Q_OBJECT
public:
    /** PackageVersion associated with this form or 0 */
    PackageVersion* pv;

    PackageVersionForm(QWidget *parent = 0);
    ~PackageVersionForm();

    /**
     * Fills the form with the data of a package version.
     *
     * @param package full package name
     * @param version package version
     */
    void fillForm(const QString& package, const Version& version);

    /**
     * Updates the view if a new icon was downloaded.
     */
    void updateIcons();

    /**
     * Updates package status.
     */
    void updateStatus();
protected:
    void changeEvent(QEvent *e);

private:
    Ui::PackageVersionForm *ui;

private slots:
    void on_labelLicense_linkActivated(QString link);
};

#endif // PACKAGEVERSIONFORM_H
