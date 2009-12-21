/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef MAINCONTROLSBUTTON_H
#define MAINCONTROLSBUTTON_H

#include <QGraphicsItem>

#include <QAction>
#include <QPainter>

/**
	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MainControlsButton : public QGraphicsItem
{
public:
    MainControlsButton( QGraphicsItem *parent );
    ~MainControlsButton();
    
    void setSvgPrefix( const QString &prefix );
    void setAction( QAction *action );

    void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
    QRectF boundingRect() const;
    QPainterPath shape() const;

protected:
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    virtual void keyPressEvent( QKeyEvent *event );
    virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );

private:
    QString  m_prefix;
    QAction *m_action;
    bool     m_mouseOver;
    bool     m_mouseDown;
};

#endif
