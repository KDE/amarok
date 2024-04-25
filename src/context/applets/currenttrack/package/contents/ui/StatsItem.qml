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
import org.kde.amarok.currenttrack 1.0
import org.kde.kirigami 2.14 as Kirigami

Item {
    id: root

    property alias playCount: playCountLabel.text
    property alias score: scoreLabel.text
    property alias lastPlayed: lastPlayedLabel.text

    Row {
        height: parent.height / 2

        Label {
            text: i18n("Play Count")
            width: root.width / 3
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            color: Kirigami.Theme.textColor
            fontSizeMode: Text.Fit
            font.pointSize: 32
            minimumPointSize: 9
        }
        Label {
            text: i18n("Score")
            width: root.width / 3
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            color: Kirigami.Theme.textColor
            fontSizeMode: Text.Fit
            font.pointSize: 32
            minimumPointSize: 9
        }
        Label {
            text: i18n("Last played")
            width: root.width / 3
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            color: Kirigami.Theme.textColor
            fontSizeMode: Text.Fit
            font.pointSize: 32
            minimumPointSize: 9
        }
    }
    Rectangle {
        width: parent.width
        height: parent.height / 2
        anchors.bottom: parent.bottom
        color: palette.base

        Row {
            height: parent.height

            Label {
                id: playCountLabel

                width: root.width / 3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                color: Kirigami.Theme.textColor
                fontSizeMode: Text.Fit
                font.pointSize: 32
                minimumPointSize: 9
                font.bold: true
                text: CurrentTrackEngine.timesPlayed
                elide: Text.ElideRight
            }
            Label {
                id: scoreLabel

                width: root.width / 3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                color: Kirigami.Theme.textColor
                fontSizeMode: Text.Fit
                font.pointSize: 32
                minimumPointSize: 9
                font.bold: true
                text: CurrentTrackEngine.score
                elide: Text.ElideRight
            }
            Label {
                id: lastPlayedLabel

                width: root.width / 3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                color: Kirigami.Theme.textColor
                fontSizeMode: Text.Fit
                font.pointSize: 32
                minimumPointSize: 9
                font.bold: true
                text: CurrentTrackEngine.lastPlayed
                elide: Text.ElideRight
            }
        }
    }

    SystemPalette {
        id: palette
    }
}
