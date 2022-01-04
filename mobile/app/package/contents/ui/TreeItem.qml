/*
 *  SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Layouts 1.4
import QtQuick.Controls 2.14 as QQC2
import org.kde.kirigami 2.13 as Kirigami
import org.kde.kitemmodels 1.0

/**
 * An item delegate for the TreeListView and TreeTableView components.
 *
 * It has the tree expander decoration but no content otherwise,
 * which has to be set as contentItem
 *
 */
Kirigami.AbstractListItem {
    id: listItem
    separatorVisible: false

    property alias decoration: decoration

    property var icon

    width: ListView.view.width - decoration.width - listItem.padding * 2

    data: [
        TreeViewDecoration {
            id: decoration
            anchors {
                left: parent.left
                top:parent.top
                bottom: parent.bottom
                leftMargin: listItem.padding
            }
            parent: listItem
            parentDelegate: listItem
            model: listItem.ListView.view ? listItem.ListView.view.model :null
        },
        Binding {
            target: contentItem.anchors
            property: "left"
            value: listItem.left
        },
        Binding {
            target: contentItem.anchors
            property: "leftMargin"
            value: decoration.width + listItem.padding * 2 + Kirigami.Units.smallSpacing
        }
    ]

    Keys.onLeftPressed: if (kDescendantExpandable && kDescendantExpanded) {
        decoration.model.collapseChildren(index);
    } else if (!kDescendantExpandable && kDescendantLevel > 0) {
        if (listItem.ListView.view) {
            const sourceIndex = decoration.model.mapToSource(decoration.model.index(index, 0));
            const newIndex = decoration.model.mapFromSource(sourceIndex.parent);
            listItem.ListView.view.currentIndex = newIndex.row;
        }
    }

    Keys.onRightPressed: if (kDescendantExpandable) {
        if (kDescendantExpanded && listItem.ListView.view) {
            ListView.view.incrementCurrentIndex();
        } else {
            decoration.model.expandChildren(index);
        }
    }

    onDoubleClicked: if (kDescendantExpandable) {
        decoration.model.toggleChildren(index);
    }
    icon: action ? action.icon.name || action.icon.source : undefined

    contentItem: Kirigami.Heading {
        wrapMode: Text.Wrap
        text: listItem.text
        level: 5
        width: listItem.ListView.view.width - (decoration.width + listItem.padding * 3 + Kirigami.Units.smallSpacing)
    }

    // FIXME: it should probably use leftInset property but Kirigami.AbstractListItem doesn't have it because can't import QQC2 more than 2.0
    background.anchors {
        left: listItem.left
        leftMargin: decoration.width + listItem.padding * 2
    }
}

