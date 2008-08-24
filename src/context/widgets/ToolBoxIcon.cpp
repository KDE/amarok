/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "Debug.h"
#include "ToolBoxIcon.h"

#include <plasma/animator.h>
#include <plasma/paintutils.h>

#include <KColorScheme>

#include <QFont>

ToolBoxIcon::ToolBoxIcon( QGraphicsItem *parent )
    : Plasma::Icon( parent )
    , m_hovering( 0 )
    , m_animHighlightFrame( 0.5 )
    , m_animHighlightId( 0 )
{
    m_text = new QGraphicsSimpleTextItem( this );
    QFont font;
    
    font.setBold( true );
    font.setStyleHint( QFont::Times );
    font.setPointSize( font.pointSize() - 2 );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    m_text->setFont( font );
    m_text->setBrush( Qt::white );
//     connect( this, SIGNAL( pressed( bool ) ), this, SLOT( mousePressed( bool ) ) );
}

ToolBoxIcon::~ToolBoxIcon()
{
    delete m_text;    
}

void
ToolBoxIcon::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->button() != Qt::LeftButton )
    {
        Plasma::Icon::mousePressEvent( event );
        return;
    }
    
    if( data( 0 ) != QVariant() )
    {
        DEBUG_LINE_INFO
        debug() << data( 0 ).toString();
        emit appletChosen( data( 0 ).toString() );
    }
    else
    {
        Plasma::Icon::mousePressEvent( event );
    }
}

void
ToolBoxIcon::mousePressed( bool pressed )
{
    DEBUG_BLOCK
    if( pressed && data( 0 ) != QVariant() )
    {
        debug() << data( 0 ).toString();
        emit appletChosen( data( 0 ).toString() );
    }
}

void
ToolBoxIcon::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    if( Plasma::Icon::drawBackground() )
    {
        if( m_text->text() == QString() )
            m_text->setText( Plasma::Icon::text() );
        
        m_text->setPos( 15, 0 );
    
        painter->save();
        
        QColor color = KColorScheme( QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme() ).background().color();
        color.setAlpha( 200 );

        painter->setBrush( color );
        painter->setRenderHint( QPainter::Antialiasing );                        
        painter->setOpacity( m_animHighlightFrame );
        painter->setPen( QPen( Qt::gray, 1) );
        painter->drawPath( shape() );
        painter->restore();
        
        m_text->show();        
    }
    else
        Plasma::Icon::paint( painter, option, widget );
}

QRectF
ToolBoxIcon::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), size() );
}


void
ToolBoxIcon::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );

    m_hovering = true;
    
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                   this, "animateHighlight" );
    QGraphicsItem::hoverEnterEvent( event );
}

void
ToolBoxIcon::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    
    m_hovering = false;
    
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseOutCurve,
                                                                   this, "animateHighlight" );
    QGraphicsItem::hoverLeaveEvent( event );
}

void
ToolBoxIcon::animateHighlight( qreal progress )
{
    if( m_hovering )
        m_animHighlightFrame = 0.5 + ( progress / 2 );
    else
        m_animHighlightFrame = 1.0 - ( progress / 2 );

    if( progress >= 1.0 )
        m_animHighlightId = 0;
    update();
}


QPainterPath
ToolBoxIcon::shape() const
{
    if( Plasma::Icon::drawBackground() )
    {
        return Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 0.0, 0.0 ),
                                                             QSize( 180, 24 ) ).adjusted( -2, -2, 2,2), 10.0 );
    }
    else
        return Plasma::Icon::shape();
}

#include "ToolBoxIcon.moc"
