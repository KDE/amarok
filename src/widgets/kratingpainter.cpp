/****************************************************************************************
 * Copyright (c) 2007-2008 Sebastian Trueg <trueg@kde.org>                              *
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

#include "kratingpainter.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QRect>
#include <QtCore/QPoint>

#include <kicon.h>
#include <kiconeffect.h>
#include <kdebug.h>


class KRatingPainter::Private
{
public:
    Private()
        : maxRating(10),
          isEnabled( true ),
          bHalfSteps(true),
          alignment(Qt::AlignCenter),
          direction(Qt::LeftToRight),
          spacing(0) {
    }

    QPixmap getPixmap( int size );

    int maxRating;
    QIcon icon;
    bool isEnabled;
    bool bHalfSteps;
    Qt::Alignment alignment;
    Qt::LayoutDirection direction;
    QPixmap customPixmap;
    int spacing;
};


QPixmap KRatingPainter::Private::getPixmap( int size )
{
    if ( !customPixmap.isNull() ) {
        return customPixmap.scaled( QSize( size, size ) );
    }
    else {
        QIcon _icon( icon );
        if ( _icon.isNull() ) {
            _icon = KIcon( "rating" );
        }
        return _icon.pixmap( size );
    }
}


KRatingPainter::KRatingPainter()
    : d(new Private())
{
}


KRatingPainter::~KRatingPainter()
{
    delete d;
}


int KRatingPainter::maxRating() const
{
    return d->maxRating;
}


bool KRatingPainter::halfStepsEnabled() const
{
    return d->bHalfSteps;
}


Qt::Alignment KRatingPainter::alignment() const
{
    return d->alignment;
}


Qt::LayoutDirection KRatingPainter::layoutDirection() const
{
    return d->direction;
}


QIcon KRatingPainter::icon() const
{
    return d->icon;
}


bool KRatingPainter::isEnabled() const
{
    return d->isEnabled;
}


QPixmap KRatingPainter::customPixmap() const
{
    return d->customPixmap;
}


int KRatingPainter::spacing() const
{
    return d->spacing;
}


void KRatingPainter::setMaxRating( int max )
{
    d->maxRating = max;
}


void KRatingPainter::setHalfStepsEnabled( bool enabled )
{
    d->bHalfSteps = enabled;
}


void KRatingPainter::setAlignment( Qt::Alignment align )
{
    d->alignment = align;
}


void KRatingPainter::setLayoutDirection( Qt::LayoutDirection direction )
{
    d->direction = direction;
}


void KRatingPainter::setIcon( const QIcon& icon )
{
    d->icon = icon;
}


void KRatingPainter::setEnabled( bool enabled )
{
    d->isEnabled = enabled;
}


void KRatingPainter::setCustomPixmap( const QPixmap& pixmap )
{
    d->customPixmap = pixmap;
}


void KRatingPainter::setSpacing( int s )
{
    d->spacing = qMax( 0, s );
}


void KRatingPainter::paint( QPainter* painter, const QRect& rect, int rating, int hoverRating ) const
{
    rating = qMin( rating, d->maxRating );
    hoverRating = qMin( hoverRating, d->maxRating );

    int numUsedStars = d->bHalfSteps ? d->maxRating/2 : d->maxRating;

    if ( hoverRating > 0 && hoverRating < rating ) {
        int tmp = hoverRating;
        hoverRating = rating;
        rating = tmp;
    }

    int usedSpacing = d->spacing;

    // get the rating pixmaps
    int maxHSizeOnePix = ( rect.width() - (numUsedStars-1)*usedSpacing ) / numUsedStars;
    QPixmap ratingPix = d->getPixmap( qMin( rect.height(), maxHSizeOnePix ) );

    QPixmap disabledRatingPix = KIconEffect().apply( ratingPix, KIconEffect::ToGray, 1.0, QColor(), false );
    QPixmap hoverPix;

    // if we are disabled we become gray and more transparent
    if ( !d->isEnabled ) {
        ratingPix = disabledRatingPix;
        KIconEffect::semiTransparent( disabledRatingPix );
    }

    bool half = d->bHalfSteps && rating%2;
    int numRatingStars = d->bHalfSteps ? rating/2 : rating;

    int numHoverStars = 0;
    bool halfHover = false;
    if ( hoverRating > 0 && rating != hoverRating && d->isEnabled ) {
        numHoverStars = d->bHalfSteps ? hoverRating/2 : hoverRating;
        halfHover = d->bHalfSteps && hoverRating%2;
        hoverPix = KIconEffect().apply( ratingPix, KIconEffect::ToGray, 0.5, QColor(), false );
    }

    if ( d->alignment & Qt::AlignJustify ) {
        int w = rect.width();
        w -= numUsedStars * ratingPix.width();
        usedSpacing = w / ( numUsedStars-1 );
    }

    int ratingAreaWidth = ratingPix.width()*numUsedStars + usedSpacing*(numUsedStars-1);

    int i = 0;
    int x = rect.x();
    if ( d->alignment & Qt::AlignRight ) {
        x += ( rect.width() - ratingAreaWidth );
    }
    else if ( d->alignment & Qt::AlignHCenter ) {
        x += ( rect.width() - ratingAreaWidth )/2;
    }

    int xInc = ratingPix.width() + usedSpacing;
    if ( d->direction == Qt::RightToLeft ) {
        x = rect.width() - ratingPix.width() - x;
        xInc = -xInc;
    }

    int y = rect.y();
    if( d->alignment & Qt::AlignVCenter ) {
        y += ( rect.height() / 2 - ratingPix.height() / 2 );
    }
    else if ( d->alignment & Qt::AlignBottom ) {
        y += ( rect.height() - ratingPix.height() );
    }
    for(; i < numRatingStars; ++i ) {
        painter->drawPixmap( x, y, ratingPix );
        x += xInc;
    }
    if( half ) {
        painter->drawPixmap( x, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? ratingPix : ( numHoverStars > 0 ? hoverPix : disabledRatingPix ),
                             0, 0, ratingPix.width()/2, ratingPix.height() );
        painter->drawPixmap( x + ratingPix.width()/2, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? ( numHoverStars > 0 ? hoverPix : disabledRatingPix ) : ratingPix,
                             ratingPix.width()/2, 0, ratingPix.width()/2, ratingPix.height() );
        x += xInc;
        ++i;
    }
    for(; i < numHoverStars; ++i ) {
        painter->drawPixmap( x, y, hoverPix );
        x += xInc;
    }
    if( halfHover ) {
        painter->drawPixmap( x, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? hoverPix : disabledRatingPix,
                             0, 0, ratingPix.width()/2, ratingPix.height() );
        painter->drawPixmap( x + ratingPix.width()/2, y, ratingPix.width()/2, ratingPix.height(),
                             d->direction == Qt::LeftToRight ? disabledRatingPix : hoverPix,
                             ratingPix.width()/2, 0, ratingPix.width()/2, ratingPix.height() );
        x += xInc;
        ++i;
    }
    for(; i < numUsedStars; ++i ) {
        painter->drawPixmap( x, y, disabledRatingPix );
        x += xInc;
    }
}


int KRatingPainter::ratingFromPosition( const QRect& rect, const QPoint& pos ) const
{
    int usedSpacing = d->spacing;
    int numUsedStars = d->bHalfSteps ? d->maxRating/2 : d->maxRating;
    int maxHSizeOnePix = ( rect.width() - (numUsedStars-1)*usedSpacing ) / numUsedStars;
    QPixmap ratingPix = d->getPixmap( qMin( rect.height(), maxHSizeOnePix ) );

    int ratingAreaWidth = ratingPix.width()*numUsedStars + usedSpacing*(numUsedStars-1);

    QRect usedRect( rect );
    if ( d->alignment & Qt::AlignRight ) {
        usedRect.setLeft( rect.right() - ratingAreaWidth );
    }
    else if ( d->alignment & Qt::AlignHCenter ) {
        int x = ( rect.width() - ratingAreaWidth )/2;
        usedRect.setLeft( rect.left() + x );
        usedRect.setRight( rect.right() - x );
    }
    else { // d->alignment & Qt::AlignLeft
        usedRect.setRight( rect.left() + ratingAreaWidth - 1 );
    }

    if ( d->alignment & Qt::AlignBottom ) {
        usedRect.setTop( rect.bottom() - ratingPix.height() + 1 );
    }
    else if ( d->alignment & Qt::AlignVCenter ) {
        int x = ( rect.height() - ratingPix.height() )/2;
        usedRect.setTop( rect.top() + x );
        usedRect.setBottom( rect.bottom() - x );
    }
    else { // d->alignment & Qt::AlignTop
        usedRect.setBottom( rect.top() + ratingPix.height() - 1 );
    }

    if ( usedRect.contains( pos ) ) {
        int x = 0;
        if ( d->direction == Qt::RightToLeft ) {
            x = usedRect.right() - pos.x();
        }
        else {
            x = pos.x() - usedRect.left();
        }

        double one = ( double )usedRect.width() / ( double )d->maxRating;

//        kDebug() << "rating:" << ( int )( ( double )x/one + 0.5 );

        return ( int )( ( double )x/one + 0.5 );
    }
    else {
        return -1;
    }
}


void KRatingPainter::paintRating( QPainter* painter, const QRect& rect, Qt::Alignment align, int rating, int hoverRating )
{
    KRatingPainter rp;
    rp.setAlignment( align );
    rp.setLayoutDirection( painter->layoutDirection() );
    rp.paint( painter, rect, rating, hoverRating );
}


int KRatingPainter::getRatingFromPosition( const QRect& rect, Qt::Alignment align, Qt::LayoutDirection direction, const QPoint& pos )
{
    KRatingPainter rp;
    rp.setAlignment( align );
    rp.setLayoutDirection( direction );
    return rp.ratingFromPosition( rect, pos );
}
