#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QFrame>
#include <QFrame>
#include <QList>
#include <QString>
#include <QTableWidget>
#include <QComboBox>

#include "package.h"
#include "selection.h"

namespace Ui {
    class MainFrame;
}

/**
 * Main frame
 */
class MainFrame : public QFrame, public Selection
{
    Q_OBJECT
private:
    void fillList();
public:
    explicit MainFrame(QWidget *parent = 0);
    ~MainFrame();

    QList<void*> getSelected(const QString& type) const;

    /**
     * This method returns a non-null Package* if something is selected
     * in the table.
     *
     * @return selected package or 0.
     */
    Package* getSelectedPackageInTable();

    /**
     * This method returns all selected Package* items
     *
     * @return [ownership:this] selected packages
     */
    QList<Package*> getSelectedPackagesInTable() const;

    /**
     * @return table with packages
     */
    QTableView *getTableWidget() const;

    /**
     * @return filter line edit
     */
    QLineEdit* getFilterLineEdit() const;

    /**
     * @brief filter for the status
     * @return 0=All, 1=Installed, 2=Updateable
     */
    int getStatusFilter() const;

    /**
     * @brief saves the column widths in the Windows registry
     */
    void saveColumns() const;

    /**
     * @brief load column widths from the Windows registry
     */
    void loadColumns() const;
private:
    Ui::MainFrame *ui;
private slots:
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_lineEditText_textChanged(QString );
    void tableWidget_selectionChanged();
    void on_radioButtonAll_toggled(bool checked);
    void on_radioButtonInstalled_toggled(bool checked);
    void on_radioButtonUpdateable_toggled(bool checked);
};

#endif // MAINFRAME_H
