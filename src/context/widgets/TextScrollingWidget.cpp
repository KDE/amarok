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

#include "core/support/Debug.h"

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
    , m_textFormat( Qt::PlainText )
    , m_delta( 0 )
    , m_currentDelta( 0. )
    , m_animDirection( QAbstractAnimation::Forward )
{
    setAcceptHoverEvents( true );
    document()->setDocumentMargin( 0 );
}

TextScrollingWidget::~TextScrollingWidget()
{
    delete m_fm;
}

void
TextScrollingWidget::setBrush( const QBrush &brush )
{
    setDefaultTextColor( brush.color() );
}

void
TextScrollingWidget::setScrollingText( const QString &text, const QRectF &rect )
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

    const int textWidth = m_fm->width( m_text );
    m_delta = textWidth + 5 > m_rect.width() ? textWidth + 8 - m_rect.width() : 0;
    setText( m_fm->elidedText( m_text, Qt::ElideRight, (int)m_rect.width() ) );
}

void
TextScrollingWidget::setText( const QString &text )
{
    if( requirePlainText() )
        setPlainText( text );
    else
        setHtml( text );
}

void
TextScrollingWidget::setTextFormat( Qt::TextFormat fmt )
{
    m_textFormat = fmt;
}

QString
TextScrollingWidget::text() const
{
    if( requirePlainText() )
        return toPlainText();
    else
        return toHtml();
}

bool
TextScrollingWidget::requirePlainText() const
{
    bool plain;
    switch( m_textFormat )
    {
    case Qt::RichText:
        plain = false;
        break;

    case Qt::AutoText:
        plain = !Qt::mightBeRichText( m_text );
        break;

    case Qt::PlainText:
    default:
        plain = true;
    }
    return plain;
}

Qt::TextFormat
TextScrollingWidget::textFormat() const
{
    return m_textFormat;
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
        m_animDirection = QAbstractAnimation::Forward;
        QTimer::singleShot( 0, this, SLOT( startAnimation() ) );
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
    {
        m_animDirection = QAbstractAnimation::Backward;
        QTimer::singleShot( 250, this, SLOT( startAnimation() ) );
    }
    else
    {
        // Scroll again if the mouse is still over.
        if( isUnderMouse() ) {
            m_animDirection = QAbstractAnimation::Forward;
            QTimer::singleShot(250, this, SLOT( startAnimation() ) );
        }
        else
            setText( m_fm->elidedText( m_text, Qt::ElideRight, (int)( m_rect.width() ) ) );
    }
}

void
TextScrollingWidget::startAnimation()
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

    animation->setDirection(m_animDirection);
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
