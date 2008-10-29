/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "TrackWidget.h"

#include "Amarok.h"
#include "Debug.h"
#include "meta/MetaUtility.h"
#include "playlist/PlaylistController.h"

#include <QFont>
#include <QFontMetricsF>


TrackWidget::TrackWidget( QGraphicsItem *parent )
    : ToolBoxIcon( parent )
    , m_track( 0 )
    , m_rating( new RatingWidget( this ) )
{
    setDrawBackground( true );
    m_rating->setSpacing( 2 );

    connect( m_rating, SIGNAL( ratingChanged( int ) ), SLOT( changeTrackRating( int ) ) );
}

TrackWidget::~TrackWidget()
{}

void
TrackWidget::changeTrackRating( int rating )
{
    if( m_track )
        m_track->setRating( rating );
}

void
TrackWidget::hide()
{
    //HACK: since the text is not being hidden until the applet is resized we set the text to an empty string
    setText( QString() );
    ToolBoxIcon::hide();
}

void
TrackWidget::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
    The::playlistController()->insertOptioned( m_track, Playlist::Append );
}

void
TrackWidget::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    m_rating->setMinimumSize( contentsRect().width() / 5, contentsRect().height() - PADDING );
    m_rating->setMaximumSize( contentsRect().width() / 5, contentsRect().height() - PADDING );

    m_rating->setPos( contentsRect().width() - PADDING - m_rating->size().width(),
                      contentsRect().height() / 2 - m_rating->size().height() / 2 - 2 );

    ToolBoxIcon::paint( painter, option, widget );
}

void
TrackWidget::setTrack( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    m_track = track;
    m_rating->setRating( track->rating() );
}

void
TrackWidget::show()
{
    // As a consequence of the hide() HACK now we have to re-set the text to display.
    if( m_track )
    {
        const QString playedLast = Amarok::verboseTimeSince( m_track->lastPlayed() );
        const QString fullText( i18n( "%1 - %2 ( %3 )", m_track->artist()->prettyName(), m_track->prettyName(), playedLast ) );
        const QFontMetricsF fm( font() );

        setText( fm.elidedText( fullText, Qt::ElideRight, contentsRect().width() - m_rating->size().width() - PADDING ) );
    }

    ToolBoxIcon::show();
}

Meta::TrackPtr
TrackWidget::track() const
{
    return m_track;
}

#include "TrackWidget.moc"

