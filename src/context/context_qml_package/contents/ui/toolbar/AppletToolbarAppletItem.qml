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

import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3


MouseArea {
    id: root

    property alias name: label.text
    property string appletId
    property var toolbar
    property var listView
    property bool configEnabled: !!toolbar ? toolbar.configEnabled : false
    property bool held: false

    height: held ? toolbar.height : toolbar.height - Context.smallSpacing
    anchors.verticalCenter: parent.verticalCenter
    implicitWidth: label.implicitWidth + Context.smallSpacing * 2

    acceptedButtons: Qt.LeftButton
    hoverEnabled: true
    drag.target: held ? content : undefined
    drag.axis: Drag.XAxis
    cursorShape: configEnabled ? held ? Qt.ClosedHandCursor : Qt.PointingHandCursor : Qt.ArrowCursor

    onPressAndHold: if (configEnabled) held = true
    onReleased: held = false
    onPressed: if (!configEnabled) listView.scrollToApplet(appletId)
    onImplicitWidthChanged: toolbar.resizeApplets()

    DropArea {
        anchors {
            fill: parent
            leftMargin: Context.smallSpacing
            rightMargin: Context.smallSpacing
        }

        onEntered: {
            AppletProxyModel.setAppletPlace(drag.source.appletId, AppletProxyModel.appletPlace(root.appletId));
        }
    }

    Rectangle {
        id: content

        readonly property string appletId: root.appletId

        border.width: 1
        color: root.pressed ? palette.highlight : palette.button
        border.color: root.containsMouse ? palette.highlight : palette.buttonText
        radius: Context.smallSpacing / 2
        anchors {
            horizontalCenter: root.horizontalCenter
            verticalCenter: root.verticalCenter
        }
        width: root.width
        height: root.height

        Drag.active: root.held
        Drag.source: root
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        states: State {
            when: root.held

            ParentChange {
                target: content
                parent: contextRoot
            }
            AnchorChanges {
                target: content
                anchors {
                    horizontalCenter: undefined
                    verticalCenter: undefined
                }
            }
        }

        Label {
            id: label

            anchors {
                fill: parent
                leftMargin: Context.smallSpacing
                rightMargin: Context.smallSpacing
            }

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            clip: true
        }
    }

    SystemPalette {
        id: palette
    }
}
