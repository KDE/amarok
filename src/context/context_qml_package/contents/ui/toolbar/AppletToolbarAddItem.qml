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


ListView {
    id: root

    Rectangle {
        z: -1
        anchors.fill: parent
        color: palette.window
        radius: Kirigami.Units.smallSpacing
        opacity: 0.9
    }
    Rectangle {
        z: 1
        color: "#00000000"
        anchors.fill: parent
        border.color: palette.windowText
        border.width: 1
        radius: Kirigami.Units.smallSpacing
        opacity: 0.9
    }

    orientation: ListView.Horizontal
    leftMargin: Kirigami.Units.smallSpacing
    rightMargin: Kirigami.Units.smallSpacing
    spacing: Kirigami.Units.smallSpacing

    model: AppletModel

    ScrollBar.horizontal: ScrollBar { id: scrollBar }

    delegate: Rectangle {
        readonly property bool appletEnabled: AppletProxyModel.enabledApplets.indexOf(appletId) != -1

        anchors {
            verticalCenter: parent.verticalCenter
        }
        height: root.height - 6 * Kirigami.Units.smallSpacing
        width: height
        radius: Kirigami.Units.smallSpacing
        color: delegateMouseArea.pressed ? palette.highlight : appletEnabled ? palette.highlight : "transparent"
        border.color: delegateMouseArea.containsMouse ? palette.highlight : "transparent"
        border.width: 2

        ColumnLayout {
            anchors.fill: parent

            Image {
                source: icon
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.margins: Kirigami.Units.smallSpacing
                sourceSize.width: 256
                sourceSize.height: 256
                fillMode: Image.PreserveAspectFit
            }
            Label {
                Layout.alignment: Qt.AlignBottom
                Layout.margins: Kirigami.Units.smallSpacing
                Layout.fillWidth: true
                color: Kirigami.Theme.textColor
                text: name
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        MouseArea {
            id: delegateMouseArea

            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton

            onClicked: AppletProxyModel.setAppletEnabled(appletId, !appletEnabled);
        }
    }

    SystemPalette {
        id: palette
    }
}
