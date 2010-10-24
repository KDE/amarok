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

#define DEBUG_PREFIX "TextScrollingWidget"
#include "TextScrollingWidget.h"

#include "core/support/Debug.h"

#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QTimer>
#include <QPropertyAnimation>

class TextScrollingWidgetPrivate
{
public:
    TextScrollingWidgetPrivate( TextScrollingWidget *parent )
        : width( 0.0 )
        , delta( 0.0 )
        , currentDelta( 0.0 )
        , alignment( Qt::AlignHCenter )
        , textItem( new QGraphicsSimpleTextItem( parent ) )
        , q_ptr( parent )
    {}

    ~TextScrollingWidgetPrivate()
    {}

    void _delayedForwardAnimation()
    {
        Q_Q( TextScrollingWidget );
        if( q->isUnderMouse() )
            q->startAnimation( QAbstractAnimation::Forward );
    }

    qreal             width;          // box width
    qreal             delta;          // complete delta
    qreal             currentDelta;   // current delta
    QString           text;           // full sentence
    Qt::Alignment     alignment;      // horizontal text item alignment
    QWeakPointer<QPropertyAnimation> animation; // scroll animation

    // QGraphicsTextItem *textItem;
    QGraphicsSimpleTextItem *textItem;

private:
    TextScrollingWidget *const q_ptr;
    Q_DECLARE_PUBLIC( TextScrollingWidget )
};

TextScrollingWidget::TextScrollingWidget( QGraphicsWidget *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , d_ptr( new TextScrollingWidgetPrivate(this) )
{
    setAcceptHoverEvents( true );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    setFlags( flags() | QGraphicsItem::ItemClipsChildrenToShape );
}

TextScrollingWidget::~TextScrollingWidget()
{
    delete d_ptr;
}

void
TextScrollingWidget::setBrush( const QBrush &brush )
{
    Q_D( TextScrollingWidget );
    d->textItem->setBrush( brush );
}

void
TextScrollingWidget::setScrollingText( const QString &text )
{
    Q_D( TextScrollingWidget );

    // reset the animation and stuff
    QPropertyAnimation *animation = d->animation.data();
    if( animation ) {
        animation->stop();
        d->animation.clear();
    }

    updateGeometry();
    QFontMetricsF fm( font() );
    int textWidth = fm.width( text );
    d->width = size().width();
    d->text = text;
    d->delta = textWidth > d->width ? textWidth - d->width : 0;
    d->textItem->setText( fm.elidedText( text, Qt::ElideRight, d->width ) );
}

void
TextScrollingWidget::setText( const QString &text )
{
    Q_D( TextScrollingWidget );
    d->textItem->setText( text );
}

void
TextScrollingWidget::setAlignment( Qt::Alignment alignment )
{
    Q_D( TextScrollingWidget );
    d->alignment = alignment;
    updateGeometry();
}

void
TextScrollingWidget::setFont( const QFont &font )
{
    Q_D( TextScrollingWidget );
    d->textItem->setFont( font );
    QFontMetricsF fm( font );
    const int textWidth = fm.width( d->text );
    d->delta = textWidth > d->width ? textWidth - d->width + 5 : 0;
}

QBrush
TextScrollingWidget::brush() const
{
    Q_D( const TextScrollingWidget );
    return d->textItem->brush();
}

QFont
TextScrollingWidget::font() const
{
    Q_D( const TextScrollingWidget );
    return d->textItem->font();
}

QString
TextScrollingWidget::text() const
{
    Q_D( const TextScrollingWidget );
    return d->text;
}

void
TextScrollingWidget::setGeometry( const QRectF &rect )
{
    Q_D( TextScrollingWidget );
    prepareGeometryChange();
    QGraphicsWidget::setGeometry( rect );
    setPos( rect.topLeft() );

    QRectF textRect = mapFromParent( rect ).boundingRect();
    qreal textX( 0.0 );
    switch( d->alignment )
    {
    case Qt::AlignHCenter:
        textX = ( textRect.width() - d->textItem->boundingRect().width() ) / 2;
        break;
    default:
    case Qt::AlignLeft:
        textX = textRect.left();
        break;
    case Qt::AlignRight:
        textX = textRect.right() - d->textItem->boundingRect().width();
        break;
    }
    d->textItem->setPos( textX, textRect.top() );
}

void
TextScrollingWidget::hoverEnterEvent( QGraphicsSceneHoverEvent* e )
{
    Q_UNUSED( e );
    Q_D( TextScrollingWidget );
    if( !isAnimating() && d->delta )
    {
        // DEBUG_BLOCK
        setText( d->text );
        QTimer::singleShot( 250, this, SLOT(_delayedForwardAnimation()) );
    }
}

bool
TextScrollingWidget::isEmpty() const
{
    Q_D( const TextScrollingWidget );
    return d->text.isEmpty();
}

bool
TextScrollingWidget::isAnimating() const
{
    Q_D( const TextScrollingWidget );
    return ( d->animation.data() && d->animation.data()->state() == QAbstractAnimation::Running );
}

qreal
TextScrollingWidget::animationValue() const
{
    Q_D( const TextScrollingWidget );
    return d->currentDelta;
}

void
TextScrollingWidget::animationFinished()
{
    Q_D( TextScrollingWidget );
    QPropertyAnimation *animation = d->animation.data();
    if( !animation )
        return;

    if( animation->direction() == QAbstractAnimation::Forward )
    {
        startAnimation( QAbstractAnimation::Backward );
    }
    else
    {
        // Scroll again if the mouse is still over.
        if( isUnderMouse() )
            startAnimation( QAbstractAnimation::Forward );
        else
        {
            setScrollingText( d->text );
            d->animation.data()->deleteLater();
        }
    }
}

void
TextScrollingWidget::startAnimation( QAbstractAnimation::Direction direction )
{
    Q_D( TextScrollingWidget );
    QPropertyAnimation *animation = d->animation.data();
    if( !animation )
    {
        animation = new QPropertyAnimation( this, "animationValue" );
        animation->setDuration( d->delta*15 );
        animation->setStartValue( 0.0 );
        animation->setEndValue( 1.0 );
        animation->setEasingCurve( QEasingCurve::InOutQuad );
        d->animation = animation;
        connect( animation, SIGNAL( finished() ), this, SLOT( animationFinished() ) );
    }
    else
    {
        animation->stop();
    }

    animation->setDirection( direction );
    animation->start( QAbstractAnimation::KeepWhenStopped );
}

void
TextScrollingWidget::animate( qreal value )
{
    // DEBUG_BLOCK
    Q_D( TextScrollingWidget );
    if( d->animation.isNull() )
        return;

    d->currentDelta = -value * d->delta;
    d->textItem->setPos( d->currentDelta, 0 );
}

QSizeF
TextScrollingWidget::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_D( const TextScrollingWidget );
    QFontMetricsF fm( font() );
    switch( which )
    {
    case Qt::MinimumSize:
        return QSizeF( fm.width( d->text ) / 4.0, fm.height() );
    case Qt::MaximumSize:
        return QSizeF( -1, fm.height() + 8 );
    case Qt::MinimumDescent:
        return QSizeF( QGraphicsWidget::sizeHint(which, constraint).width(), fm.descent() );
    default:
        break;
    }
    return QSizeF( constraint.width(), fm.height() );
}

Qt::Alignment
TextScrollingWidget::alignment() const
{
    Q_D( const TextScrollingWidget );
    return d->alignment;
}

QRectF
TextScrollingWidget::boundingRect() const
{
    Q_D( const TextScrollingWidget );
    return mapRectFromItem( d->textItem, d->textItem->boundingRect() );
}

#include "TextScrollingWidget.moc"
