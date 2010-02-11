/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#include "TextScrollingWidget.h"

#include "Debug.h"

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsWidget>
#include <QPainter>
#include <QTextDocument>
#include <QTimer>
#include <QPropertyAnimation>

#define DEBUG_PREFIX "TextScrollingWidget"


TextScrollingWidget::TextScrollingWidget( QGraphicsItem* parent )
    : QGraphicsTextItem( parent )
    , m_fm( 0 )
    , m_text( 0 )
    , m_delta( 0 )
    , m_currentDelta( 0. )
{
    setAcceptHoverEvents( true );
    document()->setDocumentMargin( 0 );
}

void
TextScrollingWidget::setBrush( const QBrush &brush )
{
    setDefaultTextColor( brush.color() );
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
    QPropertyAnimation *animation = m_animation.data();
    if( animation ) {
        animation->stop();
        m_animation.clear();
    }

    const QRect textRect = m_fm->boundingRect( m_text );
    m_delta = textRect.width() + 5 > m_rect.width()
            ? textRect.width() + 8 - m_rect.width() : 0;
    setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)m_rect.width() ) );
}

void
TextScrollingWidget::setText( const QString &text )
{
    setHtml( text );
}

QString
TextScrollingWidget::text() const
{
    return toPlainText();
}

void
TextScrollingWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // clip the widget.
    QRect rec( boundingRect().translated( m_currentDelta, 0 ).toRect() );
    rec.setWidth( m_rect.width() );

    painter->setClipRegion( QRegion( rec ) );
    QGraphicsTextItem::paint( painter, option, widget );
}

void
TextScrollingWidget::hoverEnterEvent( QGraphicsSceneHoverEvent* e )
{
    Q_UNUSED( e );
    if( !isAnimating() && m_delta )
    {
        DEBUG_BLOCK

        setText( m_text );
        QTimer::singleShot( 0, this, SLOT( startAnimation( QAbstractAnimation::Forward ) ) );
    }
}

bool
TextScrollingWidget::isAnimating()
{
    return ( m_animation.data() && m_animation.data()->state() == QAbstractAnimation::Running );
}

qreal
TextScrollingWidget::animationValue() const
{
    return m_currentDelta;
}

void
TextScrollingWidget::animationFinished()
{
    QPropertyAnimation *animation = m_animation.data();
    if( !animation )
        return;

    if( animation->property("direction") == QAbstractAnimation::Forward )
        QTimer::singleShot( 250, this, SLOT( startAnimation( QAbstractAnimation::Backward ) ) );
    else
    {
        // Scroll again if the mouse is still over.
        if( isUnderMouse() )
            QTimer::singleShot(250, this, SLOT( startAnimation( QAbstractAnimation::Forward ) ) );
        else
            setText( m_fm->elidedText ( m_text, Qt::ElideRight, (int)( m_rect.width() ) ) );
    }
}

void
TextScrollingWidget::startAnimation( QAbstractAnimation::Direction direction )
{
    QPropertyAnimation *animation = m_animation.data();
    if( !animation ) {
        animation = new QPropertyAnimation( this, "animationValue" );
        animation->setDuration( m_delta*15 );
        animation->setStartValue( 0.0 );
        animation->setEndValue( 1.0 );
        animation->setEasingCurve( QEasingCurve::InOutQuad );
        m_animation = animation;
    }
    else
        animation->pause();

    animation->setDirection(direction);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    connect( animation, SIGNAL( finished() ), this, SLOT( animationFinished() ) );
}

void
TextScrollingWidget::animate( qreal value )
{
    // DEBUG BLOCK
    if( m_animation.isNull() )
        return;

    if( m_animation.data()->property( "direction" ) == QAbstractAnimation::Forward )
        m_currentDelta = value * m_delta;
    else
        m_currentDelta = m_delta - ( value * m_delta );

    setPos( m_rect.left() - m_currentDelta, pos().y() );
}

#include "TextScrollingWidget.moc"
