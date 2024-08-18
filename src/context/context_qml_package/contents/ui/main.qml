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
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami
import "toolbar"

Item {
    id: root

    Component.onCompleted: Context.debug("Context created")

    ColumnLayout {
        anchors.fill: parent

        Flickable {
            id: appletFlickable

            signal scrollToApplet(string id)

            Layout.alignment: Qt.AlignTop
            Layout.fillHeight: true
            Layout.fillWidth: true
            contentWidth: scrollBar.visible ? width - scrollBar.width : width
            contentHeight: appletColumn.height
            flickableDirection: Flickable.VerticalFlick
            focus: true

            Keys.onUpPressed: scrollBar.decrease()
            Keys.onDownPressed: scrollBar.increase()

            ScrollBar.vertical: ScrollBar { id: scrollBar }

            Column {
                id: appletColumn

                width: parent.width
                spacing: Kirigami.Units.smallSpacing

                Repeater {
                    model: AppletProxyModel

                    delegate: Loader {
                        width: appletColumn.width
                        active: true
                        asynchronous: false

                        function initialize() {
                            setSource(mainscript, {
                                "name": name,
                                "appletId": appletId,
                                "iconSource": icon,
                                "collapsed": collapsed,
                                "contentHeight": contentHeight,
                                "configEnabled": Qt.binding(function() { return appletToolbar.configEnabled; } )
                            });
                        }

                        Component.onCompleted: initialize()

                        onStatusChanged: {
                            if (status == Loader.Error) {
                                Context.error("Error loading applet: " + appletId);
                                Context.error(sourceComponent.errorString());
                            }
                            if (status == Loader.Ready) {
                                Context.debug("Applet loaded: " + appletId);
                            }
                        }

                        Connections {
                            target: AppletProxyModel

                            onDataChanged: {
                                if (!!mainscript && mainscript != source) {
                                    Context.debug("Data changed for applet " + appletId);
                                    initialize();
                                }
                            }
                        }
                        Connections {
                            target: appletFlickable

                            onScrollToApplet: {
                                if (id == appletId) {
                                    appletFlickable.contentY = y;
                                    Context.debug("Scroll to applet: " + appletId);
                                }
                            }
                        }
                    }
                }
            }
        }
        AppletToolbarAddItem {
            id: appletToolbarAddItem

            Layout.fillWidth: true
            height: Kirigami.Units.iconSizes.enormous
            visible: appletToolbar.configEnabled
        }
        AppletToolbar {
            id: appletToolbar

            contextRoot: root
            addItem: appletToolbarAddItem
            flickable: appletFlickable
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
        }
    }
}
