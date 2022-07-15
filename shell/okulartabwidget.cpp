#include "okulartabwidget.h"

QSize OkularTabBar::tabSizeHint(int index) const
{
    const int availableWidth = size().width();
    const int defaultWidth = 20 * fontMetrics().averageCharWidth();

    int width = defaultWidth;
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