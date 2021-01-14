/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef READINGMODEACTION_H
#define READINGMODEACTION_H

#include <KToggleAction>
#include <QPointer>

// Forward declaration of classes.
class KToolBar;
class Shell;
struct TabState;

namespace KParts
{
class MainWindow;
class ReadWritePart;
}

namespace Okular
{
/**
 * @short The ReadingMode class used for displaying reading mode in Okular.
 *
 * This class is used for displaying reading mode in Okular. Reading mode basically provides a convenient way of hiding menubar,
 * sidebar, main toolbar, annotation toolbar but showing the pagebar with one click of a toggle button. This mode also shows the
 * window manager taskbar, so that windows of others programs can be viewed on screen in parallel with minimal GUI of okular
 * for workflows that require using multiple programs in parallel.
 *
 * This mode remembers the status of aformentioned GUI elements before it was triggered and restores back the GUI elements to
 * the same state as before the reading mode was triggered.
 *
 * @author Sanchit Singh <sanckde@gmail.com>
 */
class ReadingModeAction : public KToggleAction
{
    Q_OBJECT

private:
    bool m_wasMenuBarVisible;
    QList<bool> m_wereToolBarsVisible;
    QPointer<Shell> m_okularShell;
    QList<QPointer<KToolBar>> m_toolBars;

public:
    /**
     * @brief ReadingModeAction The default constructor of reading mode.
     * @param parent The parent object of this Action.
     * @param okularShell A pointer to the Shell where this instance is created.
     */
    explicit ReadingModeAction(QObject *parent, const QPointer<Shell> &okularShell);
    /**
     * @brief getWasMenuBarVisible Getter for m_wasMenuBarVisible bool variable. This holds the state of the
     * menu bar visibility prior to Reading Mode activation.
     * @return bool containing the visibility state of the menu bar prior to Reading Mode activation.
     */
    bool getWasMenuBarVisible() const;
    /**
     * @brief setWasMenuBarVisible Setting for m_wasMenuBarVisible bool variable. This holds the state of the
     * menu bar visibility prior to Reading Mode activation.
     * @param wasMenuBarVisible holds the visibility state of the menu bar.
     */
    void setWasMenuBarVisible(bool wasMenuBarVisible);
    /**
     * @brief getOkularShell Getter for m_okularShell variable which is a pointer to the Shell in which current
     * instance was created.
     * @return QPointer to the Shell in which current instance was created.
     */
    QPointer<Shell> getOkularShell() const;
    /**
     * @brief setOkularShell Setter for m_okularShell variable which is a pointer to the Shell in which current
     * instance was created.
     * @param value A const QPointer reference to Shell in which current instance was created.
     */
    void setOkularShell(const QPointer<Shell> &value);
    /**
     * @brief reloadLinks A function that reloads the pointer to GUI elements of the Shell. Useful if one doubts
     * that original pointers stored in m_toolBars have become null.
     */
    void reloadLinks();
    /**
     * @brief handleToolBarVisibility A method used to show and hide the toolbars of the Shell in which current instance
     * was created.
     * @param restore A bool which specifies if the toolbars should be restored to the state prior to Reading Mode activation or if they
     * should have a state during Reading Mode activation.
     */
    void handleToolBarVisibility(bool restore);
    /**
     * @brief synchronizeTabs A static helper method to synchronize the GUI elements state both when Reading Mode is activated and
     * de-activated between the open tabs of the Shell in which current instance was created.
     * @param tabs A reference to QList<TabState> which contain t
     * @param readingModeActivated A bool that tells the method what kind of synchronization that needs to be performed between the tabs.
     */
    static void synchronizeTabs(QList<TabState> &tabs, bool readingModeActivated);
    /**
     * @brief initalizeTabInReadingMode A static helper method to initialize a new tab when it is opened while Reading Mode is activated.
     * @param newTab A reference the new tab that is created.
     * @param currTab A reference to the current tab, which is used as the basis to synchronize the GUI elements state.
     */
    static void initalizeTabInReadingMode(TabState &newTab, const TabState currTab);
};
}

#endif // READINGMODEACTION_H
