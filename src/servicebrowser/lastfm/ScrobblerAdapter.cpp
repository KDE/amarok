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


ScrobblerAdapter::ScrobblerAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent ),
      EngineObserver( EngineController::instance() ),
      m_manager( new ScrobblerManager( this ) ),
      m_username( username )
{
    connect( m_manager, SIGNAL( status( int, QVariant ) ), this, SLOT( statusChanged( int, QVariant ) ) );

    Scrobbler::Init init;
    init.username = username;
    init.password = UnicornUtils::md5Digest( password.toUtf8() );
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

    if (trackChanged)
    {
        m_current  = TrackInfo();
        m_current.timeStampMe();

        // what we get from phonon (at least for ds9) is a pile of doggy doo
        //m_current.setArtist( newMetaData.value( Meta::valArtist ) );
        //m_current.setAlbum( newMetaData.value( Meta::valAlbum ) );
        //m_current.setTrack( newMetaData.value( Meta::valTitle ) );
        //m_current.setDuration( newMetaData.value( Meta::valLength ) );

        Meta::TrackPtr track = EngineController::instance()->currentTrack();
        if( track )
        {
            m_current.setTrack( track->name() );
            m_current.setDuration( track->length() );
            if( track->artist() )
                m_current.setArtist( track->artist()->name() );
            if( track->album() )
                m_current.setAlbum( track->album()->name() );
        }

        m_current.setUsername( m_username );

        if( !m_current.isEmpty() )
        {
            debug() << "nowPlaying: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.track();
            m_manager->nowPlaying( m_current );
        }

        m_lastPosition = m_totalPlayed = 0;
    }
}


void 
ScrobblerAdapter::engineTrackEnded( int finalPosition, int trackLength, const QString &reason )
{
    // note: in the 1.2 protocol submits are always done at end of file
    debug() << "engineTrackEnded: reason=" << reason;
    m_totalPlayed += finalPosition - m_lastPosition;
    if( m_totalPlayed >= trackLength / 2 && !m_current.isEmpty() )
    {
        debug() << "scrobble: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.track();
        m_manager->scrobble( m_current );
        m_current = TrackInfo();
    }
}


void 
ScrobblerAdapter::engineTrackPositionChanged( long position, bool userSeek )
{
    // note: in the 1.2 protocol, it's OK to submit if the user seeks
    // so long as they meet the half file played requirement.
    if( !userSeek )
        m_totalPlayed += position - m_lastPosition;
    m_lastPosition = position;
}


void
ScrobblerAdapter::statusChanged( int statusCode, QVariant /*data*/ )
{
    debug() << "statusChanged: statusCode=" << statusCode;
}
