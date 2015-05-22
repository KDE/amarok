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

#include <Plasma/FrameSvg>

#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>

class TextScrollingWidgetPrivate
{
public:
    TextScrollingWidgetPrivate( TextScrollingWidget *parent )
        : width( 0.0 )
        , delta( 0.0 )
        , currentDelta( 0.0 )
        , drawBackground( false )
        , alignment( Qt::AlignHCenter )
        , textBackground( 0 )
        , textItem( new QGraphicsSimpleTextItem( parent ) )
        , q_ptr( parent )
    {}

    ~TextScrollingWidgetPrivate()
    {}

    void _delayedForwardAnimation()
    {
        Q_Q( TextScrollingWidget );
        if( q->isUnderMouse() )
        {
            q->setText( text );
            q->startAnimation( QAbstractAnimation::Forward );
        }
    }

    void drawRoundedRectAroundText( QPainter *p )
    {
        Q_Q( TextScrollingWidget );
        p->save();
        p->setRenderHint( QPainter::Antialiasing );

        if( !textBackground )
        {
            textBackground = new Plasma::FrameSvg( q );
            textBackground->setImagePath( QLatin1String("widgets/text-background") );
            textBackground->setEnabledBorders( Plasma::FrameSvg::AllBorders );
        }

        QRectF rect = textItem->boundingRect();
        rect = q->mapRectFromItem( textItem, rect );
        rect.adjust( -5, -5, 5, 5 );

        textBackground->resizeFrame( rect.size() );
        textBackground->paintFrame( p, rect.topLeft() );
        p->restore();
    }

    void setScrollingText( const QString &str )
    {
        text = str;
    }

    void setText( const QString &str )
    {
        text = str;
        textItem->setText( text );

        if( animation )
            animation.data()->stop();

    }

    qreal             width;          // box width
    qreal             delta;          // complete delta
    qreal             currentDelta;   // current delta
    bool              drawBackground; // whether to draw background svg
    QString           text;           // full sentence
    Qt::Alignment     alignment;      // horizontal text item alignment
    Plasma::FrameSvg *textBackground; // background svg for text
    QWeakPointer<QPropertyAnimation> animation; // scroll animation
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
    d->setScrollingText( text );
    updateGeometry();
}

void
TextScrollingWidget::setText( const QString &text )
{
    Q_D( TextScrollingWidget );
    d->setText( text );
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
    QGraphicsWidget::setGeometry( rect );

    // reset the animation and stuff
    QPropertyAnimation *animation = d->animation.data();
    if( animation )
    {
        animation->stop();
        animation->deleteLater();
        d->animation.clear();
    }

    QRectF textRect = mapFromParent( rect ).boundingRect();
    QFontMetricsF fm( font() );
    int textWidth = fm.width( d->text );
    d->width = textRect.width();
    d->delta = (textWidth > d->width) ? (textWidth - d->width) : 0;
    d->textItem->setText( fm.elidedText( d->text, Qt::ElideRight, d->width ) );

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
    Q_D( TextScrollingWidget );
    if( !isAnimating() && d->delta )
        QTimer::singleShot( 150, this, SLOT(_delayedForwardAnimation()) );
    e->accept();
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
    return ( d->animation && d->animation.data()->state() == QAbstractAnimation::Running );
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
    if( !d->animation )
        return;

    QPropertyAnimation *animation = d->animation.data();
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
            update();
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
        connect( animation, SIGNAL(finished()), this, SLOT(animationFinished()) );
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

void
TextScrollingWidget::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    QGraphicsWidget::paint( p, option, widget );
    Q_D( TextScrollingWidget );
    if( d->drawBackground )
        d->drawRoundedRectAroundText( p );
}

bool
TextScrollingWidget::isDrawingBackground() const
{
    Q_D( const TextScrollingWidget );
    return d->drawBackground;
}

void
TextScrollingWidget::setDrawBackground( bool enable )
{
    Q_D( TextScrollingWidget );
    d->drawBackground = enable;
}

