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
 
#include "MainControlsButton.h"

#include "core/support/Debug.h"
#include "SvgHandler.h"

#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>

MainControlsButton::MainControlsButton( QGraphicsItem * parent )
    : QGraphicsItem( parent )
    , m_action( 0 )
    , m_mouseOver( false )
    , m_mouseDown( false )
{
    setAcceptHoverEvents ( true );
}


MainControlsButton::~MainControlsButton()
{
}

void
MainControlsButton::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * /*widget*/)
{
    // we're hovering over the button
    if( m_mouseOver )
    {
        // we're hovering over the button, and we're clicking
        if( m_mouseDown )
            painter->drawPixmap( 1, 1, The::svgHandler()->renderSvg( m_prefix, option->rect.width() - 2, option->rect.height() - 2, m_prefix ) );
        // we're not clicking
        else
            painter->drawPixmap( -2, -2, The::svgHandler()->renderSvg( m_prefix, option->rect.width() + 4, option->rect.height() + 4, m_prefix ) );
    }
    else
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( m_prefix, option->rect.width(), option->rect.height(), m_prefix ) );
}

void 
MainControlsButton::setSvgPrefix(const QString & prefix)
{
    m_prefix = prefix;
}

QRectF 
MainControlsButton::boundingRect() const
{
    return QRectF( 0.0, 0.0, 40.0, 40.0 );
}

void 
MainControlsButton::setAction(QAction * action)
{
    m_action = action;
}

void 
MainControlsButton::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED( event );
    m_mouseOver = true;
    update();
}

void 
MainControlsButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED( event );
    m_mouseOver = false;
    m_mouseDown = false;
    update();
}

void
MainControlsButton::keyPressEvent( QKeyEvent *event )
{
    DEBUG_BLOCK
    // Cancel the action if the escape key is pressed
    if( event->key() == Qt::Key_Escape )
    {
        m_mouseDown = false;
        update();
    }
    else
        event->ignore();
}

void
MainControlsButton::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    m_mouseDown = true;
    update();
}

void
MainControlsButton::mouseMoveEvent(QGraphicsSceneMouseEvent * event )
{
    const bool oldValue = m_mouseOver;
    m_mouseOver = contains( event->pos() );
    if( m_mouseOver != oldValue ) // Don't update needlessly
        update();
}

void 
MainControlsButton::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    Q_UNUSED( event );
    m_mouseDown = false;
    update();
    if( m_action != 0 && m_mouseOver )
        m_action->trigger();
}

QPainterPath 
MainControlsButton::shape() const
{
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}
