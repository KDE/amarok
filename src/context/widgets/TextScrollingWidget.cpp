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
#include <QGraphicsWidget>
#include <QPainter>
#include <QTimer>

#include <Plasma/Animator>

#define DEBUG_PREFIX "TextScrollingWidget"


TextScrollingWidget::TextScrollingWidget( QGraphicsItem* parent )
    : QGraphicsSimpleTextItem( parent )
    , m_fm( 0 )
    , m_text( 0 )
    , m_delta( 0 )
    , m_currentDelta( 0. )
    , m_animfor( 0 )
    , m_animback( 0 )
    , m_animating( false )
{
    setAcceptHoverEvents( true );
}

void
TextScrollingWidget::setScrollingText( const QString text, QRectF rect )
{
 //   DEBUG_BLOCK
    if ( !m_fm )
        m_fm = new QFontMetrics( font() );
    m_rect = rect;
    m_text = text;
    m_currentDelta = 0;

    // reset the animation and stuff
    Plasma::Animator::self()->stopCustomAnimation( m_animback );
    Plasma::Animator::self()->stopCustomAnimation( m_animfor );
    m_animating = false ;
        
    m_delta = m_fm->boundingRect( m_text ).width() + 5 > m_rect.width() ? m_fm->boundingRect( m_text ).width() + 8 - m_rect.width() : 0;
    setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)m_rect.width() ) );
}

void
TextScrollingWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // clip the widget.
    painter->setClipRegion( QRegion( boundingRect().translated( m_currentDelta, 0 ).toRect() ) );
    QGraphicsSimpleTextItem::paint( painter, option, widget );
}

void
TextScrollingWidget::hoverEnterEvent( QGraphicsSceneHoverEvent* e )
{
    Q_UNUSED( e );    
    if ( !m_animating && m_delta )
    {
        DEBUG_BLOCK
        
        m_animating = true ;
        m_animfor = Plasma::Animator::self()->customAnimation( m_delta/2, m_delta*10, Plasma::Animator::EaseInCurve, this, "animate" );
        connect ( Plasma::Animator::self(), SIGNAL(customAnimationFinished ( int ) ), this, SLOT( animationFinished( int ) ) );
    }
}

void
TextScrollingWidget::animationFinished( int id )
{
    if ( id == m_animfor )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_animfor );
        QTimer::singleShot(250, this, SLOT(startAnimBack()));
    }
    else if ( id == m_animback )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_animback );
        m_animating = false;
    }
}

void
TextScrollingWidget::startAnimBack()
{
    m_animback = Plasma::Animator::self()->customAnimation( m_delta, m_delta*5, Plasma::Animator::EaseInCurve, this, "animateBack" );
}

void
TextScrollingWidget::animate( qreal anim )
{
//    DEBUG_BLOCK
    m_currentDelta = ( float )( anim * ( m_delta ) );
    setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)( m_rect.width() + m_currentDelta ) ) );
    setPos( m_rect.left() - m_currentDelta, pos().y() );
}

void
TextScrollingWidget::animateBack( qreal anim )
{
    // DEBUG_BLOCK
    m_currentDelta = m_delta - ( float )( anim * ( m_delta ) );
    setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)( m_rect.width() + m_currentDelta ) ) );
    setPos( m_rect.left() - m_currentDelta, pos().y() );
}

#include "TextScrollingWidget.moc"
