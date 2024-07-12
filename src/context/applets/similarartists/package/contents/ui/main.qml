/****************************************************************************************
 * Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or                        *
 * modify it under the terms of the GNU General Public License as                       *
 * published by the Free Software Foundation; either version 2 of                       *
 * the License or (at your option) version 3 or any later version                       *
 * accepted by the membership of KDE e.V. (or its successor approved                    *
 * by the membership of KDE e.V.), which shall act as a proxy                           *
 * defined in Section 14 of version 3 of the license.                                   *
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
import QtQuick.Layouts 1.3
import org.kde.kirigami 2.16 as Kirigami
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.similarartists 1.0

AmarokQml.Applet {
    id: applet

    RowLayout {
        anchors.fill: parent
        Layout.margins: applet.spacing

        ColumnLayout {
            Layout.margins: applet.spacing

            Label {
                id: artistLabel
                text: SimilarArtistsEngine.currentTarget ? i18nc("%1 is the artist for which similars are currently shown", "Similar to <b>%1</b>", SimilarArtistsEngine.currentTarget) : ""
                width: 100
            }
            ListView {
                id: similarArtistsList
                model: SimilarArtistsEngine.model
                Layout.fillHeight: true
                anchors.top: artistLabel.bottom
                anchors.bottom: parent.bottom
                interactive: false
                contentHeight: height
                boundsBehavior: Flickable.StopAtBounds
                width: applet.height
                delegate: Rectangle {
                    height: ( applet.height - artistLabel.height - Kirigami.Units.smallSpacing ) / ( SimilarArtistsEngine.maximumArtists + 1 )
                    anchors.left: parent.left
                    anchors.right: parent.right
                    color: palette.window
                    MouseArea {
                        height: parent.height
                        hoverEnabled: true
                        anchors.fill: parent
                        z: 1

                        Rectangle {
                            anchors.fill: parent
                            color: Kirigami.Theme.highlightColor
                            opacity: currentlyHighlighted.header === name ? 0.2 : parent.containsMouse ? 0.1 : 0.0
                            z: 1
                        }

                        onClicked:
                        {
                            currentlyHighlighted.header = name;
                            currentlyHighlighted.lastfmLink = link;
                            currentlyHighlighted.cover = Qt.binding(function() { return albumcover });
                            currentlyHighlighted.bio = Qt.binding(function() { return bio });
                            currentlyHighlighted.counts = Qt.binding(function() {
                                if( listeners.length == 0 )
                                    return ""
                                let str = i18nc( "Artist's listener and playcount from Last.fm", "%1 listeners<br>%2 plays", listeners, plays )
                                if( parseInt( ownplays ) > 0 )
                                    str += i18nc( "Number of user's own scrobbles of an artist on Last.fm, appended to previous string if not 0", "<br>%1 plays by you", ownplays )
                                return str
                            });
                            // unfortunately disabled in last.fm api since 2019 or so
                            // currentlyHighlighted.image = image;
                        }

                        ProgressBar {
                            id: matchBar
                            from: 0
                            to: 100
                            width: parent.width / 3
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value: match
                        }

                        Label {
                            id: nameLabel
                            text: name
                            anchors.left: matchBar.right
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
        Rectangle {
            id: currentlyHighlighted
            Layout.fillHeight: true
            Layout.fillWidth: true
            property alias header: headerLabel.text
            property alias bio: bioText.text
            property alias cover: albumCover.source
            property alias counts: countLabel.text
            property alias lastfmLink: lastfmLink.url
            color: palette.base
            radius: Kirigami.Units.smallSpacing / 2
            visible: header.length > 0

            Label {
                id: headerLabel
                anchors.topMargin: applet.spacing
                anchors.top: parent.top
                anchors.left: parent.left
                font.weight: Font.Bold
                width: parent.width / 3
            }
            RoundButton {
                id: lastfmLink
                visible: headerLabel.text.length > 0
                property string url
                anchors.left: parent.left
                anchors.top: headerLabel.bottom
                //icon.name: "lastfm"
                icon.source: "images/lastfm.png" //FIXME
                radius: Kirigami.Units.smallSpacing
                onClicked: if(url) { Qt.openUrlExternally(url) }
            }
            Label {
                id: countLabel
                anchors.top: lastfmLink.bottom
                anchors.left: parent.left
            }
            Image {
                id: albumCover
                anchors.top: countLabel.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                fillMode: Image.PreserveAspectFit
                MouseArea {
                    id: albumCoverMouse
                    anchors.fill: parent
                    onClicked: SimilarArtistsEngine.navigateToArtist( headerLabel.text )
                }
            }

            Flickable {
                clip: true
                anchors.left: albumCover.status == Image.Ready ? albumCover.right : headerLabel.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: applet.spacing
                contentHeight: bioText.height
                Label {
                    id: bioText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    wrapMode: Text.Wrap
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
        }
    }
    SystemPalette {
        id: palette
    }
}
