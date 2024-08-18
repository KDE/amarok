/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

import QtQuick 2.15
import QtQuick.Dialogs 1.3
import org.kde.kirigami 2.14 as Kirigami


Rectangle {
    id: root

    default property alias contents: content.data
    property alias title: header.title
    property string name: "Nameless Applet"
    property string appletId
    property url iconSource
    property bool collapsed: false
    property bool configEnabled: false
    property real spacing: Kirigami.Units.smallSpacing
    property real padding: spacing
    property real contentHeight: content.childrenRect.height
    property Dialog configDialog: null
    readonly property AppletHeader appletHeader: header
    readonly property SystemPalette palette: palette

    radius: Kirigami.Units.smallSpacing
    border.width: 2
    border.color: palette.mid
    color: "transparent"
    clip: true
    height: content.height + header.height + 2 * padding + !collapsed * spacing

    function imageUrl(filename) {
        return AppletModel.imageUrl(root.appletId, filename);
    }

    onCollapsedChanged: AppletModel.setAppletCollapsed(appletId, collapsed)
    onContentHeightChanged: AppletModel.setAppletContentHeight(appletId, contentHeight)

    AppletHeader {
        id: header

        title: root.name
        iconSource: root.iconSource
    }

    Item {
        id: content

        anchors {
            top: header.bottom
            left: root.left
            right: root.right
            topMargin: root.spacing
            leftMargin: root.padding
            rightMargin: root.padding
        }

        height: root.collapsed ? 0 : root.contentHeight
        clip: true

        Behavior on height {
            enabled: !resizeMouseArea.pressed
            NumberAnimation { duration: 350 }
        }
    }

    MouseArea {
        id: resizeMouseArea

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: root.padding
        Rectangle {
            visible: parent.enabled
            anchors.fill: parent
            color: Kirigami.Theme.highlightColor
            radius: Kirigami.Units.smallSpacing
        }
        enabled: root.configEnabled
        cursorShape: enabled ? Qt.SizeVerCursor : Qt.ArrowCursor
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        onMouseYChanged: {
            if(pressed) {
                root.contentHeight = Math.max(Kirigami.Units.largeSpacing, root.contentHeight + mouseY);
            }
        }
    }

    SystemPalette {
        id: palette
    }
}
