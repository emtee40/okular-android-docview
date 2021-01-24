/***************************************************************************
 *   Copyright (C) 2021 by Sanchit Singh <sanckde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "../part/part.h"
#include "../part/sidebar.h"
#include "../settings.h"
#include "../shell/okular_main.h"
#include "../shell/readingmodeaction.h"
#include "../shell/shell.h"
#include <KToolBar>
#include <QMenuBar>
#include <QTest>

namespace Okular
{
// A helper class to be able to access private members of Okular::Part class.
class PartTest
{
public:
    Sidebar *getSidebar(const Okular::Part *part)
    {
        return part->m_sidebar;
    }

    QWidget *getBottombar(const Okular::Part *part)
    {
        return part->m_bottomBar;
    }
};
}
// A helper function to fetch a reference to an instance of Shell class.
Shell *findShell(Shell *ignore = nullptr)
{
    const QWidgetList wList = QApplication::topLevelWidgets();
    for (QWidget *widget : wList) {
        Shell *s = qobject_cast<Shell *>(widget);
        if (s && s != ignore)
            return s;
    }
    return nullptr;
}

class ReadingModeTest : public QObject, public Okular::PartTest
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void checkReadingMode();
    void checkReadingMode_data();
    // Private constants
private:
    /*
     * Constant chars pointers that define the datatags for the different iterations of the Reading Mode tests.
     * "Empty Shell": A test that will check that the ReadingModeAction is not enabled when there are no files opened.
     * "One Tab": A test that will check the GUI elements state prior, during and after the Reading Mode is activated.
     * "Two Tab": A test that will check the GUI elements state is synchronized between tabs when Reading Mode is activated.
     * "Two Tab Save State": A test that will check that when a Okular shell is closed with multiple tabs open while Reading Mode is activated,
     * the GUI elements state (prior to Reading Mode activation) is restored for the currently activated tab.
     */
    const char *EMPTY_SHELL_TEST = "Empty Shell";
    const char *ONE_TAB_TEST = "One Tab";
    const char *TWO_TAB_TEST = "Two Tab";
    const char *TWO_TAB_SAVESTATE_TEST = "Two Tab Save State";
    // Private variables
private:
    // Variables that are used to store GUI elements state prior to Reading Mode activation.
    QList<bool> toolBarState;
    bool menuBarState;
    QList<bool> sideBarState;
    QList<bool> bottomBarState;
    // Private functions
private:
    // Helper function to run checks prior to Reading Mode activation.
    void storePriorReadingModeState(const Shell *shell);
    // Helper function to run checks after Reading Mode activation.
    void checkAfterReadingModeState(const Shell *shell);
    // Helper function to run checks while Reading Mode is activated.
    void checkReadingModeState(Shell *shell);
    // Helper function to reset the variables that store GUI elements state prior to Reading Mode activation.
    void clearStates();
};

void ReadingModeTest::initTestCase()
{
    // Use a test user environment to store confirguration files.
    QStandardPaths::setTestModeEnabled(true);
    // Don't pollute people's okular settings
    Okular::Settings::instance(QStringLiteral("readingmodetest"));
}

void ReadingModeTest::init()
{
    /*
     * Delete the configuration file to restore the GUI element states to their defaults
     */
    QString configFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("readingmodetestrc"));
    if (QFile::exists(configFile)) {
        QFile file(configFile);
        file.remove();
    }
    Okular::Settings::self()->setDefaults();
    // Set the shell mode to open multiple files in tabs.
    Okular::Settings::setShellOpenFileInTabs(true);
}

void ReadingModeTest::checkReadingMode_data()
{
    QTest::addColumn<QStringList>("paths");

    QTest::newRow(EMPTY_SHELL_TEST) << QStringList();
    QTest::newRow(ONE_TAB_TEST) << (QStringList() << QStringLiteral(KDESRCDIR "data/file1.pdf"));
    QTest::newRow(TWO_TAB_TEST) << (QStringList() << QStringLiteral(KDESRCDIR "data/file1.pdf") << QStringLiteral(KDESRCDIR "data/file2.pdf"));
    QTest::newRow(TWO_TAB_SAVESTATE_TEST) << (QStringList() << QStringLiteral(KDESRCDIR "data/file1.pdf") << QStringLiteral(KDESRCDIR "data/file2.pdf"));
}

void ReadingModeTest::checkReadingMode()
{
    // Fetch data for the current check type.
    QFETCH(QStringList, paths);
    // Create a new shell and check its status.
    Okular::Status status = Okular::main(paths, QString());
    QCOMPARE(status, Okular::Success);
    Shell *shell = findShell();
    QVERIFY(shell);
    QVERIFY(shell->m_showReadingModeAction);
    // Run different checks depending on which check type is being run.
    if (QString::compare(QTest::currentDataTag(), EMPTY_SHELL_TEST) == 0) {
        QCOMPARE(shell->m_showReadingModeAction->isEnabled(), false);
    } else if (QString::compare(QTest::currentDataTag(), ONE_TAB_TEST) == 0) {
        QCOMPARE(shell->m_tabs.count(), 1);
        Okular::Part *part = shell->findChild<Okular::Part *>();
        QVERIFY(part);
        QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
        QCOMPARE(shell->m_showReadingModeAction->isEnabled(), true);
        storePriorReadingModeState(shell);
        shell->m_showReadingModeAction->setChecked(true);
        QTest::qWait(750);
        shell->m_showReadingModeAction->setChecked(false);
        QTest::qWait(750);
        checkAfterReadingModeState(shell);
    } else if (QString::compare(QTest::currentDataTag(), TWO_TAB_TEST) == 0) {
        QCOMPARE(shell->m_tabs.count(), 2);
        storePriorReadingModeState(shell);
        for (int i = 0; i < shell->m_tabs.count(); ++i) {
            Okular::Part *currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[i].part);
            QVERIFY(currPart);
            QCOMPARE(currPart->url().url(), QStringLiteral("file://%1").arg(paths[i]));
        }
        shell->m_showReadingModeAction->setChecked(true);
        QTest::qWait(750);
        checkReadingModeState(shell);
        shell->m_showReadingModeAction->setChecked(false);
        QTest::qWait(750);
        checkAfterReadingModeState(shell);
    } else if (QString::compare(QTest::currentDataTag(), TWO_TAB_SAVESTATE_TEST) == 0) {
        QCOMPARE(shell->m_tabs.count(), 2);
        Okular::Part *currPart = nullptr;
        storePriorReadingModeState(shell);
        for (int i = 0; i < shell->m_tabs.count(); ++i) {
            currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[i].part);
            QVERIFY(currPart);
            QCOMPARE(currPart->url().url(), QStringLiteral("file://%1").arg(paths[i]));
        }
        // Change the GUI states of one of the tabs to values that are not default
        shell->activateNextTab();
        int activeTabIndex = shell->m_tabWidget->currentIndex();
        currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[activeTabIndex].part);
        Sidebar *currSideBar = getSidebar(currPart);
        currSideBar->setSidebarVisibility(false);
        QWidget *currBottomBar = getBottombar(currPart);
        currBottomBar->setVisible(true);
        // Store the GUI states of all the tabs.
        storePriorReadingModeState(shell);
        // Activate Reading Mode.
        shell->m_showReadingModeAction->setChecked(true);
        /*
         * Activate different tabs to be able to check later that Okular restores the GUI elements states
         * of the tab that was active when Okular was closed while in Reading Mode.
         */
        QTest::qWait(750);
        shell->activatePrevTab();
        QTest::qWait(750);
        shell->activateNextTab();
        QTest::qWait(750);
        // Store the index of the currently activated tab before closing the shell
        activeTabIndex = shell->m_tabWidget->currentIndex();
        delete shell;
        // Create a new shell and check its status.
        Okular::Status status = Okular::main(QStringList(), QString());
        QCOMPARE(status, Okular::Success);
        Shell *shell = findShell();
        QVERIFY(shell);
        // Check that Okular restored the state of the activated tab when Okular was closed during last run while in Reading Mode.
        Okular::Part *part = shell->findChild<Okular::Part *>();
        currSideBar = getSidebar(part);
        QCOMPARE(currSideBar->isSidebarVisible(), sideBarState[activeTabIndex]);
        currBottomBar = getBottombar(part);
        QCOMPARE(currBottomBar->isVisible(), bottomBarState[activeTabIndex]);
    }
}

void ReadingModeTest::storePriorReadingModeState(const Shell *shell)
{
    const int iter = shell->m_tabs.count();
    // Clear the variables that store the GUI element visibility states.
    clearStates();
    // Store the visibility state of the menubar.
    menuBarState = shell->menuBar()->isVisible();
    // Store the states of the toolbars.
    const QList<KToolBar *> toolBars = shell->toolBars();
    for (int i = 0; i < toolBars.count(); ++i) {
        toolBarState.append(toolBars[i]->isVisible());
    }
    // Store the states of sidebar and bottombar.
    for (int i = 0; i < iter; ++i) {
        Okular::Part *currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[i].part);
        sideBarState.append(getSidebar(currPart)->isSidebarVisible());
        bottomBarState.append(getBottombar(currPart)->isVisible());
    }
}

void ReadingModeTest::checkAfterReadingModeState(const Shell *shell)
{
    // Check the visibility state of the menubar compared with the state prior to Reading Mode activation.
    QCOMPARE(shell->menuBar()->isVisible(), menuBarState);
    // Check the QList<KToolBar *> element count as not changed after Reading Mode de-activation.
    QCOMPARE(shell->toolBars().count(), toolBarState.count());
    // Check the visibility states of the toolbars compared with the states prior to Reading Mode activation.
    const QList<KToolBar *> toolBars = shell->toolBars();
    for (int i = 0; i < toolBars.count(); ++i) {
        QCOMPARE(toolBars[i]->isVisible(), toolBarState[i]);
    }

    const int iter = shell->m_tabs.count();
    // Check that the number of tabs have not changed after Reading Mode de-activation.
    QCOMPARE(sideBarState.count(), iter);
    QCOMPARE(bottomBarState.count(), iter);

    Okular::Part *currPart = nullptr;
    Sidebar *sideBar = nullptr;
    QWidget *bottomBar = nullptr;
    // Compare the visibility states of the sidebar and bottombar compared with their states prior to Reading Mode activation.
    for (int i = 0; i < iter; ++i) {
        currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[i].part);
        sideBar = getSidebar(currPart);
        bottomBar = getBottombar(currPart);
        QCOMPARE(sideBar->isSidebarVisible(), sideBarState[i]);
        QCOMPARE(bottomBar->isVisible(), bottomBarState[i]);
    }
}

void ReadingModeTest::checkReadingModeState(Shell *shell)
{
    // Menubar should be hidden when Reading Mode is activated.
    QCOMPARE(shell->menuBar()->isVisible(), false);
    // All toolbars should be hidden when Reading Mode is activated.
    const QList<KToolBar *> toolBars = shell->toolBars();
    for (int i = 0; i < toolBars.count(); ++i) {
        QCOMPARE(toolBars[i]->isVisible(), false);
    }
    // Sidebar should be hidden and the bottombar should be visible when Reading Mode is activated.
    const int iter = shell->m_tabs.count();
    for (int i = 0; i < iter; ++i) {
        Okular::Part *currPart = dynamic_cast<Okular::Part *>(shell->m_tabs[i].part);
        shell->setActiveTab(i);
        QTest::qWait(750);
        Sidebar *currSideBar = getSidebar(currPart);
        QCOMPARE(currSideBar->isSidebarVisible(), false);
        QWidget *currBottomBar = getBottombar(currPart);
        QCOMPARE(currBottomBar->isVisible(), true);
    }
}

void ReadingModeTest::clearStates()
{
    toolBarState.clear();
    menuBarState = false;
    sideBarState.clear();
    bottomBarState.clear();
}

void ReadingModeTest::cleanup()
{
    Shell *s;
    while ((s = findShell())) {
        delete s;
    }
}

QTEST_MAIN(ReadingModeTest)
#include "readingmodetest.moc"
