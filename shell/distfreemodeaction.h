/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef DISTRACTIONFREEMODEACTION_H
#define DISTRACTIONFREEMODEACTION_H

#include <KToggleAction>
#include <QPointer>

// Forward declaration of classes.
class KToolBar;
class Shell;
class QTabWidget;
struct TabState;

namespace KParts
{
class MainWindow;
class ReadWritePart;
}

namespace Okular
{
/**
 * @short The DistfreeMode class used for displaying distraction-free mode in Okular.
 *
 * This class is used for displaying distraction-free mode in Okular. Distraction-free mode basically provides a convenient way of hiding menubar,
 * sidebar, main toolbar, annotation toolbar but showing the pagebar with one click of a toggle button. This mode also shows the
 * window manager taskbar, so that windows of others programs can be viewed on screen in parallel with minimal GUI of okular
 * for workflows that require using multiple programs in parallel.
 *
 * This mode remembers the status of aformentioned GUI elements before it was triggered and restores back the GUI elements to
 * the same state as before the distraction-free mode was triggered.
 *
 * @author Sanchit Singh <sanckde@gmail.com>
 */
class DistFreeModeAction : public KToggleAction
{
    Q_OBJECT

private:
    bool m_wasMenuBarVisible;
    QList<bool> m_wereToolBarsVisible;
    Shell *m_okularShell;
    QList<QPointer<KToolBar>> m_toolBars;
    // Const strings used for extracting specific QActions from Part.
    const QString SHOWLEFTPANELACTIONNAME = QStringLiteral("show_leftpanel");
    const QString SHOWBOTTOMBARACTIONNAME = QStringLiteral("show_bottombar");

public:
    /**
     * DistfreeModeAction The default constructor of distraction-free mode.
     * @param parent The parent object of this Action which is Shell in this case.
     */
    explicit DistFreeModeAction(Shell *parent);
    /**
     * getWasMenuBarVisible Getter for m_wasMenuBarVisible bool variable. This holds the state of the
     * menu bar visibility prior to Distraction-free Mode activation.
     * @return bool containing the visibility state of the menu bar prior to Distraction-free Mode activation.
     */
    bool getWasMenuBarVisible() const;
    /**
     * setWasMenuBarVisible Setting for m_wasMenuBarVisible bool variable. This holds the state of the
     * menu bar visibility prior to Distraction-free Mode activation.
     * @param wasMenuBarVisible holds the visibility state of the menu bar.
     */
    void setWasMenuBarVisible(bool wasMenuBarVisible);
    /**
     * @brief reloadLinks A function that reloads the pointer to GUI elements of the Shell. Useful if one doubts
     * that original pointers stored in m_toolBars have become null.
     */
    void reloadLinks();
    /**
     * handleToolBarVisibility A method used to show and hide the toolbars of the Shell in which current instance
     * was created.
     * @param restore A bool which specifies if the toolbars should be restored to the state prior to Distraction-free Mode activation or if they
     * should have a state during Distraction-free Mode activation.
     */
    void handleToolBarVisibility(bool restore);
    /**
     * synchronizeTabs A helper method to synchronize the GUI elements state both when Distraction-free Mode is activated and
     * de-activated between the open tabs of the Shell in which current instance was created.
     * @param tabs A reference to QList<TabState> which contain TabState of all the open tabs in the shell.
     * @param distfreeModeActivated A bool that tells the method what kind of synchronization that needs to be performed between the tabs.
     */
    void synchronizeTabs(QList<TabState> &tabs, bool distfreeModeActivated);
    /**
     * initalizeTabInDistfreeMode A helper method to initialize a new tab when it is opened while Distraction-free Mode is activated.
     * @param newTab A reference the new tab that is created.
     * @param currTab A reference to the current tab, which is used as the basis to synchronize the GUI elements state.
     */
    void initalizeTabInDistfreeMode(TabState &newTab, const TabState currTab);
    /**
     * showDistfreeMode A helper method to handle activation and de-activation of Distraction-free Mode. This method is usually called
     * from outside the class.
     * @param showMenuBarAction used for hiding and showing the menu bar of the shell.
     * @param tabs reference to list of TabState to be able to synchronize current state of Distraction-free Mode across the open tabs in the shell.
     */
    void showDistfreeMode(KToggleAction *showMenuBarAction, QList<TabState> &tabs);
    /**
     * handleShellClose A helper method to handle the situation when the user tries to close the shell when
     * @param shellTabs reference to list of TabState to be able to synchronize current state of Distraction-free Mode across the open tabs in the shell.
     * @param currTabIndex this is the index of the current activated tab used for saving the state prior to Distraction-Free Mode activation, when the user tries to close the shell.
     * @param showMenuBarAction used for hiding and showing the menu bar of the shell
     */
    void handleShellClose(const QList<TabState> &shellTabs, int currTabIndex, KToggleAction *showMenuBarAction);
};
}

#endif // DISTRACTIONFREEMODEACTION_H
