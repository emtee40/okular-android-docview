/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "readingmodeaction.h"
#include "debug_ui.h"

namespace Okular
{
KParts::MainWindow *ReadingModeAction::parentWindow() const
{
    return m_parentWindow;
}

void ReadingModeAction::setParentWindow(KParts::MainWindow *parentWindow)
{
    m_parentWindow = parentWindow;
}

bool ReadingModeAction::wasBottomBarVisible() const
{
    return m_wasBottomBarVisible;
}

void ReadingModeAction::setWasBottomBarVisible(bool wasBottomBarVisible)
{
    m_wasBottomBarVisible = wasBottomBarVisible;
}

bool ReadingModeAction::wasMenuBarVisible() const
{
    return m_wasMenuBarVisible;
}

void ReadingModeAction::setWasMenuBarVisible(bool wasMenuBarVisible)
{
    m_wasMenuBarVisible = wasMenuBarVisible;
}

QList<bool> ReadingModeAction::wasToolbarsVisible() const
{
    return m_wasToolbarsVisible;
}

void ReadingModeAction::setWasToolbarsVisible(const QList<bool> &wasToolbarsVisible)
{
    m_wasToolbarsVisible = wasToolbarsVisible;
}

QList<KToolBar *> ReadingModeAction::getToolbars() const
{
    return toolbars;
}

void ReadingModeAction::setToolbars(const QList<KToolBar *> &value)
{
    toolbars = value;
}

ReadingModeAction::ReadingModeAction(QObject *parent)
    : KToggleAction(parent)
    , m_parentWindow(nullptr)
{
}

} // namespace Okular
