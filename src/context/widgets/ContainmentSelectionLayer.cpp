/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

#include "ContainmentSelectionLayer.h"

#include <plasma/paintutils.h>

#include <KColorScheme>

#include <QColor>
#include <QIcon>
#include <QFont>
#include <QPainter>
#include <QPen>

#define ICON_SIZE 60

ContainmentSelectionLayer::ContainmentSelectionLayer( QGraphicsItem *parent )
    : QGraphicsItem( parent )
    , m_mouseHover( 0 )
{
    m_containment = static_cast<Plasma::Containment *>( parent );
    setAcceptsHoverEvents( true );
    m_zoomInText = new QGraphicsSimpleTextItem( i18n( "Zoom In" ), this );
    
    QFont font;

    font.setBold( true );
    font.setStyleHint( QFont::Times );
    font.setPointSize( font.pointSize() + 10 );
    font.setStyleStrategy( QFont::PreferAntialias );

    m_zoomInText->setFont( font );
    m_zoomInText->setBrush( Qt::white );
        
    m_zoomInText->hide();
    m_zoomInIcon = new KIcon( "zoom-in" );
}

QRectF
ContainmentSelectionLayer::boundingRect() const
{
    qreal left, top, right, bottom;
    m_containment->getContentsMargins( &left, &top, &right, &bottom );
    return QRectF( m_containment->boundingRect().adjusted( left, top, right, bottom ) );
}

void
ContainmentSelectionLayer::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event );
    
    m_mouseHover = true;
    m_zoomInText->show();
    update();
}

void
ContainmentSelectionLayer::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event );
    
    m_mouseHover = false;
    m_zoomInText->hide();
    update();
}

void
ContainmentSelectionLayer::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event );
    
    m_mouseHover = false;
    m_zoomInText->hide();
    emit zoomRequested( m_containment, Plasma::ZoomIn );
}

void
ContainmentSelectionLayer::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option );
    Q_UNUSED( widget );
    
    QFontMetricsF fm( m_zoomInText->font() );
    m_zoomInText->setPos( boundingRect().width() / 2 - fm.boundingRect( m_zoomInText->text() ).width() / 2,
                          boundingRect().height() / 2 );
    painter->save();
    QColor color = KColorScheme( QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme() ).background().color();
   
    
    painter->setRenderHint( QPainter::Antialiasing );
    
    if( m_mouseHover )
    {        
        m_zoomInIcon->paint( painter, boundingRect().width() / 2 - ICON_SIZE / 2 ,
                             boundingRect().height() / 2 - ICON_SIZE, ICON_SIZE, ICON_SIZE,
                             Qt::AlignCenter, QIcon::Disabled );
        painter->setOpacity( 0.3 );
        painter->setPen( QPen( Qt::gray, 1) );
    }
    else
    {
        painter->setOpacity( 0 );
    }
    
    color.setAlpha( 200 );
    painter->setBrush( color );
    
    painter->drawRect( boundingRect() );
    
    painter->restore();
}
