/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                           *
 * Copyright (c) 2025 Tuomas Nurmi  <tuomas@norsumanageri.org>                          *
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

import QtQuick
import QtQuick.Controls
import QtQml.Models 2.15
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.albums 1.0
import org.kde.kirigami 2.14 as Kirigami


AmarokQml.Applet {
    id: applet

    TreeView {
        id: treeView
        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        model: AlbumsEngine.model
        selectionModel: ItemSelectionModel {
            id: selectionModel
            model: AlbumsEngine.model
        }
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onSingleTapped: {
                if (treeView.cellAtPosition(point.position) == Qt.point(-1,-1))
                    selectionModel.clear()
            }
        }

        delegate: TreeViewDelegate {
            implicitHeight: !!model && !!model["size"] ? model["size"].height : 0
            implicitWidth: applet.width - 2 * Kirigami.Units.smallSpacing

            MouseArea {
                // For some reason unknown to me, TableView.ExtendedSelection fails to work here, so corresponding
                // selection logic has been re-written here. I tried first based on TapHandler, but encountered some
                // seemingly unresolvable issues with first clicked item not getting selected.
                drag.target: dragItem
                drag {
                    onActiveChanged: {
                        if (drag.active )
                        {
                            if (!parent.selected)
                            {
                                treeView.selectionModel.select(treeView.index(row, column), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Current | ItemSelectionModel.Rows);
                                selectionModel.setCurrentIndex(treeView.index(row, column), ItemSelectionModel.NoUpdate)
                            }
                            dragItem.updateMimedata();
                            dragItem.Drag.active=true;
                        }
                    }
                }
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                anchors.fill: parent
                onClicked: (mouse)=> {
                    if( mouse && mouse.x >= parent.indicator.x && mouse.x <= parent.indicator.x+parent.indicator.width
                        && mouse.y >= parent.indicator.y && mouse.y <= parent.indicator.y+parent.indicator.height )
                    {
                        treeView.toggleExpanded(row)
                        return;
                    }
                    let treeIndex = treeView.index(row, column);
                    switch (mouse.modifiers) {
                        case Qt.ControlModifier:
                            if (parent.selected)
                                selectionModel.clearCurrentIndex()
                            else
                                selectionModel.setCurrentIndex(treeIndex, ItemSelectionModel.NoUpdate)

                            selectionModel.select(treeIndex, ItemSelectionModel.Toggle)
                        break;
                        case Qt.ShiftModifier:
                            if(selectionModel.currentIndex.row >= 0 )
                            {
                                for(let i=Math.min(row, selectionModel.currentIndex.row); i<Math.max(row, selectionModel.currentIndex.row); i++)
                                {
                                    selectionModel.select(treeView.index(i, column), ItemSelectionModel.Select)
                                }
                            }
                            selectionModel.setCurrentIndex(treeIndex, ItemSelectionModel.NoUpdate)
                        break;
                        default:
                            treeView.toggleExpanded(row)
                            treeView.selectionModel.select(treeIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Current | ItemSelectionModel.Rows);
                            selectionModel.setCurrentIndex(treeIndex, ItemSelectionModel.NoUpdate)
                        break;
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onSingleTapped: {
                    if (!parent.selected) {
                        selectionModel.select(treeView.index(row, column), ItemSelectionModel.ClearAndSelect);
                        selectionModel.setCurrentIndex(treeView.index(row, column), ItemSelectionModel.NoUpdate)
                    }
                    AlbumsEngine.showContextMenu(selectionModel.selectedIndexes, treeView.index(row, column));
                }
            }

            contentItem: Row {
                spacing: Kirigami.Units.mediumSpacing
                width: parent.width

                Loader {
                    id: albumCover
                    active: !!model && !!model["albumCover"]
                    height: parent.height
                    width: height
                    visible: active

                    AmarokQml.PixmapItem {
                        source: !!model ? model["albumCover"] : undefined
                    }
                }

                Text {
                    anchors.verticalCenter:  parent.verticalCenter
                    color: parent.parent.selected ? applet.palette.highlightedText : applet.palette.text
                    text: model.display
                    textFormat: Text.StyledText
                }
            }

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
    }
}
