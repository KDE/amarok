/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "lastfm"

#include "ScrobblerAdapter.h"
#include "LastFmServiceConfig.h"
#include "core/support/Amarok.h"
#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "core/meta/support/MetaConstants.h"
#include "meta/LastFmMeta.h"

#include <KLocale>

ScrobblerAdapter::ScrobblerAdapter( QObject *parent, const QString &clientId )
    : QObject( parent ),
      m_scrobbler( new lastfm::Audioscrobbler( clientId ) ),
      m_clientId( clientId ),
      m_lastSaved( 0 )
{
    DEBUG_BLOCK

    resetVariables();

    //HACK work around a bug in liblastfm---it doesn't create its config dir, so when it
    // tries to write the track cache, it fails silently. until we have a fixed version, do this
    // path finding code taken from liblastfm/src/misc.cpp
    QString lpath = QDir::home().filePath( ".local/share/Last.fm" );
    QDir ldir = QDir( lpath );
    if( !ldir.exists() )
    {
        ldir.mkpath( lpath );
    }
    
    connect( The::mainWindow(), SIGNAL( loveTrack( Meta::TrackPtr) ), SLOT( loveTrack( Meta::TrackPtr ) ) );
    connect( The::mainWindow(), SIGNAL( banTrack() ), SLOT( banTrack() ) );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped( qint64, qint64 ) ) );
    connect( engine, SIGNAL( trackPositionChanged( qint64, bool ) ),
             this, SLOT( trackPositionChanged( qint64, bool ) ) );
    //Use trackChanged instead of trackPlaying to prevent reset of current track after Unpausing.
    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
             this, SLOT( trackPlaying( Meta::TrackPtr ) ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( trackMetadataChanged( Meta::TrackPtr ) ) );
}

ScrobblerAdapter::~ScrobblerAdapter()
{
    delete m_scrobbler;
}

void
ScrobblerAdapter::trackPlaying( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    if( track )
    {
        m_lastSaved = m_lastPosition; // HACK engineController is broken :(

        debug() << "track type:" << track->type();
        const bool isRadio = ( track->type() == "stream/lastfm" );
        
        checkScrobble();

        m_current.stamp();
        
        m_current.setDuration( track->length() / 1000 );
	copyTrackMetadata( m_current, track );

        QString uid = track->uidUrl();
        if( uid.startsWith( "amarok-sqltrackuid://mb-" ) )
        {
            uid.remove( "amarok-sqltrackuid://mb-" );
            m_current.setMbid( lastfm::Mbid( uid ) );
        }

        // TODO also set fingerprint... whatever that is :)
        // m_current.setFingerprintId( qstring );
        
        m_current.setSource( isRadio ? lastfm::Track::LastFmRadio : lastfm::Track::Player );
        

        if( !m_current.isNull() )
        {
            debug() << "nowPlaying: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.title();
            m_scrobbler->nowPlaying( m_current );

            // When playing Last.fm Radio, we need to submit twice, once in Radio mode and once in Player mode
            // TODO check with mxcl if this is still required
            if( isRadio ) {
                m_current.setSource( lastfm::Track::Player );
                m_scrobbler->nowPlaying( m_current );
            }
        }
    }
}


void
ScrobblerAdapter::trackMetadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    // if we are listening to a stream, take the new metadata as a "new track" and, if we have enough info, save it for scrobbling
    if( track &&
        ( track->type() == "stream" && ( !track->name().isEmpty() 
          && ( track->artist() || scrobbleComposer( track ) ) ) ) )
        // got a stream, and it has enough info to be a new track
    {
        // don't use checkScrobble as we don't need to check timestamps, it is a stream
        debug() << "scrobble: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.title();
        m_current.setDuration( QDateTime::currentDateTime().toTime_t() - m_current.timestamp().toTime_t() );
        m_scrobbler->cache( m_current );
        m_scrobbler->submit();
        resetVariables();

        // previous implementation didn't copy over album, I have no idea why so now we use generic method that does
	copyTrackMetadata( m_current, track );
        m_current.stamp();

        m_current.setSource( lastfm::Track::NonPersonalisedBroadcast );

        if( !m_current.isNull() )
        {
            debug() << "nowPlaying: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.title();
            m_scrobbler->nowPlaying( m_current );
        }
    }
}

void
ScrobblerAdapter::stopped( qint64 finalPosition, qint64 trackLength )
{
    DEBUG_BLOCK
    Q_UNUSED( trackLength );

    trackPositionChanged( finalPosition, false );
    checkScrobble();
}


void
ScrobblerAdapter::trackPositionChanged( qint64 position, bool userSeek )
{
    // HACK enginecontroller is fscked. it sends engineTrackPositionChanged messages
    // with info for the last track even after engineNewTrackPlaying. this means that
    // we think we've played the whole new track even though we really haven't. so, temporary
    // workaround for 2.1.0 until i can rewrite this class properly to not need to do it
    // this way.
    //debug() << "m_lastPosition:" << m_lastPosition << "position:" << position << "m_lastSaved:" << m_lastSaved;

    if( m_lastPosition == 0 && m_lastSaved != 0 && position > m_lastSaved ) // this is probably when the fucked up info came through, ignore
        return;
    m_lastSaved = 0;
    
    // note: in the 1.2 protocol, it's OK to submit if the user seeks
    // so long as they meet the half file played requirement.
    //debug() << "userSeek" << userSeek << "position:" << position << "m_lastPosition" << m_lastPosition << "m_totalPlayed" << m_totalPlayed;
    if( !userSeek && position > m_lastPosition )
        m_totalPlayed += position - m_lastPosition;
    m_lastPosition = position;
    //debug() << "userSeek" << userSeek << "position:" << position << "m_lastPosition" << m_lastPosition << "m_totalPlayed" << m_totalPlayed;
}


void
ScrobblerAdapter::skip()
{
    DEBUG_BLOCK

    // NOTE doesn't exist in 1.2.1 lib... find replacement
    //m_current.setRatingFlag( Track::Skipped );
}


void
ScrobblerAdapter::love()
{
    DEBUG_BLOCK

    m_current.love();
    
}

void
ScrobblerAdapter::loveTrack( Meta::TrackPtr track ) // slot
{
    DEBUG_BLOCK

    if( track )
    {
        lastfm::MutableTrack trackInfo;
	copyTrackMetadata( trackInfo, track );

        trackInfo.love();
        Amarok::Components::logger()->shortMessage( i18nc( "As in, lastfm", "Loved Track: %1", track->prettyName() ) );
    }
}

void
ScrobblerAdapter::banTrack() // slot
{
    DEBUG_BLOCK

    m_current.ban();
}

void
ScrobblerAdapter::ban()
{
    DEBUG_BLOCK

    m_current.ban();
}

void
ScrobblerAdapter::resetVariables()
{
    m_current = lastfm::MutableTrack();
    m_totalPlayed = m_lastPosition = m_lastSaved = 0;
}


void
ScrobblerAdapter::checkScrobble()
{
    DEBUG_BLOCK
    debug() << "total played" << m_totalPlayed << "duration" << m_current.duration() * 1000 / 2 << "isNull" << m_current.isNull() << "submit?" << AmarokConfig::submitPlayedSongs();
    if( ( m_totalPlayed > m_current.duration() * 1000 / 2 ) && !m_current.isNull() && AmarokConfig::submitPlayedSongs() )
    {
        debug() << "scrobble: " << m_current.artist() << " - " << m_current.album() << " - " << m_current.title();
        m_scrobbler->cache( m_current );
        m_scrobbler->submit();
    }
    resetVariables();
}

void
ScrobblerAdapter::copyTrackMetadata( lastfm::MutableTrack& mutableTrack, Meta::TrackPtr track )
{
    DEBUG_BLOCK

    mutableTrack.setTitle( track->name() );

    bool okScrobbleComposer = scrobbleComposer( track );
    debug() << "scrobbleComposer: " << okScrobbleComposer;
    if( okScrobbleComposer )
        mutableTrack.setArtist( track->composer()->name() );
    else if( track->artist() )
        mutableTrack.setArtist( track->artist()->name() );

    if( track->album() )
        mutableTrack.setAlbum( track->album()->name() );
}

bool
ScrobblerAdapter::scrobbleComposer( Meta::TrackPtr track )
{
    KConfigGroup config = KGlobal::config()->group( LastFmServiceConfig::configSectionName() );
    return config.readEntry( "scrobbleComposer", false ) &&
        track->composer() && !track->composer()->name().isEmpty();
}
