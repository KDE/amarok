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
import org.kde.amarok.currenttrack 1.0

Column {
    id: root

    property real textSize: Context.largeSpacing
    property alias playCount: playCountLabel.text
    property alias score: scoreLabel.text
    property alias lastPlayed: lastPlayedLabel.text

    Row {
        width: parent.width

        Label {
            text: i18n("Play Count")
            width: parent.width / 3
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            font.pixelSize: root.textSize
        }
        Label {
            text: i18n("Score")
            width: parent.width / 3
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            font.pixelSize: root.textSize
        }
        Label {
            text: i18n("Last played")
            width: parent.width / 3
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 1
            font.pixelSize: root.textSize
        }
    }
    Rectangle {
        width: parent.width
        height: childrenRect.height
        color: palette.base

        Row {
            width: parent.width

            Label {
                id: playCountLabel

                width: parent.width / 3
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                font.pixelSize: root.textSize
                text: CurrentTrackEngine.timesPlayed
                elide: Text.ElideRight
            }
            Label {
                id: scoreLabel

                width: parent.width / 3
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                font.pixelSize: root.textSize
                text: CurrentTrackEngine.score
                elide: Text.ElideRight
            }
            Label {
                id: lastPlayedLabel

                width: parent.width / 3
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                font.pixelSize: root.textSize
                text: CurrentTrackEngine.lastPlayed
                elide: Text.ElideRight
            }
        }
    }

    SystemPalette {
        id: palette
    }
}
