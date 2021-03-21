/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <aacid@kde.org>               *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   With portions of code from kpdf/kpdf_pagewidget.h by:                 *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Kurt Pfeifle <kpfeifle@danka.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
// This file follows coding style described in kdebase/kicker/HACKING

#ifndef _OKULAR_KSCROLLER_H_
#define _OKULAR_KSCROLLER_H_

// TODO move to KWidgetsAddons

#include <QScroller>

class KScroller
{
public:
    KScroller(QObject *target);
    QScroller *qScroller() const;

    bool shouldIgnoreMousePress();
    bool shouldIgnoreMouseMove() const;
    bool shouldIgnoreMouseRelease();

    QScroller::State state() const;
    bool handleInput(QScroller::Input input, const QPointF &position, qint64 timestamp = 0);
    void stop();
    QPointF finalPosition() const;
    void scrollTo(const QPointF &pos);
    void scrollTo(const QPointF &pos, int scrollTime);
private:
    QScroller *m_scroller;
    bool m_isSmoothScrolling;
};

#endif
