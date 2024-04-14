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
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.currenttrack 1.0

AmarokQml.Applet {
    id: applet

    //album art
    Loader {
        id: cover

        height: parent.height
        width: height

        sourceComponent: CurrentTrackEngine.hasValidCover ? coverComponent : emptyComponent
    }

    ColumnLayout {
        anchors {
            left: cover.right
            leftMargin: applet.spacing
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        GridLayout {
            id: artistRatingGrid
            Layout.preferredWidth: parent.width
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            flow: parent.width < ( infoItem.Layout.minimumWidth + ratingItem.Layout.minimumWidth ) * 8 ? GridLayout.TopToBottom : GridLayout.LeftToRight

            InfoItem {
                id: infoItem

                width: ( artistRatingGrid.flow == GridLayout.TopToBottom ? parent.width : parent.width / 2 )
                Layout.minimumWidth: 24
                Layout.maximumWidth: parent.width
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft
            }
            AmarokQml.RatingItem {
                id: ratingItem

                Layout.preferredWidth: height * 6
                Layout.preferredHeight: parent.height / 5
                Layout.maximumHeight: parent.height / 5
                Layout.minimumWidth: 24
                Layout.maximumWidth: parent.width / ( artistRatingGrid.flow == GridLayout.TopToBottom ? 1 : 2 )
                Layout.minimumHeight: 100

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                rating: CurrentTrackEngine.rating
                onClicked: CurrentTrackEngine.rating = newRating
            }
        }
        StatsItem {
            id: statsItem

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredHeight: parent.height / 5
        }
        visible: CurrentTrackEngine.track !== ""
    }

    RowLayout {
        anchors {
            left: cover.right
            leftMargin: applet.spacing
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        Label {
            id: nothingPLaying
            font.bold: true
            Layout.alignment: Qt.AlignCenter
            Layout.minimumWidth: parent.height / 3
            text: i18n("No track playing")
        }
        AmarokQml.PixmapItem {
            id: amarokLogo
            width: parent.width / 4
            height: width
            Layout.minimumWidth: parent.height / 3
            source: Svg.renderSvg(applet.imageUrl("amarok-currenttrack.svg"),
                                  "CurrentTrack",
                                  width,
                                  height,
                                  "amarok_logo");
        }
        visible: CurrentTrackEngine.track === ""
    }

    Component {
        id: coverComponent

        Rectangle {
            id: cover

            color: "white"
            radius: Kirigami.Units.smallSpacing / 2
            border.width: albumCoverMouse.containsMouse ? 3 : 1
            border.color: albumCoverMouse.containsMouse ? Kirigami.Theme.highlightColor : applet.palette.light

            AmarokQml.PixmapItem {
                id: iconItem

                anchors.fill: parent
                anchors.margins: parent.radius
                source: CurrentTrackEngine.cover
            }
            MouseArea {
                id: albumCoverMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: CurrentTrackEngine.displayCover()
            }
        }
    }
    Component {
        id: emptyComponent

        AmarokQml.PixmapItem {
            source: Svg.renderSvg(applet.imageUrl("amarok-currenttrack.svg"),
                                  "CurrentTrack",
                                  width,
                                  height,
                                  "album_old");
        }
    }
}
