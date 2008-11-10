/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.

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

#ifndef PLASMA_TOOLBOX_P_H
#define PLASMA_TOOLBOX_P_H

#include <QGraphicsItem>
#include <QObject>

#include "animator.h"

class QAction;

namespace Plasma
{

//class Widget;
//class EmptyGraphicsItem;
class ToolBoxPrivate;

class ToolBox : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    /**
    * These flags represents what borders should be drawn
    */
    enum Corner {
        Top = 0,
        TopRight,
        TopLeft,
        Left,
        Right,
        Bottom,
        BottomRight,
        BottomLeft
    };

    explicit ToolBox(QGraphicsItem *parent = 0);
    ~ToolBox();

    /**
     * create a toolbox tool from the given action
     * @p action the action to associate hte tool with
     */
    void addTool(QAction *action);
    /**
     * remove the tool associated with this action
     */
    void removeTool(QAction *action);
    int size() const;
    void setSize(const int newSize);
    QSize iconSize() const;
    void  setIconSize(const QSize newSize);
    bool showing() const;
    void setShowing(const bool show);
    void setCorner(Corner corner);
    Corner corner() const;

    virtual void showToolBox() = 0;
    virtual void hideToolBox() = 0;
public Q_SLOTS:
    /**
     * re-show the toolbox, in case any tools have changed
     */
    void updateToolBox();
Q_SIGNALS:
    void toggled();

protected:
    QPoint toolPosition(int toolHeight);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    ToolBoxPrivate *d;
};

} // Plasma namespace
#endif // multiple inclusion guard

