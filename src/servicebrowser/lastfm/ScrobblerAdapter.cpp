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
#include "enginecontroller.h"
#include "amarok.h"
#include "debug.h"
#include "MetaConstants.h"
#include "meta/LastFmMeta.h"


ScrobblerAdapter::ScrobblerAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent ),
      EngineObserver( EngineController::instance() ),
      m_manager( new ScrobblerManager( this ) ),
      m_username( username )
{
    resetVariables();

    connect( m_manager, SIGNAL( status( int, QVariant ) ), this, SLOT( statusChanged( int, QVariant ) ) );

    Scrobbler::Init init;
    init.username = username;
    init.password = password;
    init.client_version = APP_VERSION;
    m_manager->handshake( init );
}


ScrobblerAdapter::~ScrobblerAdapter()
{
}


void 
ScrobblerAdapter::engineNewMetaData( const QHash<qint64, QString> &/*newMetaData*/, bool trackChanged )
{
    debug() << "engineNewMetaData: trackChanged=" << trackChanged;

    // if trackChanged, it's a local file
    // also need to handle radio case
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    bool isRadio = track && KUrl( track->url() ).protocol() == "lastfm";
    if (track && trackChanged || isRadio)
    {
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
        }
    }
}


void 
ScrobblerAdapter::engineTrackEnded( int finalPosition, int trackLength, const QString &reason )
{
    Q_UNUSED( trackLength );

    debug() << "engineTrackEnded: reason=" << reason;
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
    m_current.setRatingFlag( TrackInfo::Skipped );
}


void
ScrobblerAdapter::love()
{
    m_current.setRatingFlag( TrackInfo::Loved );
}


void
ScrobblerAdapter::ban()
{
    m_current.setRatingFlag( TrackInfo::Banned );
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
    if( ( m_current.isSkippedLovedOrBanned() || m_totalPlayed >= m_current.duration() * 1000 / 2 ) && !m_current.isEmpty() )
    {
        debug() << "scrobble: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.track();
        m_manager->scrobble( m_current );
    }
    resetVariables();
}
