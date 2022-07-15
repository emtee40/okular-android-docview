/***************************************************************************
 *   Copyright (C) 2020 by Theresa Gier <theresa@fam-gier.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TABWIDGET_H_
#define _OKULAR_TABWIDGET_H_

#include <QTabBar>
#include <QTabWidget>

class OkularTabBar : public QTabBar
{
protected:
    QSize tabSizeHint(int index) const override;
};

class OkularTabWidget: public QTabWidget
{
public:
    OkularTabWidget(QWidget *parent = nullptr);

private:
    OkularTabBar *m_TabBar;
};

#endif
