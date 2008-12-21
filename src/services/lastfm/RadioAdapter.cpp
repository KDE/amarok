/***************************************************************************
* copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "RadioAdapter.h"
#include "Amarok.h"
#include "LastFmService.h"
#include "LastFmSettings.h"

#include "statusbar/StatusBar.h"

RadioAdapter::RadioAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent ), m_radio( new Radio( this ) )
{
    The::webService()->setUsername( username );
    The::webService()->setPassword( password );
    m_radio->init( username, password, APP_VERSION );

    connect( m_radio, SIGNAL( error( RadioError, const QString& ) ), this, SLOT( error( RadioError, const QString& ) ) );
}


RadioAdapter::~RadioAdapter()
{
}

void
RadioAdapter::slotStationName( const QString& name )
{
    AMAROK_NOTIMPLEMENTED
}

void
RadioAdapter::slotNewTracks( const QList< Track >& tracks )
{
    foreach( const Track &track,  tracks )
        m_upcomingTracks.enqueue( track );
}

void
RadioAdapter::play( LastFm::Track *track )
{
    DEBUG_BLOCK
    if( track != m_currentTrack )
    {
        bool newStation = The::currentUser().resumeStation().url() != track->uidUrl();
        m_currentTrack = track;
        emit haveTrack( true );
        debug() << "ok, got a different track to play, newStation:" << newStation;
        if( newStation  )
        {
            debug() < "new tuner with uidUrl:" << track->uidUrl();
            m_tuner = new Tuner( RadioStation( track->uidUrl() ) );
            
            connect( m_tuner, SIGNAL( error( Ws::Error ) ), this, SLOT( error( Ws::Error ) ) );
            connect( m_tuner, SIGNAL( stationName( const QString& ) ), this, SLOT( slotStationName( const QString& ) ) );
            connect( m_tuner, SIGNAL( tracks( const QList< Track >& ) ), this, SLOT( slotNewTracks( const QList< Track >& ) ) );
            
        } //else
            // TODO doesn't seem supported w/ new API.
            //The::radio().resumeStation();
    }
}


void
RadioAdapter::next()
{
    if( m_currentTrack )
    {
        if( m_upcomingTracks.size() == 0 ) // we fetched more and failed, so just stop
            stop();
        else if( m_upcomingTracks.size() == 1 ) // gotta fetch more
        {
            play( new LastFm::Track( m_upcomingTracks.dequeue() ) );
            m_tuner->fetchFiveMoreTracks();
        } else
        {
            play( new LastFm::Track( m_upcomingTracks.dequeue() ) );
        }
    }
}


void
RadioAdapter::stop()
{
    if( m_currentTrack )
    {
        m_currentTrack->setTrackInfo( Track() ); // will emit an empty playable url, thus moving to next track in playlist
        m_currentTrack = 0;
        emit haveTrack( false );
    }
}


void
RadioAdapter::error( RadioError errorCode, const QString& message )
{
    The::statusBar()->longMessage( i18nc("Last.fm: errorMessage", "%1: %2 Error code: %3", "Last.fm", message, errorCode), StatusBar::Error );
    stop();
}


namespace The
{
    Radio &radio()
    {
        return *lastFmService()->radio()->m_radio;
    }
}
