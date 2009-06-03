/***************************************************************************
* copyright     (C) 2009 Simon Esneault <simon.esneault@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TextScrollingWidget.h"

#include "Debug.h"

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneHoverEvent>

#include <Plasma/Animator>

#define DEBUG_PREFIX "TextScrollingWidget"

TextScrollingWidget::TextScrollingWidget( QGraphicsItem* parent )
    : QGraphicsSimpleTextItem( parent )
    , m_fm( 0 )
    , m_text( 0 )
    , m_delta( 0 )
    , m_id( 0 )
{
    setAcceptHoverEvents( true );
}

void
TextScrollingWidget::setScrollingText( const QString text, QRectF rect )
{
    DEBUG_BLOCK
    if ( !m_fm )
        m_fm = new QFontMetrics( font() );
    m_rect = rect;
    m_text = text;
    debug () << "m_text "<<m_text;
    debug () << "m_rect "<<rect.width()<<" "<<rect.height()<<" "<<rect.left()<<" "<<rect.right();

    // we store the delta;
    m_delta = m_fm->boundingRect( m_text ).width() > m_rect.width() ? m_fm->boundingRect( m_text ).width() - m_rect.width() : 0;
    debug()<<" delta = " <<m_delta;

    setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)m_rect.width() ) );
}


void
TextScrollingWidget::hoverEnterEvent( QGraphicsSceneHoverEvent* e )
{
    Q_UNUSED( e );
    
    debug()<< "Entering hover ";
    startScrollAnimation();
}

void
TextScrollingWidget::hoverLeaveEvent( QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED( e );
    debug() << "Leaving hover ";
}

void
TextScrollingWidget::startScrollAnimation( void )
{
    DEBUG_BLOCK
  
    m_id = Plasma::Animator::self()->customAnimation( 50, 2, Plasma::Animator::LinearCurve, this, "animate");
  //  m_id =Plasma::Animator::self()->moveItem( this, Plasma::Animator::SlideOutMovement, QPoint(m_rect.left()-m_delta, y() ) );
}


void
TextScrollingWidget::animate( qreal anim )
{
    DEBUG_BLOCK
    debug()<<"ho yeah";
    setPos( pos()-QPointF((float)(m_delta/10.)*anim, 0 ) );
    update();

}

// we don't want to mess things
//  if ( x() == m_rect.left() )
// first move the widget ;
//  debug ()<< " m_delta*10 " << m_delta*10;
//debug ()<< " m_delta/2 " << m_delta/2;

#include "TextScrollingWidget.moc"
