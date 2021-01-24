/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Benjamin Meyer <benjamin@csh.rit.edu>           *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003 by Luboš Luňák <l.lunak@kde.org>                   *
 *   Copyright (C) 2004 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2004 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_SHELL_H_
#define _OKULAR_SHELL_H_

#include <KActionCollection>
#include <QAction>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

#include <QtDBus> // krazy:exclude=includes

class KRecentFilesAction;
class KToggleAction;
class QTabWidget;
class KPluginFactory;

namespace Okular
{
class ReadingModeAction;
}

#ifndef Q_OS_WIN
namespace KActivities
{
class ResourceInstance;
}
#endif

struct TabState {
    TabState(KParts::ReadWritePart *p)
        : part(p)
        , printEnabled(false)
        , closeEnabled(false)
    {
        lftPnlVisBeforeReadingMode = false;
        btmBarVisBeforeReadingMode = false;
    }
    KParts::ReadWritePart *part;
    bool printEnabled;
    bool closeEnabled;
    bool lftPnlVisBeforeReadingMode;
    bool btmBarVisBeforeReadingMode;
};

/**
 * This is the application "Shell".  It has a menubar and a toolbar
 * but relies on the "Part" to do all the real work.
 *
 * @short Application Shell
 * @author Wilco Greven <greven@kde.org>
 * @version 0.1
 */
class Shell : public KParts::MainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.okular")

    friend class MainShellTest;
    friend class AnnotationToolBarTest;
    friend class ReadingModeTest;

public:
    /**
     * Constructor
     */
    explicit Shell(const QString &serializedOptions = QString());

    /**
     * Default Destructor
     */
    ~Shell() override;

    QSize sizeHint() const override;

    /**
     * Returns false if Okular component wasn't found
     **/
    bool isValid() const;

    bool openDocument(const QUrl &url, const QString &serializedOptions);
    template<typename T> static inline bool findActionInPart(KParts::ReadWritePart &part, const QString &actionName, T *&foundAction)
    {
        static_assert(std::is_base_of<QAction, T>::value || std::is_same<QAction, T>::value, "Found action should be of type QAction or inherit from QAction");

        KActionCollection *ac = nullptr;
        QAction *act = nullptr;

        ac = part.actionCollection();
        act = ac->action(actionName);

        if (ac && qobject_cast<T *>(act)) {
            foundAction = qobject_cast<T *>(act);
            return true;
        }

        return false;
    }

public Q_SLOTS:
    Q_SCRIPTABLE Q_NOREPLY void tryRaise();
    Q_SCRIPTABLE bool openDocument(const QString &urlString, const QString &serializedOptions = QString());
    Q_SCRIPTABLE bool canOpenDocs(int numDocs, int desktop);

protected:
    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfigGroup &) override;

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(const KConfigGroup &) override;

    /**
     * Expose internal functions for session restore testing
     */
    void savePropertiesInternal(KConfig *config, int num)
    {
        KMainWindow::savePropertiesInternal(config, num);
    }
    void readPropertiesInternal(KConfig *config, int num)
    {
        KMainWindow::readPropertiesInternal(config, num);
    }

    void readSettings();
    void writeSettings();
    void setFullScreen(bool);

    using KParts::MainWindow::setCaption;
    void setCaption(const QString &caption) override;

    bool queryClose() override;

    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;

    void closeEvent(QCloseEvent *e) override;

private Q_SLOTS:
    void fileOpen();

    void slotUpdateFullScreen();
    void slotShowMenubar();

    void openUrl(const QUrl &url, const QString &serializedOptions = QString());
    void showOpenRecentMenu();
    void closeUrl();
    void print();
    void setPrintEnabled(bool enabled);
    void setCloseEnabled(bool enabled);
    void setTabIcon(const QMimeType &mimeType);
    void handleDroppedUrls(const QList<QUrl> &urls);

    // Tab event handlers
    void setActiveTab(int tab);
    void closeTab(int tab);
    void activateNextTab();
    void activatePrevTab();
    void undoCloseTab();
    void moveTabData(int from, int to);

    void slotFitWindowToPage(const QSize pageViewSize, const QSize pageSize);

    void slotShowReadingMode();

Q_SIGNALS:
    void moveSplitter(int sideWidgetSize);

private:
    void setupAccel();
    void setupActions();
    QStringList fileFormats() const;
    void openNewTab(const QUrl &url, const QString &serializedOptions);
    void applyOptionsToPart(QObject *part, const QString &serializedOptions);
    void connectPart(QObject *part);
    int findTabIndex(QObject *sender) const;
    int findTabIndex(const QUrl &url) const;

private:
    void reloadAllXML();
    bool eventFilter(QObject *obj, QEvent *event) override;

    KPluginFactory *m_partFactory;
    KRecentFilesAction *m_recent;
    QStringList m_fileformats;
    bool m_fileformatsscanned;
    QAction *m_printAction;
    QAction *m_closeAction;
    KToggleAction *m_fullScreenAction;
    KToggleAction *m_showMenuBarAction;
    Okular::ReadingModeAction *m_showReadingModeAction;
    bool m_menuBarWasShown, m_toolBarWasShown;
    bool m_unique;
    QTabWidget *m_tabWidget;
    KToggleAction *m_openInTab;
    QList<TabState> m_tabs;
    QList<QUrl> m_closedTabUrls;
    QAction *m_nextTabAction;
    QAction *m_prevTabAction;
    QAction *m_undoCloseTab;

#ifndef Q_OS_WIN
    KActivities::ResourceInstance *m_activityResource;
#endif
    bool m_isValid;

public:
    static const QString SHOWLEFTPANELACTIONNAME;
    static const QString SHOWBOTTOMBARACTIONNAME;
};

#endif

// vim:ts=2:sw=2:tw=78:et
