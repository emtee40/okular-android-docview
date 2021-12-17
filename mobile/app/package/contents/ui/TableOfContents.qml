/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.2 as QQC2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.10 as Kirigami
import org.kde.kitemmodels 1.0

Kirigami.ScrollablePage {
    id: root

    header: Kirigami.AbstractApplicationHeader {
        topPadding: Kirigami.Units.smallSpacing / 2;
        bottomPadding: Kirigami.Units.smallSpacing / 2;
        rightPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing

        width: root.width
        Kirigami.SearchField {
            id: searchField
            width: parent.width
            placeholderText: i18n("Search...")
        }
    }
    ListView {
        width: parent.width
        model: KDescendantsProxyModel {
            model: documentItem.tableOfContents
            expandsByDefault: false
        }

        delegate: TreeItem {
           text: display
        }
    }
}
