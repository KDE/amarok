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
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.similarartists 1.0

AmarokQml.Applet {
    id: applet

    ColumnLayout {
        anchors {
            left: parent.left
            leftMargin: applet.spacing
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        Label {
            id: artistLabel
            text: SimilarArtistsEngine.artist
            width: 100
        }
        ListView {
            id: similarArtistsList
            model: SimilarArtistsEngine.similarArtists
            Layout.fillWidth: true
            Layout.fillHeight: true
            delegate: Rectangle {
                height: 25
                width: parent.width
                color: "lightgray"
                Label { text: modelData; anchors.centerIn: parent }
            }
        }
    }
}
