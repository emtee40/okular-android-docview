/***************************************************************************
 *   Copyright (C) 2020 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef READINGMODE_H
#define READINGMODE_H

#include <QObject>
#include <QPointer>
#include <QWidget>

// Forward declaration of classes.
class KToggleAction;
class KToolBar;
class Sidebar;

namespace KParts
{
class MainWindow;
}

namespace Okular
{
class Part;

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
 * There are two parts to using this class. First the constuctor @ref ReadingMode::ReadingMode should be called in the
 * @ref Part::setupActions method in order to create proper signal and slot connections.
 * Later @ref ReadingMode::initializeLinks needs to be called to make the reading mode fully functional since not all references
 * required by the @ref ReadingMode::initializeLinks method are available during the execution of @ref Part::setupActions method.
 *
 * @author Sanchit Singh <sanckde@gmail.com>
 */
class ReadingMode : public QObject
{
    Q_OBJECT

private:
    Part *m_part;
    KParts::MainWindow *m_parentWindow;
    bool m_wasBottomBarVisible;
    bool &m_wasSideBarVisible;
    bool m_wasMenubarVisible;
    QList<bool> m_wasToolbarsVisible;
    QList<KToolBar *> m_toolbars;
    Sidebar *m_sidebar;
    QPointer<QWidget> m_bottomBar;

    KToggleAction *m_showReadingMode;
    KToggleAction *m_showMenuBarAction;
    KToggleAction *m_showBottomBar;
    KToggleAction *m_showLeftPanel;

public:
    /**
     * @brief ReadingMode The default constructor of the ReadingMode class.
     *
     * @param part This is a reference to the Part class instance of Okular
     * @param showReadingMode This is the reference to the KToggleAction created within the Part class instance.
     * @param wasSideBarVisible This is a reference to the wasSideBarVisible bool in Part class instance
     */
    explicit ReadingMode(Part *part, KToggleAction *showReadingMode, bool &wasSideBarVisible);
    ~ReadingMode();

    /**
     * @brief initializeLinks A method used for storing reference to GUI elements of the Okular to be able to restore
     * them to their initial state i.e. before the reading mode was initiated.
     *
     * @param show_LeftPanel This is a reference to the left panel in Okular GUI.
     * @param showMenuBarAction This is a reference to the showMenuBarAction stored in Part class instance.
     * @param showBottomBar This is a reference to the showBottomBarAction stored in the Part class instance.
     * @param sidebar This is a reference to the Sidebar class instance sotred in the Part class instance.
     * @param bottomBar This is a reference to the bottom bar QWidget instance stored in the Part class instance.
     */

    void initializeLinks(KToggleAction *show_LeftPanel, KToggleAction *showMenuBarAction, KToggleAction *showBottomBar, Sidebar *sidebar, QPointer<QWidget> &bottomBar);

    void showReadingMode();
};
}

#endif // READINGMODE_H
