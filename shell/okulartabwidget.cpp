/*
    SPDX-FileCopyrightText: 2020 Theresa Gier <theresa@fam-gier.de>
    SPDX-FileCopyrightText: 2022 Tom Knuf <tom.knuf@outlook.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "okulartabwidget.h"

QSize OkularTabBar::tabSizeHint(int index) const
{
    const int availableWidth = size().width();
    const int defaultWidth = 40 * fontMetrics().averageCharWidth();

    int width = QTabBar::tabSizeHint(index).width();
    if (defaultWidth * count() > availableWidth) {
        // tabbar full, shrink tabs
        const int shrinkWidth = availableWidth / count();
        width = std::max(shrinkWidth, defaultWidth / 2);
    }

    const int height = QTabBar::tabSizeHint(index).height();
    return {width, height};
}

OkularTabWidget::OkularTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_TabBar = new OkularTabBar();
    setTabBar(m_TabBar);
}
