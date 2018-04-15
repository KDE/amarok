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


RatingItem::RatingItem( QQuickItem* parent )
    : QQuickPaintedItem( parent )
    , m_ratingPainter( new KRatingPainter )
{
    setAcceptedMouseButtons( Qt::LeftButton );
    setAcceptHoverEvents( true );

    connect( qApp, &QGuiApplication::paletteChanged, this, &QQuickItem::update );
}


RatingItem::~RatingItem()
{
    delete m_ratingPainter;
}


void
RatingItem::setIcon( const QString& iconName )
{
    if( iconName == icon() )
        return;

    m_ratingPainter->setIcon( QIcon::fromTheme( iconName ) );
    emit iconChanged();

    update();
}


int
RatingItem::spacing() const
{
    return m_ratingPainter->spacing();
}


QString
RatingItem::icon() const
{
    return m_ratingPainter->icon().name();
}


void
RatingItem::setSpacing( int s )
{
    if( s == m_ratingPainter->spacing() )
        return;

    m_ratingPainter->setSpacing( s );
    emit spacingChanged();

    update();
}


Qt::Alignment
RatingItem::alignment() const
{
    return m_ratingPainter->alignment();
}


void
RatingItem::setAlignment( Qt::Alignment align )
{
    if( align == m_ratingPainter->alignment() )
        return;

    m_ratingPainter->setAlignment( align );
    emit alignmentChanged();

    update();
}


Qt::LayoutDirection
RatingItem::layoutDirection() const
{
    return m_ratingPainter->layoutDirection();
}


void
RatingItem::setLayoutDirection( Qt::LayoutDirection direction )
{
    if( direction == m_ratingPainter->layoutDirection() )
        return;

    m_ratingPainter->setLayoutDirection( direction );
    emit layoutDirectionChanged();

    update();
}


unsigned int
RatingItem::rating() const
{
    return m_rating;
}


int
RatingItem::maxRating() const
{
    return m_ratingPainter->maxRating();
}


int RatingItem::hoverRating() const
{
    return m_hoverRating;
}


bool
RatingItem::halfStepsEnabled() const
{
    return m_ratingPainter->halfStepsEnabled();
}

void
RatingItem::setRating( int rating )
{
    if( rating == m_rating )
        return;

    m_rating = rating;
    m_hoverRating = rating;
    emit ratingChanged();
    emit hoverRatingChanged();

    update();
}

void
RatingItem::setMaxRating( int max )
{
    if( max == m_ratingPainter->maxRating() )
        return;

    bool halfSteps = m_ratingPainter->halfStepsEnabled();

    m_ratingPainter->setMaxRating( max );
    emit maxRatingChanged();

    if( halfSteps != m_ratingPainter->halfStepsEnabled() )
        emit halfStepsEnabledChanged();

    update();
}


void
RatingItem::setHalfStepsEnabled( bool enabled )
{
    if( enabled == m_ratingPainter->halfStepsEnabled() )
        return;

    m_ratingPainter->setHalfStepsEnabled( enabled );
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
        int ratingFromPos = m_ratingPainter->ratingFromPosition( rect, e->pos() );
        debug() << "Rating item clicked. New rating:" << ratingFromPos;

        if ( ratingFromPos >= 0 )
            emit clicked( ratingFromPos );
    }
}

void
RatingItem::hoverMoveEvent( QHoverEvent* e )
{
    QRect rect( 0, 0, width(), height() );
    m_hoverRating = m_ratingPainter->ratingFromPosition( rect, e->pos() );

    update();
}

void
RatingItem::hoverEnterEvent( QHoverEvent* e )
{
    QRect rect( 0, 0, width(), height() );
    m_hoverRating = m_ratingPainter->ratingFromPosition( rect, e->pos() );

    update();
}

void
RatingItem::hoverLeaveEvent( QHoverEvent* )
{
    m_hoverRating = -1;

    update();
}


void
RatingItem::paint( QPainter* painter )
{

    m_ratingPainter->setEnabled( isEnabled() );
    QRect rect( 0, 0, width(), height() );
    m_ratingPainter->paint( painter, rect, m_rating, m_hoverRating );
}
