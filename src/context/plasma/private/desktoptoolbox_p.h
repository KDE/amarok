/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_DESKTOPTOOLBOX_P_H
#define PLASMA_DESKTOPTOOLBOX_P_H

#include <QGraphicsItem>
#include <QObject>
#include <QTime>

#include <KIcon>

#include <private/toolbox_p.h>

#include "animator.h"

namespace Plasma
{

class Widget;
class EmptyGraphicsItem;
class DesktopToolBoxPrivate;

class DesktopToolBox : public ToolBox
{
    Q_OBJECT

public:
    explicit DesktopToolBox(QGraphicsItem *parent = 0);
    ~DesktopToolBox();
    QRectF boundingRect() const;
    QPainterPath shape() const;

    void showToolBox();
    void hideToolBox();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

protected slots:
    void animateCircle(qreal progress);
    void animateHighlight(qreal progress);
    void toolMoved(QGraphicsItem*);
    /**
     * show/hide the toolbox
     */
    void toggle();
private:
    DesktopToolBoxPrivate *d;
};

} // Plasma namespace
#endif // multiple inclusion guard

