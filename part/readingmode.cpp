/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "readingmode.h"

// Qt includes
#include <QMenuBar>

// Kde includes
#include <KParts/MainWindow>
#include <KToggleAction>
#include <KToolBar>

// Okular includes
#include "debug_ui.h"
#include "part.h"
#include "sidebar.h"

namespace Okular
{
ReadingMode::ReadingMode(Part *part, KToggleAction *showReadingMode, bool &wasSideBarVisible)
    : m_part(part)
    , m_wasSideBarVisible(wasSideBarVisible)
    , m_sidebar(nullptr)
    , m_showReadingMode(showReadingMode)
    , m_showMenuBarAction(nullptr)
    , m_showBottomBar(nullptr)
    , m_showLeftPanel(nullptr)
{
}

ReadingMode::~ReadingMode()
{
    m_part = nullptr;
    m_showReadingMode = nullptr;
    m_showLeftPanel = nullptr;
    m_showMenuBarAction = nullptr;
    m_showBottomBar = nullptr;
    m_sidebar = nullptr;
}

void ReadingMode::initializeLinks(KToggleAction *showLeftPanel, KToggleAction *showMenuBarAction, KToggleAction *showBottomBar, Sidebar *sidebar, QPointer<QWidget> bottomBar)
{
    // Store references
    m_showLeftPanel = showLeftPanel;
    m_showMenuBarAction = showMenuBarAction;
    m_showBottomBar = showBottomBar;
    m_sidebar = sidebar;
    m_bottomBar = bottomBar;

    // Find reference to the KParts::MainWindow of Okular.
    bool parentWindowFound = Part::getSpecificWidgetfromList<KParts::MainWindow, QWidget>(m_showLeftPanel->associatedWidgets(), m_parentWindow);
    if (parentWindowFound)
        // Find references to the tool bars in Okular.
        m_toolbars = m_parentWindow->toolBars();
    else {
        qCWarning(OkularUiDebug) << "Unable to find a reference to main window to view reading mode";
        return;
    }
}

void ReadingMode::slotShowReadingMode()
{
    /*
     * Check to see that all the needed references are not null and show warning message for each of the references
     * that are null before exiting the method.
     */
    if (!m_showBottomBar || !m_showMenuBarAction || !m_sidebar || !m_bottomBar || m_toolbars.count() <= 0) {
        if (!m_showBottomBar)
            qCWarning(OkularUiDebug) << "Unable to find a reference to showBottomBar action";
        if (!m_showMenuBarAction)
            qCWarning(OkularUiDebug) << "Unable to find a reference to showMenuBar action";
        if (!m_sidebar)
            qCWarning(OkularUiDebug) << "Unable to find a reference to sidebar";
        if (!m_bottomBar)
            qCWarning(OkularUiDebug) << "Unable to find a reference to bottombar";
        if (m_toolbars.count() <= 0)
            qCWarning(OkularUiDebug) << "No toolbars in the main window:" << m_parentWindow << "were found.";

        return;
    }

    // Perform actions when the showReadingMode KToggleAction is checked.
    if (m_showReadingMode->isChecked()) {
        // Hide the side bar and store it's current visible state.
        m_wasSideBarVisible = m_sidebar->isSidebarVisible();
        m_sidebar->setSidebarVisibility(false);
        m_showLeftPanel->setChecked(false);

        // Set the bottom bar as visible to be able to show page numbers to the user.
        m_wasBottomBarVisible = m_bottomBar->isVisible();
        m_bottomBar->setVisible(true);
        m_showBottomBar->setChecked(true);

        // Hide the main menu bar and store it's current visible state.
        m_wasMenubarVisible = m_parentWindow->menuBar()->isVisible();
        m_parentWindow->menuBar()->setVisible(false);
        m_showMenuBarAction->setChecked(false);

        /* Hide all the toolbars associated with Okular and store each of
         * their current state in QList m_wasToolbarsVisible. If the QList does not contain any
         * items (which would be the case when the current method is called for the
         * first time) append the status of each of the toolbars to the QList.
         */
        if (m_wasToolbarsVisible.count() <= 0) {
            for (int i = 0; i < m_toolbars.count(); i++) {
                m_wasToolbarsVisible.append(m_toolbars[i]->isVisible());
            }
        } else {
            for (int i = 0; i < m_toolbars.count(); i++) {
                m_wasToolbarsVisible[i] = m_toolbars[i]->isVisible();
            }
        }
        for (KToolBar *toolbar : qAsConst(m_toolbars)) {
            toolbar->setVisible(false);
        }
        // Perform actions when the showReadingMode KToggleAction is unchecked.
    } else {
        // Restore the state of the side bar before the reading mode was triggered.
        m_sidebar->setSidebarVisibility(m_wasSideBarVisible);
        m_showLeftPanel->setChecked(m_wasSideBarVisible);

        // Restore the state of the bottom bar before the reading mode was triggered.
        m_bottomBar->setVisible(m_wasBottomBarVisible);
        m_showBottomBar->setChecked(m_wasBottomBarVisible);

        // Restore the state of the main menu bar before the reading mode was triggered.
        m_parentWindow->menuBar()->setVisible(m_wasMenubarVisible);
        m_showMenuBarAction->setChecked(m_wasMenubarVisible);

        // Restore the state of the all the tool bars of Okular before the reading mode was triggered.
        if (m_toolbars.count() == m_wasToolbarsVisible.count()) {
            for (int i = 0; i < m_toolbars.count(); i++) {
                m_toolbars[i]->setVisible(m_wasToolbarsVisible[i]);
            }
        }
    }
}
} // namespace Okular
