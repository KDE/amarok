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

#include "ToolBoxIcon.h"

#include "Debug.h"

#include <plasma/animator.h>
#include <plasma/paintutils.h>

#include <KColorScheme>

#include <QFont>

#define PADDING 15


static const float TOOLBOX_OPACITY = 0.4;

ToolBoxIcon::ToolBoxIcon( QGraphicsItem *parent )
    : Plasma::Icon( parent )
    , m_hovering( 0 )
    , m_animOpacity( TOOLBOX_OPACITY )
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
    m_text->show();
}

ToolBoxIcon::~ToolBoxIcon()
{}

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
        if( m_text->text().isEmpty() )
            m_text->setText( text() );
        
        const QFontMetricsF fm( m_text->font() );
        m_text->setPos( PADDING, size().height() / 2 - fm.boundingRect( m_text->text() ).height() / 2 );
    
        painter->save();
        
        QColor color = KColorScheme( QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme() ).background().color();

        painter->setBrush( color );
        painter->setRenderHint( QPainter::Antialiasing );                        
        painter->setOpacity( m_animOpacity );
        painter->setPen( QPen( Qt::gray, 1 ) );
        painter->drawPath( shape() );
        painter->restore();

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );
        painter->setPen( QPen( Qt::white, 2 ) );
        QSize innerRectSize(  size().width() - 7, size().height() - 7 );
        QPainterPath innerRect( Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 2.5, 2.5 ), innerRectSize ), 8 ) );
        painter->drawPath( innerRect );
        painter->restore();
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
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve, this, "animateHighlight" );

    Plasma::Icon::hoverEnterEvent( event );
}

void
ToolBoxIcon::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    
    m_hovering = false;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseOutCurve, this, "animateHighlight" );

    Plasma::Icon::hoverLeaveEvent( event );
}

void
ToolBoxIcon::animateHighlight( qreal progress )
{
    if( m_hovering )
        m_animOpacity = TOOLBOX_OPACITY + ( ( 1.0 - TOOLBOX_OPACITY ) * progress );
    else
        m_animOpacity = 1.0 - ( ( 1.0 - TOOLBOX_OPACITY ) * progress );

    if( progress >= 1.0 )
        m_animHighlightId = 0;

    update();
}


QPainterPath
ToolBoxIcon::shape() const
{
    if( Plasma::Icon::drawBackground() )
    {
        QSize shapeSize( size().width() - 2, size().height() - 2 );
        return Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 0.0, 0.0 ), shapeSize ), 10.0 );
    }

    return Plasma::Icon::shape();
}

void
ToolBoxIcon::setText( const QString &text )
{
    m_text->setText( text );
    update();
}

QString
ToolBoxIcon::text() const
{
    return m_text->text();
}


#include "ToolBoxIcon.moc"

