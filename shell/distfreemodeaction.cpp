/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <KToolBar>
#include <QMenuBar>

#include "debug_distfreemode.h"
#include "distfreemodeaction.h"
#include "shell.h"

namespace Okular
{
void DistFreeModeAction::showDistfreeMode(KToggleAction *showMenuBarAction, QList<TabState> &tabs)
{
    // Reload pointers just in case some have become null.
    reloadLinks();

    // Handle the activated trigger
    if (isChecked()) {
        // Store the state of menu bar prior to Distraction-free Mode activation.
        setWasMenuBarVisible(showMenuBarAction->isChecked());
        // Hide the menu bar
        showMenuBarAction->setChecked(false);
        m_okularShell->menuBar()->setVisible(false);

        // Hide the toolbars
        handleToolBarVisibility(false);

    } else { // Handle deactivated trigger
        // Restore the state of the menu bar prior to Distraction-free Mode activation.
        showMenuBarAction->setChecked(getWasMenuBarVisible());
        m_okularShell->menuBar()->setVisible(getWasMenuBarVisible());
        // Restore the state of the tool bars prior to Distraction-free Mode activation.
        handleToolBarVisibility(true);
    }

    /*
     * Depending upon the checked state of the DistfreeModeAction, synchronize the GUI state across the open tabs if Distraction-free Mode is activated or
     * restore the GUI state of all open tabs to the state prior to Distraction-free Mode activation.
     */
    synchronizeTabs(tabs, isChecked());
}

void DistFreeModeAction::handleShellClose(const QList<TabState> &shellTabs, int currTabIndex, KToggleAction *showMenuBarAction)
{
    /*
     * Distraction-free Mode should be deactivated so that the GUI is restored to the state
     * prior to Distraction-free Mode activation, the next time Okular is started. This is if the user
     * tries to close the Shell while in the Distraction-free Mode
     */
    if (isChecked() && shellTabs.count() == 1)
        setChecked(false);
    /*
     * Distraction-free Mode is deactivated in a different way when there are multiple
     * tabs open. The idea is that GUI state is restored and saved for the current activated tab
     * so that it is the last active tab's GUI state that is restored next time Okular is launched.
     */
    else if (isChecked() && shellTabs.count() > 1) {
        // Restore the menu bar state prior to Distraction-free Mode activation.
        showMenuBarAction->setChecked(getWasMenuBarVisible());
        m_okularShell->menuBar()->setVisible(getWasMenuBarVisible());
        // Restore the tool bars states prior to Distraction-free Mode activation.
        handleToolBarVisibility(true);
        QList<TabState> tabs;
        // Restore the part GUI states only for the currently activated tab.
        tabs.append(shellTabs[currTabIndex]);
        synchronizeTabs(tabs, false);
    }
}

DistFreeModeAction::DistFreeModeAction(Shell *parent)
    : KToggleAction(parent)
    , m_okularShell(parent)
{
}

bool DistFreeModeAction::getWasMenuBarVisible() const
{
    return m_wasMenuBarVisible;
}

void DistFreeModeAction::setWasMenuBarVisible(bool wasMenuBarVisible)
{
    m_wasMenuBarVisible = wasMenuBarVisible;
}

void DistFreeModeAction::reloadLinks()
{
    m_toolBars.clear();
    const QList<KToolBar *> toolbars = m_okularShell->toolBars();
    for (KToolBar *toolBar : toolbars) {
        m_toolBars.append(toolBar);
    }
}

void DistFreeModeAction::handleToolBarVisibility(bool restore)
{
    // Reload m_wereToolBarsVisible QList if it had been corrupted.
    if (m_toolBars.count() != m_wereToolBarsVisible.count()) {
        m_wereToolBarsVisible.clear();
        for (int i = 0; i < m_toolBars.count(); i++) {
            if (m_toolBars[i])
                m_wereToolBarsVisible.append(m_toolBars[i]->isVisible());
            else {
                qCWarning(OkularDistfreeModeDebug) << "Pointer to a toolbar is either missing or corrupted.";
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
                qCWarning(OkularDistfreeModeDebug) << "Pointer to a toolbar is either missing or corrupted!";
            }
        }
    } else {
        for (int i = 0; i < m_toolBars.count(); i++) {
            if (m_toolBars[i])
                m_toolBars[i]->setVisible(m_wereToolBarsVisible[i]);
            else {
                qCWarning(OkularDistfreeModeDebug) << "Pointer to a toolbar is either missing or corrupted!";
            }
        }
    }
}

void DistFreeModeAction::synchronizeTabs(QList<TabState> &tabs, bool distfreeModeActivated)
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
        KActionCollection *ac = currPart->actionCollection();
        show_bottomBar = qobject_cast<KToggleAction *>(ac->action(SHOWBOTTOMBARACTIONNAME));
        show_leftPanel = qobject_cast<KToggleAction *>(ac->action(SHOWLEFTPANELACTIONNAME));
        // If pointers to left panel and bottom bar are found then continue otherwise return from the function
        if (!ac || !show_bottomBar || !show_leftPanel) {
            if (!ac)
                qCWarning(OkularDistfreeModeDebug) << "Pointer to Action Collection of" << currPart << "was not found!";
            else if (!show_leftPanel)
                qCWarning(OkularDistfreeModeDebug) << "Pointer to left panel of" << currPart << "was not found!";
            else if (!show_bottomBar)
                qCWarning(OkularDistfreeModeDebug) << "Pointer to bottom bar of" << currPart << "was not found!";
            return;
        }
        if (distfreeModeActivated) { // Handle GUI state when Distraction-free Mode is activated
            /*
             * Remember visibility state of bottom bar of the current tab. So that the
             * state can be restored once Distraction-free Mode is deactivated.
             */
            tabs[i].btmBarVisBeforeDistfreeMode = show_bottomBar->isChecked();
            // Show the bottom bar.
            show_bottomBar->setChecked(true);
            /*
             * Remember visibility state of side panel of the current tab. So that the
             * state can be restored once Distraction-free Mode is deactivated.
             */
            tabs[i].lftPnlVisBeforeDistfreeMode = show_leftPanel->isChecked();
            // Hide the left panel.
            show_leftPanel->setChecked(false);

        } else { // Handle GUI state when Distraction-free Mode is deactivated.
            // Restore the bottom bar and left panel visibility state prior to Distraction-free Mode activation for the current tab.
            show_bottomBar->setChecked(tabs[i].btmBarVisBeforeDistfreeMode);
            show_leftPanel->setChecked(tabs[i].lftPnlVisBeforeDistfreeMode);
        }
    }
}

void DistFreeModeAction::initalizeTabInDistfreeMode(TabState &newTab, const TabState currTab)
{
    // Copy the left panel visibility state from the passed currTab reference
    newTab.lftPnlVisBeforeDistfreeMode = currTab.lftPnlVisBeforeDistfreeMode;
    // Copy the bottom bar visibility state from the passed currTab reference
    newTab.btmBarVisBeforeDistfreeMode = currTab.btmBarVisBeforeDistfreeMode;
}
} // namespace Okular

Q_LOGGING_CATEGORY(OkularDistfreeModeDebug, "org.kde.okular.distfreemode", QtWarningMsg);
