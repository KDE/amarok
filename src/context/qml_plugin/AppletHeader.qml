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


RowLayout {
    id: root

    property var applet: parent
    property alias title: label.text
    property alias iconSource: icon.source

    anchors {
        left: parent.left
        right: parent.right
        top: parent.top
        margins: parent.padding
    }
    height: collapseButton.height

    Image {
        id: icon

        height: collapseButton.height
        width: height
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    Label {
        id: label

        text: root.title
        fontSizeMode: Text.VerticalFit
        horizontalAlignment: Text.AlignHCenter
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }

    ToolButton {
        id: configButton

        visible: applet.configEnabled && !!applet.configDialog
        iconName: "preferences-other"
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

        onClicked: applet.configDialog.open()
    }
    ToolButton {
        id: collapseButton

        //TODO: Icons are not part of official standard. Maybe provide our own icons?
        iconName: !applet.collapsed ? "window-minimize" : "window-restore"
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

        onClicked: applet.collapsed = applet.collapsed ? false : true
    }
}
