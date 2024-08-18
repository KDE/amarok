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
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.photos 1.0

AmarokQml.Applet {
    id: applet

    title: name + ": " + PhotosEngine.artist

    Flickable {
        anchors.fill: parent
        contentHeight: height
        contentWidth: contentRow.width

        Row {
            id: contentRow

            height: parent.height
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: PhotosEngine.photoTitles.length

                Item {
                    height: parent.height
                    width: image.width

                    Image {
                        id: image

                        anchors.top: parent.top
                        height: parent.height
                        width: height * sourceSize.width / sourceSize.height
                        source: PhotosEngine.photoUrls[index]
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onClicked: Context.runLink(PhotosEngine.pageUrls[index]);
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        text: PhotosEngine.error
        visible: PhotosEngine.Status === PhotosEngine.Error
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: PhotosEngine.Status === PhotosEngine.Fetching
    }
}
