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

#include "AudioController.h"
#include "core/Radio.h"
#include "EngineController.h"
#include "LastFmService.h"
#include "RadioAdapter.h"

#include <KLocale>

AudioController::AudioController( QObject *parent )
    : QObject( parent ), EngineObserver( The::engineController() )
{
}


AudioController::~AudioController()
{
}


void
AudioController::setVolume( int vol )
{
    The::engineController()->setVolume( vol );
}


void
AudioController::play()
{
    loadNext();
}


void
AudioController::play( RadioPlaylist &playlist )
{
    m_playlist = &playlist;
    loadNext();
}


void
AudioController::play( const QUrl &trackUrl )
{
    TrackInfo ti;
    ti.setPath( trackUrl.toString() );
    play( ti );
}


void
AudioController::play( const TrackInfo &track )
{
    m_playlist = 0;
    playTrack( track );
}


void
AudioController::stop()
{
    // do nothing, we ignore the radio's plea to stop and handle it elsewhere
}


void
AudioController::loadNext()
{
    if ( m_playlist == 0 )
    {
        The::lastFmService()->radio()->stop();
    }
    else if ( m_playlist->hasMore() )
    {
        TrackInfo track = m_playlist->nextTrack();
        playTrack( track );
    }
    else
    {
        emit error( Radio_OutOfPlaylist, i18n( "Radio playlist has ended." ) );
    }
}


QString
AudioController::currentTrackUrl() const
{
    LastFm::TrackPtr track = The::lastFmService()->radio()->currentTrack();
    return track ? track->uidUrl() : "";
}


void
AudioController::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    if( currentState != oldState )
    {
        switch( currentState )
        {
            case Phonon::StoppedState:
            case Phonon::LoadingState:
                emit stateChanged( State_Stopping );
                emit stateChanged( State_Stopped );
                break;

            case Phonon::PlayingState:
                emit stateChanged( State_Streaming );
                break;

                //FIXME: This is completely incorrect!  state Changes get triggered for all tracks.  This causes pausing anything to stop playback...
//             case Phonon::PausedState:
//                 // not supposed to pause the radio, so we'll just stop it
//                 The::engineController()->stop();
//                 break;
            default:
                break;
        }
    }
}


void
AudioController::enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason )
{
    Q_UNUSED( trackLength );
    if( The::lastFmService()->radio()->currentTrack() )
    {
        emit trackEnded( m_currentTrackInfo, finalPosition );
        if( reason == EndedStopped )
            The::lastFmService()->radio()->stop();
    }
}


void
AudioController::engineNewTrackPlaying()
{
    if( The::lastFmService()->radio()->currentTrack() )
    {
        if( Meta::TrackPtr::staticCast( The::lastFmService()->radio()->currentTrack() ) == The::engineController()->currentTrack() )
            emit trackStarted( m_currentTrackInfo );
        else
            The::lastFmService()->radio()->stop();
    }
}


void
AudioController::playTrack( const TrackInfo &track )
{
    LastFm::TrackPtr lfmTrack = The::lastFmService()->radio()->currentTrack();
    if( lfmTrack )
    {
        if( !m_currentTrackInfo.isEmpty() )
        {
            TrackInfo old = m_currentTrackInfo;
            m_currentTrackInfo = track;
            emit trackChanged( m_currentTrackInfo, old );
        }
        else
        {
            m_currentTrackInfo = track;
        }
        lfmTrack->setTrackInfo( track ); // will emit new playable url and kick off playback
    }
}
