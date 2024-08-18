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
import QtQuick.Layouts 1.15
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.lyrics 1.0
import org.kde.kirigami 2.14 as Kirigami

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

    RowLayout {
        id: buttonRow

        y: applet.spacing
        x: LyricsEngine.alignment === TextEdit.AlignRight ? applet.spacing : parent.width - width - applet.spacing - Kirigami.Units.largeSpacing * 2

        Button {
            Layout.alignment: Qt.AlignRight
            checkable: true
            checked: applet.autoScroll
            icon.name: "arrow-down"

            ToolTip.text: i18n("Toggle auto scroll")
            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            hoverEnabled: true

            onClicked: applet.autoScroll = !applet.autoScroll
        }
        Button {
            Layout.alignment: Qt.AlignRight
            icon.name: "view-refresh"
            ToolTip.text: i18n("Reload lyrics")
            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            hoverEnabled: true

            onClicked: LyricsEngine.refetchLyrics()
        }
    }

    Component {
        id: textComponent

        ScrollView {
            id: textView
            anchors.fill: parent

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
                            var top = middle - textView.height / 2;
                            var maxTop = textArea.contentHeight - textView.height + Kirigami.Units.largeSpacing;
                            var boundTop = Math.min(maxTop, Math.max(0, top));
                            textView.contentItem.contentY = boundTop;
                        }
                    }
                }


                Label {
                    anchors.centerIn: parent
                    text: i18n("No lyrics found")
                    color: Kirigami.Theme.textColor
                    visible: !textArea.text
                    font.pointSize: LyricsEngine.fontSize
                    font.family: LyricsEngine.font
                }
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
