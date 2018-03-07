/****************************************************************************************
 * Copyright (c) 2008 William Viana Soarjs <vianasw@gmail.com>                          *
 * Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "RatingItem.h"

#include "core/support/Debug.h"

#include <QGuiApplication>
#include <QHoverEvent>
#include <QIcon>
#include <QMouseEvent>

#include <KRatingPainter>

class RatingItem::Private
{
public:
    Private()
        : rating(0)
        , hoverRating(-1)
        , pixSize( 16 )
    {
    }

    int rating;
    int hoverRating;
    int pixSize;

    KRatingPainter ratingPainter;
};


RatingItem::RatingItem( QQuickItem* parent )
    : QQuickPaintedItem( parent )
    , d( new Private() )
{
    setAcceptedMouseButtons( Qt::LeftButton );
    setAcceptHoverEvents( true );

    connect( qApp, &QGuiApplication::paletteChanged, this, &QQuickItem::update );
}


RatingItem::~RatingItem()
{
    delete d;
}


void
RatingItem::setIcon( const QString& iconName )
{
    if( iconName == icon() )
        return;

    d->ratingPainter.setIcon( QIcon::fromTheme( iconName ) );
    emit iconChanged();

    update();
}


int
RatingItem::spacing() const
{
    return d->ratingPainter.spacing();
}


QString
RatingItem::icon() const
{
    return d->ratingPainter.icon().name();
}


void
RatingItem::setSpacing( int s )
{
    if( s == d->ratingPainter.spacing() )
        return;

    d->ratingPainter.setSpacing( s );
    emit spacingChanged();

    update();
}


Qt::Alignment
RatingItem::alignment() const
{
    return d->ratingPainter.alignment();
}


void
RatingItem::setAlignment( Qt::Alignment align )
{
    if( align == d->ratingPainter.alignment() )
        return;

    d->ratingPainter.setAlignment( align );
    emit alignmentChanged();

    update();
}


Qt::LayoutDirection
RatingItem::layoutDirection() const
{
    return d->ratingPainter.layoutDirection();
}


void
RatingItem::setLayoutDirection( Qt::LayoutDirection direction )
{
    if( direction == d->ratingPainter.layoutDirection() )
        return;

    d->ratingPainter.setLayoutDirection( direction );
    emit layoutDirectionChanged();

    update();
}


unsigned int
RatingItem::rating() const
{
    return d->rating;
}


int
RatingItem::maxRating() const
{
    return d->ratingPainter.maxRating();
}


int RatingItem::hoverRating() const
{
    return d->hoverRating;
}


bool
RatingItem::halfStepsEnabled() const
{
    return d->ratingPainter.halfStepsEnabled();
}

void
RatingItem::setRating( int rating )
{
    if( rating == d->rating )
        return;

    d->rating = rating;
    d->hoverRating = rating;
    emit ratingChanged();
    emit hoverRatingChanged();

    update();
}

void
RatingItem::setMaxRating( int max )
{
    if( max == d->ratingPainter.maxRating() )
        return;

    bool halfSteps = d->ratingPainter.halfStepsEnabled();

    d->ratingPainter.setMaxRating( max );
    emit maxRatingChanged();

    if( halfSteps != d->ratingPainter.halfStepsEnabled() )
        emit halfStepsEnabledChanged();

    update();
}


void
RatingItem::setHalfStepsEnabled( bool enabled )
{
    if( enabled == d->ratingPainter.halfStepsEnabled() )
        return;

    d->ratingPainter.setHalfStepsEnabled( enabled );
    emit halfStepsEnabledChanged();

    update();
}

void
RatingItem::mousePressEvent( QMouseEvent* e )
{
    DEBUG_BLOCK

    if ( e->button() == Qt::LeftButton )
    {
        QRect rect( 0, 0, width(), height() );
        int ratingFromPos = d->ratingPainter.ratingFromPosition( rect, e->pos() );
        debug() << "Rating item clicked. New rating:" << ratingFromPos;

        if ( ratingFromPos >= 0 )
        {
            //             setToolTip( i18n( "Track rating: %1", (float)d->rating / 2 ) );
            emit clicked( ratingFromPos );
        }
    }
}


void
RatingItem::hoverMoveEvent( QHoverEvent* e )
{
    QRect rect( 0, 0, width(), height() );
    d->hoverRating = d->ratingPainter.ratingFromPosition( rect, e->pos() );

    update();
}


void
RatingItem::hoverEnterEvent( QHoverEvent* e )
{
    QRect rect( 0, 0, width(), height() );
    d->hoverRating = d->ratingPainter.ratingFromPosition( rect, e->pos() );

    //     setToolTip( i18n( "Track rating: %1", (float)d->rating / 2 ) );

    update();
}

void
RatingItem::hoverLeaveEvent( QHoverEvent* )
{
    d->hoverRating = -1;
    update();
}


void
RatingItem::paint( QPainter* painter )
{

    d->ratingPainter.setEnabled( isEnabled() );
    QRect rect( 0, 0, width(), height() );
    d->ratingPainter.paint( painter, rect, d->rating, d->hoverRating );
}
