#ifndef LICENSEFORM_H
#define LICENSEFORM_H

#include <QWidget>

#include "license.h"

namespace Ui {
    class LicenseForm;
}

class LicenseForm : public QWidget {
    Q_OBJECT
public:
    LicenseForm(QWidget *parent = 0);
    ~LicenseForm();

    /**
     * @param license a license
     */
    void fillForm(License* license);
protected:
    void changeEvent(QEvent *e);

private:
    Ui::LicenseForm *ui;
};

#endif // LICENSEFORM_H
