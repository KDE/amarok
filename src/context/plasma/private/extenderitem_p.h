/*
 * Copyright 2008 by Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef LIBS_PLASMA_EXTENDERITEM_P_H
#define LIBS_PLASMA_EXTENDERITEM_P_H

#include <QPointF>
#include <QPoint>
#include <QRect>
#include <QString>

class QGraphicsItem;
class QGraphicsWidget;
class QGraphicsLinearLayout;
class QGraphicsView;
class QTimer;

namespace Plasma
{
    class Applet;
    class ExtenderItem;
    class Extender;
    class IconWidget;
    class FrameSvg;

class ExtenderItemPrivate
{
    public:
        ExtenderItemPrivate(ExtenderItem *extenderItem, Extender *hostExtender);
        ~ExtenderItemPrivate();

        QRectF dragHandleRect();
        QRectF titleRect();
        bool leaveCurrentView(const QRect &rect);
        QRect screenRect();
        void toggleCollapse();
        void updateToolBox();
        void repositionToolbox();
        QPointF scenePosFromScreenPos(const QPoint &pos) const;
        Applet *hostApplet() const;
        void themeChanged();
        void sourceAppletRemoved();

        ExtenderItem *q;

        QGraphicsItem *widget;
        QGraphicsWidget *toolbox;
        QGraphicsLinearLayout *toolboxLayout;
        QGraphicsView *toplevel;

        Extender *previousTargetExtender;
        Extender *extender;
        Applet *sourceApplet;

        KConfigGroup config;

        FrameSvg *dragger;
        FrameSvg *background;

        IconWidget *collapseIcon;

        QMap<QString, QAction*> actions;

        QString title;
        QString name;

        uint extenderItemId;

        qreal dragLeft, dragTop, dragRight, dragBottom;
        qreal bgLeft, bgTop, bgRight, bgBottom;

        QPointF deltaScene;
        QPoint mousePos;

        bool mousePressed;
        bool mouseOver;

        QTimer *expirationTimer;

        static uint s_maxExtenderItemId;
};

}

#endif // LIB_PLASMA_EXTENDERITEM_P_H
