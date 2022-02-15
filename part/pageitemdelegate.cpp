/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pageitemdelegate.h"

// qt/kde includes
#include <QAbstractItemDelegate>
#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QModelIndex>
#include <QTextDocument>
#include <QVariant>

// local includes
#include "settings.h"

#define PAGEITEMDELEGATE_INTERNALMARGIN 3

class PageItemDelegate::Private
{
public:
    Private()
    {
    }

    QModelIndex index;
    QTreeView *parent;
    QAbstractItemModel *model;
};

PageItemDelegate::PageItemDelegate(QTreeView *parent, QAbstractItemModel *model)
    : QItemDelegate(parent)
    , d(new Private)
{
    d->parent = parent;
    d->model = model;
}

PageItemDelegate::~PageItemDelegate()
{
    delete d;
}

void PageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    d->index = index;
//    QStyleOptionViewItem m_option = option;
//    d->model->setData(index, option.rect.width(), ItemModelWidth);
    QItemDelegate::paint(painter, option, index);
}

void PageItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
{
    QVariant pageVariant = d->index.data(PageRole);
    QVariant labelVariant = d->index.data(PageLabelRole);
    if ((labelVariant.type() != QVariant::String && !pageVariant.canConvert(QVariant::String)) || !Okular::Settings::tocPageColumn()) {
        QItemDelegate::drawDisplay(painter, option, rect, text);
        return;
    }
    QString label = labelVariant.toString();
    QString page = label.isEmpty() ? pageVariant.toString() : label;
    QTextDocument document;
    document.setPlainText(page);
    document.setDefaultFont(option.font);
    int margindelta = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    int pageRectWidth = (int)document.size().width();
    QRect newRect(rect);
    QRect pageRect(rect);
    pageRect.setWidth(pageRectWidth + 2 * margindelta);
    newRect.setWidth(newRect.width() - pageRectWidth - PAGEITEMDELEGATE_INTERNALMARGIN);
    if (option.direction == Qt::RightToLeft)
        newRect.translate(pageRectWidth + PAGEITEMDELEGATE_INTERNALMARGIN, 0);
    else
        pageRect.translate(newRect.width() + PAGEITEMDELEGATE_INTERNALMARGIN - 2 * margindelta, 0);
    // Apply word wrapping if applicable
    //    if (Okular::Settings::self()->tOCWordWrap()) {
    //        QSize sz = newRect.size();
    //        QFontMetrics metrics(option.font);
    //        QRect outRect = metrics.boundingRect(QRect(QPoint(0, 0), sz), Qt::AlignLeft | Qt::TextWordWrap, text);
    //        sz.setHeight(outRect.height());

    //        newRect.setSize(sz);
    //    }
    qDebug() << "Paint width " << option.rect.width() << "FOr index" << text;
    QItemDelegate::drawDisplay(painter, option, newRect, text);
    QStyleOptionViewItem newoption(option);
    newoption.displayAlignment = (option.displayAlignment & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight;
    QItemDelegate::drawDisplay(painter, newoption, pageRect, page);
}

QSize PageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    qDebug() << "PageItemDelegate::sizeHint width" << d->parent->header()->width() << "For Index " << index.data().toString();
//    QSize sz = QItemDelegate::sizeHint(option, index);
//     https://stackoverflow.com/questions/34623036/implementing-a-delegate-for-wordwrap-in-a-qtreeview-qt-pyside-pyqt
    QSize sz = QSize(d->parent->header()->width()*0.6, 10000);
//    QSize sz = QSize(index.data(ItemModelWidth).toInt(), 10000);
//    qDebug() << index.data().toString() << sz;
    QFontMetrics metrics(index.data(Qt::FontRole).value<QFont>());
    QRect outRect = metrics.boundingRect(QRect(QPoint(0, 0), sz), Qt::AlignLeft | Qt::TextWordWrap, index.data().toString());
    sz.setHeight(outRect.height());
    return sz;
}

void PageItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QItemDelegate::updateEditorGeometry(editor, option, index);
}

#include "moc_pageitemdelegate.cpp"
