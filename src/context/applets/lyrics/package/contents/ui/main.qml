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
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.lyrics 1.0

AmarokQml.Applet {
    id: applet

    property bool autoScroll: true

    configDialog: ConfigDialog {}

    Loader {
        id: loader

        width: parent.width
        height: parent.height

        sourceComponent: LyricsEngine.text === "" && LyricsEngine.suggestions.length > 0 ? suggestionsComponent : textComponent

        BusyIndicator {
            anchors.centerIn: parent
            running: LyricsEngine.fetching
        }
    }

    Component {
        id: textComponent

        TextArea {
            id: textArea

            text: LyricsEngine.text
            font.pointSize: LyricsEngine.fontSize
            font.family: LyricsEngine.font
            wrapMode: TextEdit.Wrap
            horizontalAlignment: LyricsEngine.alignment
            verticalAlignment: TextEdit.AlignTop
            textFormat: TextEdit.AutoText
            readOnly: true

            Connections {
                target: LyricsEngine
                onPositionChanged: {
                    if (applet.autoScroll) {
                        var middle = textArea.contentHeight * LyricsEngine.position;
                        var top = middle - textArea.height / 2;
                        var maxTop = textArea.contentHeight - textArea.height;
                        var boundTop = Math.min(maxTop, Math.max(0, top));
                        textArea.flickableItem.contentY = boundTop;
                    }
                }
            }

            RowLayout {
                id: buttonRow

                y: applet.spacing
                x: parent.effectiveHorizontalAlignment === TextEdit.AlignRight ? applet.spacing : parent.viewport.width - width - applet.spacing

                Button {
                    Layout.alignment: Qt.AlignRight
                    checkable: true
                    checked: applet.autoScroll
                    iconName: "arrow-down"
                    tooltip: i18n("Toggle auto scroll")

                    onClicked: applet.autoScroll = !applet.autoScroll
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    iconName: "view-refresh"
                    tooltip: i18n("Reload lyrics")

                    onClicked: LyricsEngine.refetchLyrics()
                }
            }

            Label {
                anchors.centerIn: parent
                text: i18n("No lyrics found")
                visible: !textArea.text
                font.pointSize: LyricsEngine.fontSize
                font.family: LyricsEngine.font
            }
        }
    }

    // untested
    Component {
        id: suggestionsComponent

        ListView {
            model: LyricsEngine.suggestions
            header: Label {
                text: i18n("Suggestions: ") + LyricsEngine.suggestions.length
            }
            delegate: Item {
                width: parent.width

                Column {
                    width: parent.width

                    Label {
                        text: '<b>' + i18n("Title:") + '</b> ' + modelData[0]
                    }
                    Label {
                        text: '<b>' + i18n("Artist:") + '</b> ' + modelData[1]
                    }
                }
                MouseArea {
                    anchors.fill: parent

                    onClicked: LyricsEngine.fetchLyrics(modelData[0], modelData[1], modelData[2])
                }
            }
        }
    }
}
