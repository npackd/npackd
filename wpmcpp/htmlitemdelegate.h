#ifndef HTMLITEMDELEGATE_H
#define HTMLITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QSize>

class HTMLItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HTMLItemDelegate(QObject *parent = 0);
protected:
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
signals:

public slots:

};

#endif // HTMLITEMDELEGATE_H
