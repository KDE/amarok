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
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami


MouseArea {
    id: root

    property alias name: label.text
    property string appletId
    property var toolbar
    property var flickable
    property bool configEnabled: !!toolbar ? toolbar.configEnabled : false
    property bool held: false

    height: toolbar.height - Kirigami.Units.smallSpacing;
    anchors.verticalCenter: parent.verticalCenter
    implicitWidth: label.implicitWidth + Kirigami.Units.smallSpacing * 2

    acceptedButtons: Qt.LeftButton
    hoverEnabled: true
    drag.target: held ? content : undefined
    drag.axis: Drag.XAxis
    cursorShape: configEnabled ? held ? Qt.ClosedHandCursor : Qt.PointingHandCursor : Qt.ArrowCursor
    pressAndHoldInterval: 200;

    onPressAndHold: if (configEnabled) { held = true; toolbarAppletRow.interactive = false; height = toolbar.height; }
    onReleased: { toolbarAppletRow.interactive = true; held = false; height = toolbar.height - Kirigami.Units.smallSpacing; }
    onPressed: if (!configEnabled) flickable.scrollToApplet(appletId)
    onImplicitWidthChanged: toolbar.resizeApplets()

    DropArea {
        anchors {
            fill: parent
            leftMargin: Kirigami.Units.smallSpacing
            rightMargin: Kirigami.Units.smallSpacing
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
        radius: Kirigami.Units.smallSpacing / 2
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
            color: Kirigami.Theme.textColor

            anchors {
                fill: parent
                leftMargin: Kirigami.Units.smallSpacing
                rightMargin: Kirigami.Units.smallSpacing
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
