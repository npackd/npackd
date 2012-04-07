#include "htmlitemdelegate.h"

#include <QTextDocument>
#include <QFont>
#include <QStyle>

HTMLItemDelegate::HTMLItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void HTMLItemDelegate::paint(QPainter* painter,
        const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    painter->save();

    QTextDocument doc;
    if (options.state.testFlag(QStyle::State_Selected) &&
            options.state.testFlag(QStyle::State_Active))
        doc.setDefaultStyleSheet("p { color: white; }");
    else
        doc.setDefaultStyleSheet("p { color: black; }");
    doc.setHtml(options.text);
    doc.setTextWidth(options.rect.width());

    /* Call this to get the focus rect and selection background. */
    options.text = "";
    options.widget->style()->drawControl(
            QStyle::CE_ItemViewItem, &options, painter);

    /* Draw using our rich text document. */
    int h = doc.size().height();
    if (h > options.rect.height())
        painter->translate(options.rect.left(), options.rect.top());
    else
        painter->translate(options.rect.left(), options.rect.top() +
                (options.rect.height() - h) / 2);
    QRect clip(0, 0, options.rect.width(), options.rect.height());
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize HTMLItemDelegate::sizeHint(
        const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    QTextDocument doc;
    doc.setHtml(options.text);
    doc.setTextWidth(options.rect.width());
    return QSize(doc.idealWidth(), doc.size().height());
}
