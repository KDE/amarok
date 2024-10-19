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
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.lyrics 1.0


Dialog {
    id: dialog
    width: parent.width - Kirigami.Units.largeSpacing * 4
    height: parent.height - Kirigami.Units.largeSpacing * 4

    function accept() {
        LyricsEngine.fontSize = sizeBox.value;
        LyricsEngine.font = fontCombo.currentText;
        switch (alignmentCombo.currentIndex) {
            case 0:
                LyricsEngine.alignment = TextEdit.AlignLeft;
                break;
            case 1:
                LyricsEngine.alignment = TextEdit.AlignRight;
                break;
            case 2:
                LyricsEngine.alignment = TextEdit.AlignHCenter;
                break;
        }
    }

    onAccepted: accept()
    onApplied: accept()

    title: i18n("Lyrics config")
    standardButtons: Dialog.Ok | Dialog.Apply | Dialog.Cancel

    Column {
        width: parent.width

        RowLayout {
            width: parent.width

            Label {
                Layout.alignment: Qt.AlignLeft
                text: i18n("Font size:")
            }
            SpinBox {
                id: sizeBox

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                value: LyricsEngine.fontSize
                editable: true
            }
        }
        RowLayout {
            width: parent.width

            Label {
                Layout.alignment: Qt.AlignLeft
                text: i18n("Text alignment:")
            }
            ComboBox {
                id: alignmentCombo

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                model: [i18n("Align left"), i18n("Align right"), i18n("Align center")]
                currentIndex: {
                    switch (LyricsEngine.alignment) {
                        case TextEdit.AlignLeft:
                            return 0;
                        case TextEdit.AlignRight:
                            return 1;
                        case TextEdit.AlignHCenter:
                            return 2;
                    }
                    return 0;
                }
            }
        }
        RowLayout {
            width: parent.width

            Label {
                Layout.alignment: Qt.AlignLeft
                text: i18n("Font:")
            }
            ComboBox {
                id: fontCombo

                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                model: LyricsEngine.availableFonts()
                currentIndex: LyricsEngine.availableFonts().indexOf(LyricsEngine.font)
            }
        }
    }
}
