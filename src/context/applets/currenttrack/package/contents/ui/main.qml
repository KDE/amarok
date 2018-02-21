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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.currenttrack 1.0

AmarokQml.Applet {
    id: applet

    //album art
    Rectangle {
        id: cover

        color: "white"
        radius: Context.smallSpacing / 2
        border.width: 1
        border.color: applet.palette.light
        height: parent.height
        width: height

        AmarokQml.PixmapItem {
            id: iconItem

            anchors.fill: parent
            anchors.margins: parent.radius
            source: CurrentTrackEngine.cover

            onWidthChanged: CurrentTrackEngine.coverWidth = width

            //show standard empty cover if no data is available
            AmarokQml.PixmapItem {
                anchors.fill: parent
                source: Svg.renderSvg("file://" + applet.packagePath + "images/amarok-currenttrack.svg",
                                        "CurrentTrack",
                                        width,
                                        height,
                                        "album_old");
                visible: !iconItem.valid
            }
        }
    }
    ColumnLayout {
        anchors {
            left: cover.right
            leftMargin: applet.spacing
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop

            InfoItem {
                id: infoItem

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
            }
            AmarokQml.RatingItem {
                id: ratingItem

                height: Context.largeSpacing * 2
                width: height * 6
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                rating: CurrentTrackEngine.rating
                onClicked: CurrentTrackEngine.rating = newRating
            }
        }
        StatsItem {
            id: statsItem

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            height: Context.largeSpacing * 3
        }
    }
}
