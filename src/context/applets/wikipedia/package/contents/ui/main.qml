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
import QtWebEngine 1.10
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.wikipedia 1.0
import org.kde.kirigami 2.14 as Kirigami

AmarokQml.Applet {
    id: applet

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Button {
                icon.name: "go-previous"
                enabled: content.canGoBack
                Layout.alignment: Qt.AlignLeft
                ToolTip.text: i18n("Previous")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: {WikipediaEngine.message = ""; content.goBack() }
            }
            Button {
                icon.name: "go-next"
                enabled: content.canGoForward
                Layout.alignment: Qt.AlignLeft
                ToolTip.text: i18n("Next")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: {WikipediaEngine.message = ""; content.goForward() }
            }
            Button {
                icon.name: "view-refresh"
                enabled: !content.loading
                Layout.alignment: Qt.AlignLeft
                ToolTip.text: i18n("Refresh")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: {WikipediaEngine.message = ""; WikipediaEngine.reloadWikipedia() }
            }
            Button {
                icon.name: "media-playback-pause"
                enabled: !content.loading
                Layout.alignment: Qt.AlignLeft
                ToolTip.text: i18n("Pause") //TODO better string when not freeze
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true
                checkable: true

                onToggled: WikipediaEngine.pauseState = checked
            }
            Slider {
                from: 0
                value: content.scrollPosition.y
                to: content.contentsSize.height - content.height
                Layout.fillWidth: true
                onMoved: content.runJavaScript( "window.scrollTo(0, " + value + ");")
            }
            Button {
                icon.name: "filename-artist-amarok"
                Layout.alignment: Qt.AlignRight
                ToolTip.text: i18n("Artist")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: WikipediaEngine.selection = WikipediaEngine.Artist
            }
            Button {
                icon.name: "filename-composer-amarok"
                Layout.alignment: Qt.AlignRight
                ToolTip.text: i18n("Composer")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: WikipediaEngine.selection = WikipediaEngine.Composer
            }
            Button {
                icon.name: "filename-album-amarok"
                Layout.alignment: Qt.AlignRight
                ToolTip.text: i18n("Album")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: WikipediaEngine.selection = WikipediaEngine.Album
            }
            Button {
                icon.name: "filename-title-amarok"
                Layout.alignment: Qt.AlignRight
                ToolTip.text: i18n("Track")
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                hoverEnabled: true

                onClicked: WikipediaEngine.selection = WikipediaEngine.Track
            }
        }

        Kirigami.InlineMessage {
            visible: WikipediaEngine.message.length > 0
            text: WikipediaEngine.message
            type: Kirigami.MessageType.Warning
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            anchors.margins: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
        }

        WebEngineView {
            id: content
            visible: WikipediaEngine.message.length === 0

            Menu {
                id: wikipediaContextMenu
                MenuItem {
                    text: i18n("Previous")
                    icon.name: "go-previous"
                    enabled: content.canGoBack
                    onTriggered: content.triggerWebAction(WebEngineView.Backward)
                }
                MenuItem {
                    text: i18n("Next")
                    icon.name: "go-next"
                    enabled: content.canGoForward
                    onTriggered: content.triggerWebAction(WebEngineView.Forward)
                }
                MenuItem {
                    text: i18n("Refresh")
                    icon.name: "view-refresh"
                    enabled: !content.loading
                    onTriggered: content.triggerWebAction(WebEngineView.Reload)
                }
                MenuItem {
                    id: copyLink
                    text: i18n("Copy to Clipboard") // TODO: make "Copy Link to Clipboard" when not string freeze
                    icon.name: "edit-copy"
                    visible: false
                    onTriggered: content.triggerWebAction(WebEngineView.CopyLinkToClipboard)
                }
                // TODO: Possibly add Copy Image to Clipboard post-3.0
            }

            backgroundColor: "transparent"
            settings.showScrollBars: false

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignBottom

            onNavigationRequested: {
                if (request.navigationType == WebEngineNavigationRequest.LinkClickedNavigation) {
                    request.action = WebEngineNavigationRequest.IgnoreRequest
                    WikipediaEngine.url = request.url
                }
            }

            Connections {
                target: WikipediaEngine

                onPageChanged: content.loadHtml(WikipediaEngine.page, WikipediaEngine.url)
            }

            BusyIndicator {
                anchors.centerIn: parent
                z: 1
                running: WikipediaEngine.busy
            }

            onContextMenuRequested: function(request) {
                request.accepted = true;
                copyLink.visible = request.linkUrl.toString().length > 0
                wikipediaContextMenu.popup();
            }
        }
    }
}
