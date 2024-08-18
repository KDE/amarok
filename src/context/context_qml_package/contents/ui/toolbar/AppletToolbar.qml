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
import QtQml.Models 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami


Rectangle {
    id: root

    readonly property alias configEnabled: configureButton.checked
    property var addItem
    property var flickable
    property var contextRoot

    function resizeApplets() {
        var items = [];
        var children = toolbarAppletRow.contentItem.visibleChildren;
        for (var i=0; i<children.length; i++) {
            if (!!children[i].status && children[i].status != Loader.Error)
                items.push(children[i]);
        }
        if (items.length > 0) {
            var space = toolbarAppletRow.width - (items.length - 1) * toolbarAppletRow.spacing;
            var threshold = space / items.length;
            var smallApplets = [];
            var largeApplets = [];
            for (var i=0; i<items.length; i++) {
                if (!!items[i] && !!items[i].status && items[i].status != Loader.Error)
                    smallApplets.push(items[i]);
            }
            while (smallApplets.length > 0 && space > 0) {
                var countSmallApplets = smallApplets.length;
                smallApplets.forEach(function (applet, index) {
                    if (applet.implicitWidth >= threshold) {
                        largeApplets.push(applet);
                        applet.width = applet.implicitWidth;
                        space -= applet.width;
                    }
                });
                smallApplets = smallApplets.filter(function (applet) { return largeApplets.indexOf(applet) == -1 });
                if (countSmallApplets == smallApplets.length) {
                    smallApplets.forEach(function (applet, index) {
                        applet.width = space / countSmallApplets;
                    });
                    return;
                }
                threshold = space / smallApplets.length;
            }
            if (smallApplets.length == 0) return;
            for (var i=0; i<smallApplets.length; i++) {
                var applet = smallApplets[i];
                applet.width = applet.implicitWidth;
            }
        }
    }

    height: configureButton.height + Kirigami.Units.smallSpacing
    radius: 4
    color: palette.mid

    ListView {
        id: toolbarAppletRow

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: configureButton.left
            leftMargin: spacing
            rightMargin: spacing
        }
        orientation: ListView.Horizontal
        spacing: Kirigami.Units.smallSpacing
        boundsBehavior: Flickable.StopAtBounds
        interactive: true
        clip: true

        model: AppletProxyModel

        delegate: Loader {
            active: true
            anchors.verticalCenter: parent.verticalCenter

            Component.onCompleted: {
                setSource("AppletToolbarAppletItem.qml", {
                    "name": name,
                    "appletId": appletId,
                    "toolbar": root,
                    "flickable": root.flickable,
                    "contextRoot": root.contextRoot
                });
                root.resizeApplets();
            }
            onStatusChanged: {
                if (status == Loader.Error) {
                    Context.error("Error loading toolbar item for applet: " + name);
                    Context.error(sourceComponent.errorString());
                }
            }
        }
        onWidthChanged: root.resizeApplets()
        onCountChanged: root.resizeApplets()
    }

    ToolButton {
        id: configureButton

        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.margins: (parent.height - height) / 2
        icon.name: "configure"
        checkable: true
        ToolTip.text: i18n( "Configure Applets..." )
        ToolTip.visible: hovered
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        hoverEnabled: true
    }

    SystemPalette {
        id: palette
    }
}
