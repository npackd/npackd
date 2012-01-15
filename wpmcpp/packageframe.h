#ifndef PACKAGEFRAME_H
#define PACKAGEFRAME_H

#include <QFrame>
#include <QModelIndex>

#include "package.h"

namespace Ui {
    class PackageFrame;
}

class PackageFrame : public QFrame
{
    Q_OBJECT

private:
    void showDetails();
public:
    /** package associated with this form or 0 */
    Package* p;

    explicit PackageFrame(QWidget *parent = 0);
    ~PackageFrame();

    /**
     * Fills the form with the data of a package.
     *
     * @param p a package
     */
    void fillForm(Package* p);

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
private slots:
    void on_labelLicense_linkActivated(const QString &link);

    void on_tableWidgetVersions_doubleClicked(const QModelIndex &index);

private:
    Ui::PackageFrame *ui;
};

#endif // PACKAGEFRAME_H
