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
     * @return combobox for the status filter
     */
    QComboBox* getStatusComboBox() const;
private:
    Ui::MainFrame *ui;
private slots:
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_lineEditText_textChanged(QString );
    void on_comboBoxStatus_currentIndexChanged(int index);
    void tableWidget_selectionChanged();
};

#endif // MAINFRAME_H
