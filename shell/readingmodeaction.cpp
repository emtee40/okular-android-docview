/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <KToolBar>

#include "debug_readingmode.h"
#include "readingmodeaction.h"
#include "shell.h"

namespace Okular
{
ReadingModeAction::ReadingModeAction(QObject *parent, const QPointer<Shell> &okularShell)
    : KToggleAction(parent)
    , m_okularShell(okularShell)
{
}

bool ReadingModeAction::getWasMenuBarVisible() const
{
    return m_wasMenuBarVisible;
}

void ReadingModeAction::setWasMenuBarVisible(bool wasMenuBarVisible)
{
    m_wasMenuBarVisible = wasMenuBarVisible;
}

QPointer<Shell> ReadingModeAction::getOkularShell() const
{
    return m_okularShell;
}

void ReadingModeAction::setOkularShell(const QPointer<Shell> &value)
{
    m_okularShell = value;
}

void ReadingModeAction::reloadLinks()
{
    m_toolBars.clear();
    const QList<KToolBar *> toolbars = m_okularShell->toolBars();
    for (KToolBar *toolBar : toolbars) {
        m_toolBars.append(toolBar);
    }
}

void ReadingModeAction::handleToolBarVisibility(bool restore)
{
    // Reload m_wereToolBarsVisible QList if it had been corrupted.
    if (m_toolBars.count() != m_wereToolBarsVisible.count()) {
        m_wereToolBarsVisible.clear();
        for (int i = 0; i < m_toolBars.count(); i++) {
            if (m_toolBars[i])
                m_wereToolBarsVisible.append(m_toolBars[i]->isVisible());
            else {
                qCWarning(OkularReadingModeDebug) << "Pointer to a toolbar is either missing or corrupted.";
            }
        }
    }
    // Handle visibility of the toolbar based on restore bool.
    if (!restore) {
        for (int i = 0; i < m_toolBars.count(); i++) {
            if (m_toolBars[i]) {
                m_wereToolBarsVisible[i] = m_toolBars[i]->isVisible();
                m_toolBars[i]->setVisible(false);
            } else {
                qCWarning(OkularReadingModeDebug) << "Pointer to a toolbar is either missing or corrupted!";
            }
        }
    } else {
        for (int i = 0; i < m_toolBars.count(); i++) {
            if (m_toolBars[i])
                m_toolBars[i]->setVisible(m_wereToolBarsVisible[i]);
            else {
                qCWarning(OkularReadingModeDebug) << "Pointer to a toolbar is either missing or corrupted!";
            }
        }
    }
}

void ReadingModeAction::synchronizeTabs(QList<TabState> &tabs, bool readingModeActivated)
{
    for (int i = 0; i < tabs.count(); i++) {
        // Pointer to store bottom bar action reference for a currPart
        KToggleAction *show_bottomBar = nullptr;
        // Pointer to store left panel action reference
        KToggleAction *show_leftPanel = nullptr;
        // Pointer to store current part reference
        KParts::ReadWritePart *currPart = nullptr;

        // Extract reference to current part from QList<TabState>
        currPart = tabs[i].part;

        // Extract pointer to bottom bar action
        bool foundBottomBarAct = Shell::findActionInPart<KToggleAction>(*currPart, Shell::SHOWBOTTOMBARACTIONNAME, show_bottomBar);
        // Extract pointer to left panel action
        bool foundLeftPanelAct = Shell::findActionInPart<KToggleAction>(*currPart, Shell::SHOWLEFTPANELACTIONNAME, show_leftPanel);
        // If pointers to left panel and bottom bar are found then continue otherwise return from the function
        if (!foundLeftPanelAct || !foundBottomBarAct) {
            if (!foundLeftPanelAct)
                qCWarning(OkularReadingModeDebug) << "Pointer to left panel of" << currPart << "was not found!";
            else if (!foundBottomBarAct)
                qCWarning(OkularReadingModeDebug) << "Pointer to bottom bar of" << currPart << "was not found!";
            return;
        }
        if (readingModeActivated) { // Handle GUI state when Reading Mode is activated
            /*
             * Remember visibility state of bottom bar of the current tab. So that the
             * state can be restored once Reading Mode is deactivated.
             */
            tabs[i].btmBarVisBeforeReadingMode = show_bottomBar->isChecked();
            // Show the bottom bar.
            show_bottomBar->setChecked(true);
            /*
             * Remember visibility state of side panel of the current tab. So that the
             * state can be restored once Reading Mode is deactivated.
             */
            tabs[i].lftPnlVisBeforeReadingMode = show_leftPanel->isChecked();
            // Hide the left panel.
            show_leftPanel->setChecked(false);

        } else { // Handle GUI state when Reading Mode is deactivated.
            // Restore the bottom bar and left panel visibility state prior to Reading Mode activation for the current tab.
            show_bottomBar->setChecked(tabs[i].btmBarVisBeforeReadingMode);
            show_leftPanel->setChecked(tabs[i].lftPnlVisBeforeReadingMode);
        }
    }
}

void ReadingModeAction::initalizeTabInReadingMode(TabState &newTab, const TabState currTab)
{
    // Copy the left panel visibility state from the passed currTab reference
    newTab.lftPnlVisBeforeReadingMode = currTab.lftPnlVisBeforeReadingMode;
    // Copy the bottom bar visibility state from the passed currTab reference
    newTab.btmBarVisBeforeReadingMode = currTab.btmBarVisBeforeReadingMode;
}
} // namespace Okular

Q_LOGGING_CATEGORY(OkularReadingModeDebug, "org.kde.okular.readingmode", QtWarningMsg);
