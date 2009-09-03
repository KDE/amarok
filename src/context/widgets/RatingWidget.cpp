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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/*
  Significant parts of this code is inspired and/or copied from
  KDE Nepomuk sources, available at kdelibs/nepomuk
*/

#include "RatingWidget.h"

#include "Debug.h"

#include "kratingpainter.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QKeyEvent>
#include <QtGui/QImage>
#include <QtGui/QIcon>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>

class RatingWidget::Private
{
public:
    Private()
        : rating(0),
          hoverRating(-1),
          pixSize( 16 ),
          showing( true ){
    }

    int rating;
    int hoverRating;
    int pixSize;

    bool showing;

    KRatingPainter ratingPainter;
};


RatingWidget::RatingWidget( QGraphicsItem* parent )
    : QGraphicsWidget( parent ),
      d( new Private() )
{
    setAcceptHoverEvents( true );
}


RatingWidget::~RatingWidget()
{
    delete d;
}

void
RatingWidget::show()
{
    d->showing = true;
}

void
RatingWidget::hide()
{
    d->showing = false;
}

void
RatingWidget::setCustomPixmap( const QPixmap& pix )
{
    d->ratingPainter.setCustomPixmap( pix );
    update();
}


void
RatingWidget::setIcon( const QIcon& icon )
{
    d->ratingPainter.setIcon( icon );
    update();
}


void
RatingWidget::setPixmapSize( int size )
{
    d->pixSize = size;
    updateGeometry();
}


int
RatingWidget::spacing() const
{
    return d->ratingPainter.spacing();
}


QIcon
RatingWidget::icon() const
{
    return d->ratingPainter.icon();
}


void
RatingWidget::setSpacing( int s )
{
    d->ratingPainter.setSpacing( s );
    update();
}


Qt::Alignment
RatingWidget::alignment() const
{
    return d->ratingPainter.alignment();
}


void
RatingWidget::setAlignment( Qt::Alignment align )
{
    d->ratingPainter.setAlignment( align );
    update();
}


Qt::LayoutDirection
RatingWidget::layoutDirection() const
{
    return d->ratingPainter.layoutDirection();
}


void
RatingWidget::setLayoutDirection( Qt::LayoutDirection direction )
{
    d->ratingPainter.setLayoutDirection( direction );
    update();
}


unsigned int
RatingWidget::rating() const
{
    return d->rating;
}


int
RatingWidget::maxRating() const
{
    return d->ratingPainter.maxRating();
}


bool
RatingWidget::halfStepsEnabled() const
{
    return d->ratingPainter.halfStepsEnabled();
}

void
RatingWidget::setRating( int rating )
{
    d->rating = rating;
    d->hoverRating = rating;
    update();
}

void
RatingWidget::setMaxRating( int max )
{
    d->ratingPainter.setMaxRating( max );
    update();
}


void
RatingWidget::setHalfStepsEnabled( bool enabled )
{
    d->ratingPainter.setHalfStepsEnabled( enabled );
    update();
}

void
RatingWidget::mousePressEvent( QGraphicsSceneMouseEvent* e )
{
    if ( e->button() == Qt::LeftButton )
    {
        QRect rect( contentsRect().topLeft().x(), contentsRect().topLeft().y(),
                    contentsRect().width(), contentsRect().height() );
        int ratingFromPos = d->ratingPainter.ratingFromPosition( rect, QPoint( e->pos().x(), e->pos().y() ) );
        if ( ratingFromPos >= 0 )
        {
            d->hoverRating = d->rating = ratingFromPos;
            update();
            emit ratingChanged( d->rating );
        }
    }
}


void
RatingWidget::hoverMoveEvent( QGraphicsSceneHoverEvent* e )
{
    QRect rect( contentsRect().topLeft().x(), contentsRect().topLeft().y(),
                    contentsRect().width(), contentsRect().height() );
    d->hoverRating = d->ratingPainter.ratingFromPosition( rect, QPoint( e->pos().x(), e->pos().y() ) );

    update();
}


void
RatingWidget::hoverEnterEvent( QGraphicsSceneHoverEvent* e )
{
    QRect rect( contentsRect().topLeft().x(), contentsRect().topLeft().y(),
                    contentsRect().width(), contentsRect().height() );
    d->hoverRating = d->ratingPainter.ratingFromPosition( rect, QPoint( e->pos().x(), e->pos().y() ) );

    update();
}

void
RatingWidget::hoverLeaveEvent( QGraphicsSceneHoverEvent* )
{
    d->hoverRating = -1;
    update();
}


void
RatingWidget::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED( option )
    Q_UNUSED( widget )
    if( d->showing )
    {
        d->ratingPainter.setEnabled( isEnabled() );
        QRect rect( contentsRect().topLeft().x(), contentsRect().topLeft().y(),
                    contentsRect().width(), contentsRect().height() );
        d->ratingPainter.paint( painter, rect, d->rating, d->hoverRating );
    }
}

QSizeF
RatingWidget::sizeHint( Qt::SizeHint hint, const QSizeF& size ) const
{
    Q_UNUSED( hint )
    Q_UNUSED( size )
    int numPix = d->ratingPainter.maxRating();
    if( d->ratingPainter.halfStepsEnabled() )
        numPix /= 2;

    QSizeF pixSize( d->pixSize, d->pixSize );
    if ( !d->ratingPainter.customPixmap().isNull() ) {
        pixSize = d->ratingPainter.customPixmap().size();
    }

    return QSizeF( pixSize.width()*numPix + spacing()*(numPix-1) + contentsRect().width(),
                  pixSize.height() + contentsRect().width() );
}


// void
// RatingWidget::resizeEvent( QGraphicsSceneResizeEvent* e )
// {
//     DEBUG_BLOCK
// 
// //     QFrame::resizeEvent( e );
// 
//     // FIXME: Disabled because this causes infinite recursion
//     //updateGeometry();
// }

#include "RatingWidget.moc"

