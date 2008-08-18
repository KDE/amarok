/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "lastfm"

#include "ScrobblerAdapter.h"
#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "MetaConstants.h"
#include "meta/LastFmMeta.h"
#include "libUnicorn/WebService/Request.h"


ScrobblerAdapter::ScrobblerAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent ),
      EngineObserver( The::engineController() ),
      m_manager( new ScrobblerManager( this ) ),
      m_username( username )
{
    resetVariables();

    connect( m_manager, SIGNAL( status( int, QVariant ) ), this, SLOT( statusChanged( int, QVariant ) ) );
    connect( The::mainWindow(), SIGNAL(loveTrack(Meta::TrackPtr)), SLOT(slotTrackLoved(Meta::TrackPtr)));

    Scrobbler::Init init;
    init.username = username;
    init.password = password;
    init.client_version = APP_VERSION;
    m_manager->handshake( init );
}


ScrobblerAdapter::~ScrobblerAdapter()
{}


void
ScrobblerAdapter::engineNewTrackPlaying()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        const bool isRadio = ( track->type() == "stream/lastfm" );

        checkScrobble();

        m_current.timeStampMe();

        m_current.setTrack( track->name() );
        m_current.setDuration( track->length() );
        if( track->artist() )
            m_current.setArtist( track->artist()->name() );
        if( track->album() )
            m_current.setAlbum( track->album()->name() );

        // TODO: need to get music brainz id from somewhere
        // m_current.setMbId( );

        m_current.setSource( isRadio ? TrackInfo::Radio : TrackInfo::Player );
        m_current.setUsername( m_username );

        if( !m_current.isEmpty() )
        {
            debug() << "nowPlaying: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.track();
            m_manager->nowPlaying( m_current );

            // When playing Last.fm Radio, we need to submit twice, once in Radio mode and once in Player mode
            if( isRadio ) {
                m_current.setSource( TrackInfo::Player );
                m_manager->nowPlaying( m_current );
            }
        }
    }
}

void
ScrobblerAdapter::engineTrackEnded( int finalPosition, int /*trackLength*/, const QString &/*reason*/ )
{
    engineTrackPositionChanged( finalPosition, false );
    checkScrobble();
    resetVariables();
}


void
ScrobblerAdapter::engineTrackPositionChanged( long position, bool userSeek )
{
    // note: in the 1.2 protocol, it's OK to submit if the user seeks
    // so long as they meet the half file played requirement.
    if( !userSeek && position > m_lastPosition )
        m_totalPlayed += position - m_lastPosition;
    m_lastPosition = position;
}


void
ScrobblerAdapter::skip()
{
    DEBUG_BLOCK

    m_current.setRatingFlag( TrackInfo::Skipped );
}


void
ScrobblerAdapter::love()
{
    DEBUG_BLOCK

    m_current.setRatingFlag( TrackInfo::Loved );

    LoveRequest* request = new LoveRequest( m_current );
    request->start();
}

void
ScrobblerAdapter::slotTrackLoved( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    TrackInfo trackInfo;
    trackInfo.setTrack( track->name() );
    if( track->artist() )
        trackInfo.setArtist( track->artist()->name() );
    if( track->album() )
        trackInfo.setAlbum( track->album()->name() );

    LoveRequest* request = new LoveRequest( trackInfo );
    request->start();
}


void
ScrobblerAdapter::ban()
{
    DEBUG_BLOCK

    m_current.setRatingFlag( TrackInfo::Banned );

    BanRequest* request = new BanRequest( m_current );
    request->start();
}


void
ScrobblerAdapter::statusChanged( int statusCode, QVariant /*data*/ )
{
    debug() << "statusChanged: statusCode=" << statusCode;
}


void
ScrobblerAdapter::resetVariables()
{
    m_current = TrackInfo();
    m_totalPlayed = m_lastPosition = 0;
}


void
ScrobblerAdapter::checkScrobble()
{
    // note: in the 1.2 protocol submits are always done at end of file
    if( ( m_current.isSkippedLovedOrBanned() || m_totalPlayed >= m_current.duration() * 1000 / 2 ) && !m_current.isEmpty() && AmarokConfig::submitPlayedSongs() )
    {
        debug() << "scrobble: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.track();
        m_manager->scrobble( m_current );
    }
    resetVariables();
}
