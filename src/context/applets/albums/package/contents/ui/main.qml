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
import QtQuick.Controls 1.4
import QtQml.Models 2.15
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.albums 1.0

AmarokQml.Applet {
    id: applet

    TreeView {
        id: treeView

        anchors.fill: parent
        sortIndicatorVisible: false
        headerVisible: false
        model: AlbumsEngine.model
        selectionMode: SelectionMode.ExtendedSelection
        selection: ItemSelectionModel {
            id: selectionModel

            model: AlbumsEngine.model
        }

        itemDelegate: Row {
            Loader {
                active: !!model && !!model["albumCover"]
                height: parent.height
                width: height
                visible: active

                AmarokQml.PixmapItem {
                    source: !!model ? model["albumCover"] : undefined
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: styleData.selected ? applet.palette.highlightedText : applet.palette.text
                elide: styleData.elideMode
                text: styleData.value
                textFormat: Text.StyledText
            }
        }

        rowDelegate: Rectangle {
            property color backgroundColor: styleData.alternate ? applet.palette.alternateBase : applet.palette.base

            height: !!model && !!model["size"] ? model["size"].height : 0
            width: parent.width
            color: styleData.selected ? applet.palette.highlight : backgroundColor

            MouseArea {
                id: rowMouseArea

                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }

        TableViewColumn {
            title: i18n("Display")
            role: "display"
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: {
                var index = treeView.indexAt(mouse.x, mouse.y);
                if (!selectionModel.isSelected(index)) {
                    selectionModel.select(index, ItemSelectionModel.ClearAndSelect);
                }
                AlbumsEngine.showContextMenu(selectionModel.selectedIndexes, index);
            }
        }

        Component.onCompleted: {
            __mouseArea.pressAndHoldInterval=200;
        }

        Item {
            id: dragItem
            width: 1; height: 1
            Drag.hotSpot.x: 1
            Drag.hotSpot.y: 1
            Drag.supportedActions: Qt.CopyAction
            function updateMimedata() {
                Drag.mimeData={"text/plain": AlbumsEngine.getSelectedUrlList(selectionModel.selectedIndexes),
                               "text/uri-list": AlbumsEngine.getSelectedUrlList(selectionModel.selectedIndexes)}
            }

            Drag.dragType: Drag.Automatic
        }

        onPressAndHold: function() {
            dragItem.updateMimedata();
            dragItem.Drag.active=true;
        }
    }
}
