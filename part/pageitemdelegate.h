/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PAGEITEMDELEGATE_H
#define PAGEITEMDELEGATE_H

#include <QItemDelegate>
#include <QTreeView>

class PageItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit PageItemDelegate(QTreeView *parent = nullptr);
    ~PageItemDelegate() override;

    static const int PageRole = 0x000f0001;
    static const int PageLabelRole = 0x000f0002;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    //    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    class Private;
    Private *const d;
};

#endif
