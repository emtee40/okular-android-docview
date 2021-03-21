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

#include "kscroller.h"
#include <QScroller>

// FIXME Sometimes the scroller goes out of control and the scroll position jumps back and forth rapidly

KScroller::KScroller(QObject *target)
{
    m_scroller = QScroller::scroller(target);
    QScroller::grabGesture(target);

    QScrollerProperties prop;
    prop.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.3);
    prop.setScrollMetric(QScrollerProperties::MaximumVelocity, 1);
    prop.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.2); // Workaround for QTBUG-88249 (non-flick gestures recognized as accelerating flick)
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0);
    m_scroller->setScrollerProperties(prop);
}

QScroller *KScroller::qScroller() const
{
    return m_scroller;
}

// TODO Ensure this works with multiple pointing devices used simultaneously

bool KScroller::shouldIgnoreMousePress()
{
    bool result = m_scroller->state() != QScroller::Inactive;
    if (result) {
        m_isSmoothScrolling = true;
    }
    return result;
}

bool KScroller::shouldIgnoreMouseMove() const
{
    return m_isSmoothScrolling;
}

// FIXME Make it much easier to not scroll and tap on an item
// Currently you can sort of tap by trying very hard to keep you finger in one place and tapping twice
// This can possibly be done by taking the code from Dolphin

bool KScroller::shouldIgnoreMouseRelease()
{
    bool result = m_isSmoothScrolling;
    m_isSmoothScrolling = false;
    return result;
}

QScroller::State KScroller::state() const
{
    return m_scroller->state();
}

bool KScroller::handleInput(QScroller::Input input, const QPointF &position, qint64 timestamp) // clazy:exclude=function-args-by-value
{
    return m_scroller->handleInput(input, position, timestamp);
}

void KScroller::stop()
{
    m_scroller->stop();
}

QPointF KScroller::finalPosition() const
{
    return m_scroller->finalPosition();
}

void KScroller::scrollTo(const QPointF &pos) // clazy:exclude=function-args-by-value
{
    m_scroller->scrollTo(pos);
}

void KScroller::scrollTo(const QPointF &pos, int scrollTime) // clazy:exclude=function-args-by-value
{
    m_scroller->scrollTo(pos, scrollTime);
}
