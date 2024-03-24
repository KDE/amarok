/****************************************************************************************
 * Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

MouseArea {
    property Button copyMarker
    property string targetText

    anchors.fill: parent
    cursorShape: Qt.PointingHandCursor
    hoverEnabled: true

    function copy( str ) {
        copyEdit.text=str
        copyEdit.selectAll()
        copyEdit.copy()
    }

    onClicked:
    {
        copyMarker.animationRunning = true
        copy(targetText);
    }
    onExited:
    {
        copyMarker.opacity = 0
    }
    onEntered:
    {
        copyMarker.x = Math.min( parent.implicitWidth - 20, parent.width - 30 )
        copyMarker.opacity = 0.3
    }

    TextEdit {
        id: copyEdit
        visible: false
    }

    Button {
        id: copyMarker

        property alias animationRunning: clickAnimation.running

        anchors.top: parent.top
        width: 20
        height: 20
        z: -1
        opacity: 0

        icon.name: "edit-copy"
        flat: true

        SequentialAnimation {
            id: clickAnimation
            OpacityAnimator { target: copyMarker; from: 0.3; to: 0.9; duration: 200; }
            OpacityAnimator { target: copyMarker; from: 0.9; to: 0.3; duration: 200; }
        }
        Behavior on opacity { PropertyAnimation {} }
    }
}
