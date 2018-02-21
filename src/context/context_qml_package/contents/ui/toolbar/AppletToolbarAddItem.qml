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

import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3


ScrollView {
    id: root

    verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    frameVisible: true

    ListView {
        id: listView

        anchors.margins: Context.smallSpacing
        orientation: ListView.Horizontal
        spacing: Context.smallSpacing

        model: AppletModel

        delegate: Rectangle {
            readonly property bool appletEnabled: AppletProxyModel.enabledApplets.indexOf(appletId) != -1

            height: root.height - 3 * Context.smallSpacing
            width: height
            radius: Context.smallSpacing
            color: delegateMouseArea.pressed ? palette.highlight : appletEnabled ? palette.highlight : "transparent"
            border.color: delegateMouseArea.containsMouse ? palette.highlight : "transparent"
            border.width: 2

            ColumnLayout {
                anchors.fill: parent

                Image {
                    source: "image://icon/" + icon
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.margins: Context.smallSpacing
                    sourceSize.width: width
                    sourceSize.height: height
                }
                Label {
                    Layout.alignment: Qt.AlignBottom
                    Layout.margins: Context.smallSpacing
                    Layout.fillWidth: true
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
}
