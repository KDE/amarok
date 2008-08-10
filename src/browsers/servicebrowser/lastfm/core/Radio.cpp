/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Radio.h"
#include "LastFmSettings.h"
//#include "lastfmapplication.h"
#include "WebService/Request.h"
//#include "playerlistener.h"

#include "Station.h"
#include "MooseCommon.h"
#include "logger.h"

#include <KLocale>

#define SKIP_LIMIT_ENABLEDx

Radio::Radio( QObject* parent ) :
    QObject( parent ),
    m_state( State_Uninitialised ),
    m_currentChangeRequest( 0 ),
    m_skipsLeft( -1 ),
    m_resumePossible( false ),
    m_handshakeAfterNext( false ),
    m_broken( false ),
    m_cachedPlaylistErrorCode( (RadioError)-1 )
{
    connect( &m_playlist, SIGNAL( playlistLoaded( QString, int ) ),
             this,        SLOT  ( onPlaylistLoaded( QString, int ) ) );

    connect( &m_playlist, SIGNAL( error( RadioError, const QString& ) ),
             this,        SLOT  ( onPlaylistError( RadioError, const QString& ) ) );

    // A state change in the audio controller is a state change in the radio
    connect( &m_audioController, SIGNAL( stateChanged( RadioState ) ),
             this,               SLOT  ( onAudioControllerStateChanged( RadioState ) ) );

    connect( &m_audioController,  SIGNAL( buffering( int, int ) ),
             this,                SIGNAL( buffering( int, int ) ) );

    connect( &m_audioController, SIGNAL( error( RadioError, const QString& ) ),
             this,               SLOT  ( onAudioControllerError( RadioError, const QString& ) ) );

    connect( &m_audioController, SIGNAL( trackChanged( TrackInfo&, const TrackInfo& ) ),
             this,               SLOT  ( onTrackChanged( TrackInfo&, const TrackInfo& ) ) );

    connect( &m_audioController, SIGNAL( trackStarted( const TrackInfo& ) ),
             this,               SLOT  ( onTrackStarted( const TrackInfo& ) ) );

    connect( &m_audioController, SIGNAL( trackEnded( const TrackInfo&, int ) ),
             this,               SLOT  ( onTrackEnded( const TrackInfo&, int ) ) );
}

void
Radio::init( QString username,
             QString password,
             QString version )
{
    // We have to let the radio handshake even if HIDE_RADIO is defined because
    // the handshake returns vital parameters for the app like the basehost etc.
    // Quite a broken design really.

    stop();

    // UGLY! If we reinitialise the radio while it's playing, it will emit State_Stopping
    // followed by State_Handshaking. This will lead to the GUI not clearing properly
    // as the Stopped is never received. So we force a Stopped here before we go to
    // Handshaking. This should be safe as the handler for the Handshaking state should
    // disable all radio controls.
    setState( State_Stopped );

    m_username = username;
    m_password = password;
    m_version = version;

    handshake();
}

void
Radio::handshake()
{
    // TODO: rename to RadioHandshake
    Handshake* handshake = new Handshake;
    handshake->setUsername( m_username );
    handshake->setPassword( m_password );
    handshake->setVersion( m_version );
    handshake->setLanguage( The::settings().appLanguage() );
    handshake->start();

    connect( handshake, SIGNAL( result( Request* ) ),
             this,      SLOT  ( handshakeReturn( Request* ) ),
             Qt::QueuedConnection );
             
    m_resumePossible = false;
    setState( State_Handshaking );
}             

void
Radio::handshakeReturn( Request* req )
{
    Handshake* handshake = static_cast<Handshake*>( req );

    if ( handshake->failed() )
    {
        // Let Container deal with all the error conditions and display appropriate messages
        emit error( static_cast<RadioError>( handshake->resultCode() ),
                    handshake->errorMessage() );
        setState( State_Uninitialised );
        return;
    }

    if ( handshake->resultCode() == Request_Success )
    {
        m_session = handshake->session();
        m_basePath = handshake->basePath();
        
        The::settings().setFingerprintUploadUrl( handshake->fingerprintUploadUrl() );
        
        setState( State_Handshaken );

        if ( !m_pendingStation.isEmpty() )
        {
            playStation( m_pendingStation );
            m_pendingStation.clear();
        }
    }
    else
    {
        // Should only be able to be aborted here. Do nothing.
        Q_ASSERT( handshake->resultCode() == Request_Aborted );
    }

}


void
Radio::setDiscoveryMode( bool enabled )
{
    The::settings().currentUser().setDiscovery( enabled );
    
    // Requery for new XSPF    
    if ( m_playlist.type() == RadioPlaylist::Type_Station )
    {
        m_playlist.discardRemaining();
    }
}


void
Radio::play( Track track )
{
#ifndef HIDE_RADIO

    Q_DEBUG_BLOCK;

    Request *r = new TrackToIdRequest( track );
    connect( r, SIGNAL(result( Request* )), SLOT(trackToIdReturn( Request* )) );
    r->start();

#endif
}

void
Radio::trackToIdReturn( Request* _r )
{
#ifndef HIDE_RADIO

    Q_DEBUG_BLOCK;

    TrackToIdRequest *r = (TrackToIdRequest*)_r;

    if (!r->failed() && r->isStreamable())
        playStation( StationUrl( "lastfm://play/tracks/" + QString::number( r->id() ) ) );
    else
        emit error( Radio_InvalidUrl, 
                    i18n( "Sorry, this track isn't in the Last.fm catalog, and thus cannot be streamed." ) 
                    + "<p>" + r->track().toString());
#endif
}


void
Radio::playStation( StationUrl lastfmUrl )
{
#ifndef HIDE_RADIO
    
    if (lastfmUrl.isEmpty()) {
        Q_DEBUG_BLOCK << "Empty url passed";
        return;
    }
    
    LOGL( 3, "Starting station: " << lastfmUrl );

    if ( m_broken )
    {
        LOGL( 2, "Radio broken, early out" );
        return;
    }

    m_stationUrl = lastfmUrl;
    
    switch ( m_state )
    {
        case State_Uninitialised:
        {
            LOGL( 3, "Radio was uninitialised, trying to handshake..." );

            m_pendingStation = lastfmUrl;
            handshake();
        }
        break;
        
        case State_Handshaking:
        {
            LOGL( 3, "Radio is handshaking, setting pending station..." );

            m_pendingStation = lastfmUrl;
        }
        break;
        
        case State_Handshaken:
        case State_Stopped:
        case State_ChangingStation:
        case State_FetchingPlaylist:
        case State_FetchingStream:
        case State_StreamFetched:
        case State_Buffering:
        case State_Streaming:
        case State_Skipping:
        {
            stop();
            
            m_playlist.clear();
            m_stationWatch.stop();
            m_stationWatch.reset();

            LOGL( 3, "Calling adjust for: " << lastfmUrl );

            m_currentChangeRequest = new ChangeStationRequest();
            m_currentChangeRequest->setStationUrl( lastfmUrl );
            m_currentChangeRequest->setBasePath( m_basePath );
            m_currentChangeRequest->setSession( m_session );
            m_currentChangeRequest->setLanguage( The::settings().appLanguage() );
            m_currentChangeRequest->start();

            connect( m_currentChangeRequest, SIGNAL( result( Request* ) ),
                     this,                   SLOT( changeStationRequestReturn( Request* ) ) );

            m_skipsLeft = -1;
            
            setState( State_ChangingStation );
        }
        break;

        case State_Stopping:
        default:
            Q_ASSERT( !"Should not be possible" );
    }

#endif // HIDE_RADIO
}

void
Radio::changeStationRequestReturn( Request* req )
{
    if ( req == m_currentChangeRequest )
    {
        m_currentChangeRequest = 0;
    }

    if ( req->aborted() )
    {
        return;
    }

    ChangeStationRequest* request = static_cast<ChangeStationRequest*>( req );

    if ( request->failed() )
    {
        if ( request->resultCode() == ChangeStation_InvalidSession )
        {
            reHandshake();        
        }
        else
        {
            // Let Container deal with all the error conditions and display appropriate messages
            emit error( static_cast<RadioError>( request->resultCode() ),
                        request->errorMessage() );

            // Reset the GUI state
            setState( State_Stopping );
            setState( State_Stopped );
            
            // The cache for the playlist in audiocontroller is now inaccurate
            m_resumePossible = false;
        }        
        
        return;
    }

    LOGL( 3, "Adjust succeeded, now fetch playlist" );

    // All stations but previews get their station name here
    m_stationName = request->stationName();

    if ( !request->stationName().isEmpty() )
    {
        // if it's empty, it's prolly a preview track, which means we get
        // the stationname from the Xspf instead, in onPlaylistLoaded
        addStationToHistory( m_stationUrl, m_stationName );
    }

    // Previews have their XSPF already in the return from adjust
    if ( request->hasXspf() )
    {
        // Set the state first because the setXspf call will lead to a
        // synchronous call to onPlaylistLoaded
        setState( State_FetchingPlaylist );
        m_playlist.setXspf( request->xspf() );
    }
    else
    {
        // Don't store previews (i.e. the ones that have XSPF)
        The::currentUser().setResumeStation( m_stationUrl );
        
        m_playlist.setBasePath( m_basePath );
        m_playlist.setSession( m_session );
        setState( State_FetchingPlaylist );
    }
}

void
Radio::onPlaylistLoaded( const QString& stationName,
                         int skipsLeft )
{
    if ( stationName != m_stationName && !stationName.isEmpty() )
    {
        // For previews, all we get is XSPF so this is the only place we get
        // the station name
        m_stationName = stationName;
        
        // OK, we don't want to add previews of single tracks to the history
        if ( m_playlist.size() > 1 )
        {
            addStationToHistory( m_stationUrl, m_stationName );
        }
    }
    
    m_skipsLeft = skipsLeft;

    LOGL( 3, "Playlist loaded, skips left: " << m_skipsLeft );

    if ( m_state == State_FetchingPlaylist )
    {
        // This means we've stopped
        
        // Set state before calling play as otherwise the play call might
        // lead to state change signals bubbling up to us as part of the same
        // call stack and wrong things will happen.
		setState( State_FetchingStream );
        m_audioController.play( m_playlist );
	}

    if ( m_playlist.type() == RadioPlaylist::Type_Station )
    {
        m_resumePossible = true;
    }
}


void
Radio::onPlaylistError( RadioError errorCode,
                        const QString& message )
{
    if ( errorCode == (int)Playlist_InvalidSession )
    {
        LOGL( 2, "Playlist returned invalid session, scheduling rehandshake..." );

        if ( m_state == State_FetchingPlaylist )
        {
            // Nothing is playing, re-handshake immediately
            reHandshake();
        }
        else
        {
            // Wait until current track is finished before re-handshaking
            m_handshakeAfterNext = true;
        }
    }
    else
    {
        if ( m_state == State_FetchingPlaylist )
        {
            emit error( errorCode, message );
    
            // Will only be in this state when there is no playlist left
            // so safe to stop as there is no track playing.
            stop();
        }
        else
        {
            // If we're already playing a track, we don't emit an error
            // until we're totally out of playlist.
            LOGL( 2, "Playlist fetching failed but just keep playing what we've got left of current playlist" );

            m_cachedPlaylistErrorCode = errorCode;
            m_cachedPlaylistErrorMessage = message;
        }
    }
}                        


void
Radio::reHandshake()
{
    setState( State_Uninitialised );
    playStation( m_stationUrl );
}


void
Radio::resumeStation()
{
#ifndef HIDE_RADIO

    // The GUI should make sure the play button is not enabled when the radio
    // is in any other state than these.
    Q_ASSERT( m_state == State_Stopped || m_state == State_Handshaken );

    LOGL( 3, "Resuming station" );

    m_stationWatch.reset();

    if ( !m_resumePossible )
    {
        StationUrl url = The::currentUser().resumeStation();
        
        if ( url.isEmpty() )
        {
            //url = "lastfm://user/" + QUrl::toPercentEncoding( m_username ) + "/personal";

            // Putting this back for now as personal radio aren't available to non-subscribers
            emit error( Radio_IllegalResume,
                i18n( "Can't resume a station without having listened to one first." ) );
        }

        playStation( url );
    }
    else
    {
        // Setting the session again is needed to kick off the playlist
        // again in case it's run out of XSPF.
        m_playlist.setSession( m_session );     
        m_audioController.play();
    }

#endif // HIDE_RADIO
}


void
Radio::stop()
{
#ifndef HIDE_RADIO

    LOGL( 3, "Stopping radio" );

    m_stationWatch.stop();

    switch ( m_state )
    {
        case State_Uninitialised:
        case State_Handshaking:
        case State_Handshaken:
        case State_Stopping:
        case State_Stopped:
        {
            // Do nothing
        }
        break;
        
        case State_ChangingStation:
        {
            if ( m_currentChangeRequest )
            {
                m_currentChangeRequest->abort();
            }

            // Can emit both here as we don't need to stop the AudioController,
            // i.e. there's no asynchronous streaming stuff going on yet.
            setState( State_Stopping );
            setState( State_Stopped );
        }
        break;
        
        case State_FetchingPlaylist:
        {
            m_playlist.abort();

            setState( State_Stopping );
			setState( State_Stopped );
        }
        break;

        case State_FetchingStream:
        case State_StreamFetched:
        case State_Buffering: 
        case State_Streaming:
        case State_Skipping:
        {
            m_playlist.abort();
            m_audioController.stop();
            setState( State_Stopping );
        }
        break;

        default:
            Q_ASSERT( !"Unhandled case" );
    }
    

#endif // HIDE_RADIO
}
    
void
Radio::skip()
{
#ifndef HIDE_RADIO

    LOGL( 3, "Radio skip" );

    m_trackWatch.stop();
    m_stationWatch.stop();

    #ifdef SKIP_LIMIT_ENABLED
        if ( m_skipsLeft != 0 )
        {
            m_audioController.loadNext();
            m_skipsLeft--;
        }
        else
        {
            emit error( Radio_SkipLimitExceeded, i18n( "Skip limit exceeded." ) );
        }   
    #else
        m_audioController.loadNext();
    #endif // SKIP_LIMIT_ENABLED

#endif // HIDE_RADIO
}


void
Radio::onAudioControllerStateChanged( RadioState newState )
{
    // Stop and resume stopwatch when buffering
    if ( newState == State_Buffering && m_state == State_Streaming )
    {
        m_stationWatch.stop();
        m_trackWatch.stop();
    }
    else if ( newState == State_Streaming && m_state == State_Buffering )
    {
        if ( m_trackWatch.getTime() != 0 )
        {
            LOGL( 3, "Sending a rebuffer report" );
            ReportRebufferingRequest* r = new ReportRebufferingRequest();
            QString host = QUrl( m_audioController.currentTrackUrl() ).host();
            r->setStreamerHost( host );
            r->setUserName( The::currentUsername() );
            r->start();
        }

        m_stationWatch.start();
        m_trackWatch.start();
    }

    if ( m_state == State_Uninitialised ||
         m_state == State_Handshaking ||                 
         m_state == State_Handshaken ||                 
         m_state == State_ChangingStation ||                 
         m_state == State_FetchingPlaylist )
    {
        // Don't let audio controller override these states as we manage
        // them ourselves
    }
    else if ( newState == State_Stopped && m_state != State_Stopping )
    {
        // Just an in-between tracks stop, don't forward
    }
    else
    {
        // Forward all other states on
        setState( newState );
    }                 

}


void
Radio::onAudioControllerError( RadioError err,
                               const QString& message )
{
    if ( err == Radio_OutOfPlaylist )
    {
        if ( !m_cachedPlaylistErrorMessage.isEmpty() )
        {
            emit error( m_cachedPlaylistErrorCode, m_cachedPlaylistErrorMessage );
            m_cachedPlaylistErrorCode = (RadioError)-1;
            m_cachedPlaylistErrorMessage.clear();
        }
        else if( !m_playlist.isOutOfContent() )
        {
            LOGL( 3, "Radio out of playlist but station yet not out of content. Going to State_FetchingPlaylist." );
        
            // This can happen if the user skipped quickly and the playlist
            // has yet to fetch the next chunk. onPlaylistLoaded will kick off
            // the stream again.
            setState( State_FetchingPlaylist );
        }
        else if ( m_playlist.type() == RadioPlaylist::Type_Playlist )
        {
            // Don't emit error for a preview playlist coming to an end
        }
        else
        {
            emit error( err, message );
        }
    }
    else
    {
        if ( err == Radio_PluginLoadFailed )
        {
            m_broken = true;
        }

        // TODO: need to test this when a session invalidates and causes
        // a re-handshake. Will the streamer throw an error that will be
        // propagated to the GUI via this function?
        emit error( err, message );
    }
}


void
Radio::setState( RadioState newState )
{
    if ( newState != m_state )
    {
        LOGL( 4, "Radio state: " << radioState2String( newState ) );

        m_state = newState;

        emit stateChanged( newState );
    }
}

void
Radio::onTrackChanged( TrackInfo& track, const TrackInfo& previous )
{
    Q_UNUSED( previous )

    LOGL( 4, "Radio sending " << track.toString() << " to listener." );
    
    track.setStopWatch( &m_trackWatch );

    //CPlayerListener& listener = The::app().listener();
    //CPlayerCommand cmd( PCMD_START, MooseConstants::kRadioPluginId, track );
    //listener.Handle( cmd );
}

void
Radio::onTrackStarted( const TrackInfo& track )
{
    Q_UNUSED( track )

    m_trackWatch.start();
    m_stationWatch.start();
}

void
Radio::onTrackEnded( const TrackInfo& track, int at )
{
    Q_UNUSED( at )

    m_trackWatch.stop();
    m_trackWatch.reset();
    m_stationWatch.stop();
    
    LOGL( 4, "Radio sending stop for " << track.toString() << " to listener." );

    //CPlayerListener& listener = The::app().listener();
    //CPlayerCommand cmd( PCMD_STOP, MooseConstants::kRadioPluginId, track );
    //listener.Handle( cmd );

    if ( m_handshakeAfterNext )
    {
        m_audioController.stop();
        m_handshakeAfterNext = false;
        reHandshake();
    }
}


void
Radio::addStationToHistory( QString url, QString name )
{
    Station station;
    station.setUrl( url );
    station.setName( name );
    The::currentUser().addRecentStation( station );
}


namespace The
{
    AudioController &audioController()
    {
        return The::radio().m_audioController;
    }
}
