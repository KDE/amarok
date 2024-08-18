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
import QtQuick.Dialogs 1.3 as Dialogs // QtQuick.Controls Dialogs only work properly with ApplicationWindow
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami
import org.kde.amarok.qml 1.0 as AmarokQml
import org.kde.amarok.analyzer 1.0

AmarokQml.Applet {
    id: applet

    BlockAnalyzer {
        id: blocky

        anchors.fill: parent
    }

    configDialog: Dialogs.Dialog {
        id: dialog

        title: i18nc("The title of the analyzer config dialog", "%1 config", applet.name)
        width: Kirigami.Units.largeSpacing * 25
        standardButtons: Dialogs.StandardButton.Ok | Dialogs.StandardButton.Apply | Dialogs.StandardButton.Cancel

        function applyChanges() {
            blocky.columnWidth = columnWidthSlider.trueValue;
            blocky.showFadebars = showFadebarsBox.checked;
            blocky.fallSpeed = fallSpeedSlider.value;
//             blocky.windowFunction = windowFunctionCombo.currentIndex;
            blocky.minimumFrequency = freqSlider.minValue;
            blocky.maximumFrequency = freqSlider.maxValue;
            blocky.sampleSize = sampleSizeSlider.sampleSize;
        }

        onAccepted: applyChanges();
        onApply: applyChanges();

        Column {
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                width: parent.width

                Label {
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Bar width:")
                }
                Slider {
                    id: columnWidthSlider

                    readonly property int trueValue: value + 1 // Slider buggy if you set "from"" to 1

                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    from: 0
                    to: 19
                    stepSize: 1
                    value: blocky.columnWidth - 1

                    ToolTip {
                        text: i18np("%1 pixel", "%1 pixels", columnWidthSlider.trueValue)
                        parent: columnWidthSlider.handle
                        x: columnWidthSlider.visualPosition * columnWidthSlider.width - width / 2
                        visible: columnWidthSlider.pressed
                    }
                }
            }
            RowLayout {
                width: parent.width

                Label {
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Show fadebars:")
                }
                CheckBox {
                    id: showFadebarsBox

                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    checked: blocky.showFadebars
                }
            }
            RowLayout {
                width: parent.width

                Label {
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Bars fall speed:")
                }
                Slider {
                    id: fallSpeedSlider

                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    from: 0
                    to: 4
                    stepSize: 1
                    value: blocky.fallSpeed
                }
            }
            RowLayout {
                width: parent.width

                Label {
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Frequency range:")
                }
                RangeSlider {
                    id: freqSlider

                    readonly property real exp: Math.pow(1000, 1.0/100)
                    readonly property real minValue: 20 * Math.pow(exp, first.value)
                    readonly property real maxValue: 20 * Math.pow(exp, second.value)

                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true

                    first.value: Math.log(blocky.minimumFrequency / 20) / Math.log(exp)
                    second.value: Math.log(blocky.maximumFrequency / 20) / Math.log(exp)
                    from: 0
                    to: 100

                    ToolTip {
                        text: i18n("%1 Hz", Math.round(freqSlider.minValue))
                        parent: freqSlider.first.handle
                        visible: freqSlider.first.pressed
                    }
                    ToolTip {
                        text: i18n("%1 Hz", Math.round(freqSlider.maxValue))
                        parent: freqSlider.second.handle
                        visible: freqSlider.second.pressed
                    }
                }
            }
            RowLayout {
                width: parent.width

                Label {
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Sample size:")
                }
                Slider {
                    id: sampleSizeSlider

                    readonly property int sampleSize: 1024 * Math.pow(2, value)

                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    from: 0
                    to: 4
                    stepSize: 1
                    value: {
                        if (blocky.sampleSize <= 1024)
                            return 0;
                        else if (blocky.sampleSize <=  2048)
                            return 1;
                        else if (blocky.sampleSize <= 4096)
                            return 2;
                        else if (blocky.sampleSize <= 8192)
                            return 3;
                        return 4;
                    }
                    snapMode: Slider.SnapAlways

                    ToolTip {
                        text: Number(sampleSizeSlider.sampleSize).toLocaleString(Qt.locale(), 'f', 0)
                        parent: sampleSizeSlider.handle
                        x: sampleSizeSlider.visualPosition * sampleSizeSlider.width - width / 2
                        visible: sampleSizeSlider.pressed
                    }
                }
            }
//             RowLayout {
//                 width: parent.width
//
//                 Label {
//                     Layout.alignment: Qt.AlignLeft
//                     text: i18n("Window function:")
//                 }
//                 ComboBox {
//                     id: windowFunctionCombo
//
//                     Layout.alignment: Qt.AlignRight
//                     Layout.fillWidth: true
//                     currentIndex: blocky.windowFunction
//                     model: [i18n("Rectangular"), i18n("Hann"), i18n("Nuttall"), i18n("Lanczos"), i18n("Sine")]
//                 }
//             }
        }
    }
}
