/*
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.10 as Kirigami

Kirigami.Page {
    property alias document: pageArea.document
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0

    actions.main: Kirigami.Action {
        icon.name: pageArea.page.bookmarked ? "bookmark-remove" : "bookmarks-organize"
        checkable: true
        onCheckedChanged: pageArea.page.bookmarked = checked
        text: pageArea.page.bookmarked ? i18n("Remove bookmark") : i18n("Bookmark this page")
    }

    Okular.DocumentView {
        id: pageArea
        anchors.fill: parent

        onPageChanged: {
            bookmarkConnection.target = page
            actions.main.checked = page.bookmarked
        }
        onClicked: fileBrowserRoot.controlsVisible = !fileBrowserRoot.controlsVisible
    }

    // TODO KF 5.64 replace usage by upstream PlaceholderMessage
    PlaceholderMessage {
        visible: documentItem.url.toString().length === 0
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        anchors.centerIn: parent
        text: i18n("No document open")
        helpfulAction: openDocumentAction
    }

    Connections {
        id: bookmarkConnection
        target: pageArea.page
        onBookmarkedChanged: actions.main.checked = pageArea.page.bookmarked
    }
    QQC2.ProgressBar {
        id: bar
        z: 99
        visible: applicationWindow().controlsVisible
        height: Kirigami.Units.smallSpacing
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        value: documentItem.pageCount !== 0 ? ((documentItem.currentPage+1) / documentItem.pageCount) : 0
    }
}
