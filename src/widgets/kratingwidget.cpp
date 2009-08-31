/****************************************************************************************
 * Copyright (c) 2006-2007 Sebastian Trueg <trueg@kde.org>                              *
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

#include "kratingwidget.h"
#include "kratingpainter.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QKeyEvent>
#include <QtGui/QImage>
#include <QtGui/QIcon>

#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>

class KRatingWidget::Private
{
public:
    Private()
        : rating(0),
          hoverRating(-1),
          pixSize( 16 ) {
    }

    int rating;
    int hoverRating;
    int pixSize;

    KRatingPainter ratingPainter;
};



KRatingWidget::KRatingWidget( QWidget* parent )
    : QFrame( parent ),
      d( new Private() )
{
    setMouseTracking( true );
}


KRatingWidget::~KRatingWidget()
{
    delete d;
}


void KRatingWidget::setPixmap( const QPixmap& pix )
{
    setCustomPixmap( pix );
}


void KRatingWidget::setCustomPixmap( const QPixmap& pix )
{
    d->ratingPainter.setCustomPixmap( pix );
    update();
}


void KRatingWidget::setIcon( const QIcon& icon )
{
    d->ratingPainter.setIcon( icon );
    update();
}


void KRatingWidget::setPixmapSize( int size )
{
    d->pixSize = size;
    updateGeometry();
}


int KRatingWidget::spacing() const
{
    return d->ratingPainter.spacing();
}


QIcon KRatingWidget::icon() const
{
    return d->ratingPainter.icon();
}


void KRatingWidget::setSpacing( int s )
{
    d->ratingPainter.setSpacing( s );
    update();
}


Qt::Alignment KRatingWidget::alignment() const
{
    return d->ratingPainter.alignment();
}


void KRatingWidget::setAlignment( Qt::Alignment align )
{
    d->ratingPainter.setAlignment( align );
    update();
}


Qt::LayoutDirection KRatingWidget::layoutDirection() const
{
    return d->ratingPainter.layoutDirection();
}


void KRatingWidget::setLayoutDirection( Qt::LayoutDirection direction )
{
    d->ratingPainter.setLayoutDirection( direction );
    update();
}


unsigned int KRatingWidget::rating() const
{
    return d->rating;
}


int KRatingWidget::maxRating() const
{
    return d->ratingPainter.maxRating();
}


bool KRatingWidget::halfStepsEnabled() const
{
    return d->ratingPainter.halfStepsEnabled();
}


void KRatingWidget::setRating( unsigned int rating )
{
    setRating( (int)rating );
}


void KRatingWidget::setRating( int rating )
{
    d->rating = rating;
    d->hoverRating = rating;
    update();
}


void KRatingWidget::setMaxRating( unsigned int max )
{
    setMaxRating( (int)max );
}


void KRatingWidget::setMaxRating( int max )
{
    d->ratingPainter.setMaxRating( max );
    update();
}


void KRatingWidget::setHalfStepsEnabled( bool enabled )
{
    d->ratingPainter.setHalfStepsEnabled( enabled );
    update();
}


void KRatingWidget::setOnlyPaintFullSteps( bool fs )
{
    setHalfStepsEnabled( !fs );
}


void KRatingWidget::mousePressEvent( QMouseEvent* e )
{
    if ( e->button() == Qt::LeftButton ) {
        d->hoverRating = d->rating = d->ratingPainter.ratingFromPosition( contentsRect(), e->pos() );
        update();
        emit ratingChanged( d->rating );
        emit ratingChanged( (unsigned int)d->rating );
    }
}


void KRatingWidget::mouseMoveEvent( QMouseEvent* e )
{
    // when moving the mouse we show the user what the result of clicking will be
    d->hoverRating = d->ratingPainter.ratingFromPosition( contentsRect(), e->pos() );
    if ( d->hoverRating >= 0 && e->buttons() & Qt::LeftButton ) {
        d->rating = d->hoverRating;
        emit ratingChanged( d->rating );
        emit ratingChanged( (unsigned int)d->rating );
    }
    update();
}


void KRatingWidget::leaveEvent( QEvent* )
{
    d->hoverRating = -1;
    update();
}


void KRatingWidget::paintEvent( QPaintEvent* e )
{
    QFrame::paintEvent( e );
    QPainter p( this );
    d->ratingPainter.setEnabled( isEnabled() );
    d->ratingPainter.paint( &p, contentsRect(), d->rating, d->hoverRating );
}


QSize KRatingWidget::sizeHint() const
{
    int numPix = d->ratingPainter.maxRating();
    if( d->ratingPainter.halfStepsEnabled() )
        numPix /= 2;

    QSize pixSize( d->pixSize, d->pixSize );
    if ( !d->ratingPainter.customPixmap().isNull() ) {
        pixSize = d->ratingPainter.customPixmap().size();
    }

    return QSize( pixSize.width()*numPix + spacing()*(numPix-1) + frameWidth()*2,
                  pixSize.height() + frameWidth()*2 );
}


void KRatingWidget::resizeEvent( QResizeEvent* e )
{
    QFrame::resizeEvent( e );
}

#include "kratingwidget.moc"
