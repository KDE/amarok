/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPER_ITEM_P_H
#define POPUPDROPPER_ITEM_P_H

#include <QtDebug>
#include <QColor>
#include <QFont>
#include <QTimeLine>

#include "PopupDropperItem.h"

class PopupDropperItemPrivate
{

public:

    PopupDropperItemPrivate( PopupDropperItem* parent );
    
    ~PopupDropperItemPrivate();

    QAction* action;
    QString text;
    QTimeLine hoverTimer;
    QString elementId;
    QGraphicsTextItem* textItem;
    QGraphicsRectItem* borderRectItem;
    QGraphicsSvgItem* svgItem;
    QGraphicsRectItem* hoverIndicatorRectItem;
    QGraphicsRectItem* hoverIndicatorRectFillItem;
    int borderWidth;
    int hoverIndicatorRectWidth;
    QFont font;
    bool submenuTrigger;
    QColor baseTextColor;
    QColor hoveredTextColor;
    QPen hoveredBorderPen;
    QBrush hoveredFillBrush;
    QBrush hoverIndicatorRectFillBrush;
    bool hoveredOver;
    bool customBaseTextColor;
    bool customHoveredTextColor;
    bool customHoveredBorderPen;
    bool customHoveredFillBrush;
    qreal subitemOpacity;
    QString file;
    QRect svgElementRect;
    QSvgRenderer* sharedRenderer;
    int horizontalOffset;
    int textOffset;
    bool separator;
    bool hasLineSeparatorPen;
    QPen lineSeparatorPen;
    PopupDropperItem::HoverIndicatorShowStyle hoverIndicatorShowStyle;
    PopupDropperItem::Orientation orientation;
    PopupDropperItem::TextProtection textProtection;
    PopupDropperItem::SeparatorStyle separatorStyle;
    PopupDropper* pd;

private:
    PopupDropperItem* q;
};

#endif

