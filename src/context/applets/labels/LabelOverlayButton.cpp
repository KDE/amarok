/****************************************************************************************
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#include "LabelOverlayButton.h"

#include <KIconEffect>
#include <KIconLoader>

#include <QPainter>


LabelOverlayButton::LabelOverlayButton( QGraphicsItem *parent )
    : QGraphicsItem( parent ),
    m_iconEffect( 0 ),
    m_size( 8 )
{
    setAcceptHoverEvents( true );
    
    m_iconEffect = new KIconEffect();
}

LabelOverlayButton::~LabelOverlayButton()
{
    if( m_iconEffect )
        delete m_iconEffect;
}

void
LabelOverlayButton::setPixmap( const QPixmap& pixmap )
{
    m_pixmap = pixmap;

    if( isUnderMouse() )
        m_scaledPixmap = m_iconEffect->apply( m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation ), KIconLoader::Desktop, KIconLoader::ActiveState );
    else
        m_scaledPixmap = m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation );
}

QPixmap
LabelOverlayButton::pixmap()
{
    return m_pixmap;
}

void
LabelOverlayButton::setSize( int size )
{
    m_size = size;
    
    if( isUnderMouse() )
        m_scaledPixmap = m_iconEffect->apply( m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation ), KIconLoader::Desktop, KIconLoader::ActiveState );
    else
        m_scaledPixmap = m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation );
}

int
LabelOverlayButton::size()
{
    return m_size;
}

void
LabelOverlayButton::updateHoverStatus()
{
    if( isUnderMouse() )
        m_scaledPixmap = m_iconEffect->apply( m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation ), KIconLoader::Desktop, KIconLoader::ActiveState );
    else
        m_scaledPixmap = m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation );
}

void
LabelOverlayButton::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    
    m_scaledPixmap = m_iconEffect->apply( m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation ), KIconLoader::Desktop, KIconLoader::ActiveState );
    update();
}

void
LabelOverlayButton::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    
    m_scaledPixmap = m_pixmap.scaledToHeight( m_size, Qt::SmoothTransformation );
    update();
}

QRectF
LabelOverlayButton::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), m_scaledPixmap.size() );
}

void
LabelOverlayButton::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->drawPixmap( 0, 0, m_scaledPixmap );
}
    