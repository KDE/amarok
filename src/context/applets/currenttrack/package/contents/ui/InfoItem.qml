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
import QtQuick.Layouts 1.3
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.currenttrack 1.0

ColumnLayout {
    id: root

    property alias title: titleLabel.text
    property alias album: albumLabel.text
    property alias artist: artistLabel.text

    spacing: Kirigami.Units.smallSpacing

    Label {
        id: titleLabel

        text: CurrentTrackEngine.track
        textFormat: Text.PlainText

        Layout.fillHeight: true
        Layout.fillWidth: true

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        color: Kirigami.Theme.textColor
        maximumLineCount: 1
        fontSizeMode: Text.Fit
        font.pointSize: 32
        minimumPointSize: 12
        elide: Text.ElideRight

        CopyArea {
            copyMarker: copyMarker
            targetText: titleLabel.text
        }
    }
    Label {
        id: artistLabel

        text: CurrentTrackEngine.artist
        textFormat: Text.PlainText

        Layout.fillHeight: true
        Layout.fillWidth: true

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        color: Kirigami.Theme.textColor
        maximumLineCount: 1
        fontSizeMode: Text.Fit
        font.pointSize: 32
        font.italic: true
        minimumPointSize: 12
        elide: Text.ElideRight

        CopyArea {
            copyMarker: copyMarker
            targetText: artistLabel.text
        }
    }
    Label {
        id: albumLabel

        text: CurrentTrackEngine.album
        textFormat: Text.PlainText

        Layout.fillHeight: true
        Layout.fillWidth: true

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        color: Kirigami.Theme.textColor
        maximumLineCount: 1
        fontSizeMode: Text.Fit
        font.pointSize: 32
        minimumPointSize: 12
        elide: Text.ElideRight

        CopyArea {
            copyMarker: copyMarker
            targetText: albumLabel.text
        }
    }
}
