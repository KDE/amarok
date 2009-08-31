/****************************************************************************************
 * Copyright (c) 2007 Richard J. Moore <rich@kde.org>                                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "uiloader.h"

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QStringList>

#include <Plasma/BusyWidget>
#include <Plasma/CheckBox>
#include <Plasma/ComboBox>
#include <Plasma/FlashingLabel>
#include <Plasma/Frame>
#include <Plasma/GroupBox>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <Plasma/LineEdit>
#include <Plasma/Meter>
#include <Plasma/PushButton>
#include <Plasma/RadioButton>
#include <Plasma/ScrollBar>
#include <Plasma/SignalPlotter>
#include <Plasma/Slider>
#include <Plasma/SpinBox>
#include <Plasma/SvgWidget>
#include <Plasma/TabBar>
#include <Plasma/TextEdit>
#include <Plasma/ToolButton>
#include <Plasma/TreeView>
#include <Plasma/WebView>
#include <Plasma/VideoWidget>


QGraphicsWidget *createBusyWidget(QGraphicsWidget *parent) { return new Plasma::BusyWidget(parent); }
QGraphicsWidget *createCheckBox(QGraphicsWidget *parent) { return new Plasma::CheckBox(parent); }
QGraphicsWidget *createComboBox(QGraphicsWidget *parent) { return new Plasma::ComboBox(parent); }
QGraphicsWidget *createFlashingLabel(QGraphicsWidget *parent) { return new Plasma::FlashingLabel(parent); }
QGraphicsWidget *createFrame(QGraphicsWidget *parent) { return new Plasma::Frame(parent); }
QGraphicsWidget *createGroupBox(QGraphicsWidget *parent) { return new Plasma::GroupBox(parent); }
QGraphicsWidget *createIconWidget(QGraphicsWidget *parent) { return new Plasma::IconWidget(parent); }
QGraphicsWidget *createLabel(QGraphicsWidget *parent) { return new Plasma::Label(parent); }
QGraphicsWidget *createLineEdit(QGraphicsWidget *parent) { return new Plasma::LineEdit(parent); }
QGraphicsWidget *createMeter(QGraphicsWidget *parent) { return new Plasma::Meter(parent); }
QGraphicsWidget *createPushButton(QGraphicsWidget *parent) { return new Plasma::PushButton(parent); }
QGraphicsWidget *createRadioButton(QGraphicsWidget *parent) { return new Plasma::RadioButton(parent); }
QGraphicsWidget *createScrollBar(QGraphicsWidget *parent) { return new Plasma::ScrollBar(parent); }
QGraphicsWidget *createSignalPlotter(QGraphicsWidget *parent) { return new Plasma::SignalPlotter(parent); }
QGraphicsWidget *createSlider(QGraphicsWidget *parent) { return new Plasma::Slider(parent); }
QGraphicsWidget *createSpinBox(QGraphicsWidget *parent) { return new Plasma::SpinBox(parent); }
QGraphicsWidget *createSvgWidget(QGraphicsWidget *parent) { return new Plasma::SvgWidget(parent); }
QGraphicsWidget *createTabBar(QGraphicsWidget *parent) { return new Plasma::TabBar(parent); }
QGraphicsWidget *createTextEdit(QGraphicsWidget *parent) { return new Plasma::TextEdit(parent); }
QGraphicsWidget *createToolButton(QGraphicsWidget *parent) { return new Plasma::ToolButton(parent); }
QGraphicsWidget *createTreeView(QGraphicsWidget *parent) { return new Plasma::TreeView(parent); }
QGraphicsWidget *createVideoWidget(QGraphicsWidget *parent) { return new Plasma::VideoWidget(parent); }
QGraphicsWidget *createWebView(QGraphicsWidget *parent) { return new Plasma::WebView(parent); }

UiLoader::UiLoader()
{
    m_widgetCtors.insert("BusyWidget", createBusyWidget);
    m_widgetCtors.insert("CheckBox", createCheckBox);
    m_widgetCtors.insert("ComboBox", createComboBox);
    m_widgetCtors.insert("FlashingLabel", createFlashingLabel);
    m_widgetCtors.insert("Frame", createFrame);
    m_widgetCtors.insert("GroupBox", createGroupBox);
    m_widgetCtors.insert("IconWidget", createIconWidget);
    m_widgetCtors.insert("Label", createLabel);
    m_widgetCtors.insert("LineEdit", createLineEdit);
    m_widgetCtors.insert("Meter", createMeter);
    m_widgetCtors.insert("PushButton", createPushButton);
    m_widgetCtors.insert("RadioButton", createRadioButton);
    m_widgetCtors.insert("ScrollBar", createScrollBar);
    m_widgetCtors.insert("SignalPlotter", createSignalPlotter);
    m_widgetCtors.insert("Slider", createSlider);
    m_widgetCtors.insert("SpinBox", createSpinBox);
    m_widgetCtors.insert("SvgWidget", createSvgWidget);
    m_widgetCtors.insert("TabBar", createTabBar);
    m_widgetCtors.insert("TextEdit", createTextEdit);
    m_widgetCtors.insert("ToolButton", createToolButton);
    m_widgetCtors.insert("TreeView", createTreeView);
    m_widgetCtors.insert("VideoWidget", createVideoWidget);
    m_widgetCtors.insert("WebView", createWebView);
}

UiLoader::~UiLoader()
{
    kDebug();
}

QStringList UiLoader::availableWidgets() const
{
    return m_widgetCtors.keys();
}

QGraphicsWidget *UiLoader::createWidget(const QString &className, QGraphicsWidget *parent)
{
    widgetCreator w = m_widgetCtors.value(className, 0);
    if (w) {
        return (w)(parent);
    }

    return 0;
}


