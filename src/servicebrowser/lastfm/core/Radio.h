/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef RADIO_H
#define RADIO_H

#include "RadioPlaylist.h"
#include "AudioController.h"
#include "RadioEnums.h"
#include "StopWatch.h"
#include "MooseCommon.h"
#include "StationUrl.h"

/*************************************************************************/ /**
    Facade to the rest of the world for radio streaming functionality.
    
    When I started designing this new subsystem, a well-defined state system
    seemed to be the answer to the somewhat spaghetti-like structure of the
    old signal/slot-drive system. Each of the main classes used have a
    state member of type RadioState. Not all states from this enum are
    valid for all classes but it was simpler to reuse the enum rather than
    having each class define their own enum with many duplicated entries.
    
    The states valid for each classes are asserted for at the beginning of
    each class's setState function.
    
    In the end, this state system became a lot more convoluted than I'd
    wanted, and is in places far from obvious. Each parent class that
    receives state changes from its owned child class filters the new state
    based on its own current state and only adopts the new state of the child
    class if it makes sense to do so. This state propagation logic isn't
    entirely obvious so think carefully before hacking on it. I've tried
    commenting all the non-obvious stages here though.
    
    Another unfortunate side effect of using these states is that almost
    every function becomes a case statement to be able to take the right
    action depending on the current state.
    
******************************************************************************/
namespace The
{
    AudioController &audioController();
}


class Radio : public QObject
{
    Q_OBJECT

    friend AudioController &The::audioController();

public:

    Radio( QObject* parent );

    /*********************************************************************/ /**
        Call this to handshake once the app has a current user.
    **************************************************************************/
    void
    init( QString username,
          QString password,
          QString version );

    RadioState
    state() { return m_state; }

    QString
    stationName() { return m_stationName; }

    StationUrl
    stationUrl() { return m_stationUrl; }

    QString
    sessionKey() { return m_session; }

    bool
    isPlaying() { return m_state == State_Streaming; }
    
    QStringList
    soundSystems() { return m_audioController.soundSystems(); }

    QStringList
    devices() { return m_audioController.devices(); }

    void
    setDiscoveryMode( bool enabled );

    StopWatch&
    stationStopWatch() { return m_stationWatch; }
    
public slots:

    /*********************************************************************/ /**
        @param lastfmUrl - a URL of the form lastfm://play/tracks/123
    **************************************************************************/
    void
    playStation( StationUrl lastfmUrl );
    
    /** convenience function - you don't have to percent encode anything, Qt does it */
    void
    playStation( QUrl url ) 
    {
        playStation( StationUrl( QString::fromUtf8( url.toEncoded() ) ) );
    }
    
    void
    play( Track track );

    /*********************************************************************/ /**
        Can be called to resume play of the last played station. Illegal
        to call this directly after startup without having played a station
        first.
    **************************************************************************/
    void
    resumeStation();

    void
    stop();
    
    void
    skip();

    void
    setVolume( int vol ) { m_audioController.setVolume( vol ); }

signals:

    void
    stateChanged( RadioState newState );

    void
    error( RadioError errorCode, const QString& message );

    /*********************************************************************/ /**
        See HttpInput header.
    **************************************************************************/
    void
    buffering( int size, int total );

private:
    void
    handshake();

    /*********************************************************************/ /**
        Re-handshakes and reinitialises current station. Called when a session
        times out.
    **************************************************************************/
    void
    reHandshake();

    void
    addStationToHistory( QString url, QString name );

    RadioState m_state;
    
    QString m_username;
    QString m_password;
    QString m_version;

    /// 32-bit session token for communicating with radio web services
    QString m_session;

    /// Base path string to pass to radio web services
    QString m_basePath;

    QString m_stationName;
    StationUrl m_stationUrl;
    
    StationUrl m_pendingStation;

    ChangeStationRequest* m_currentChangeRequest;
    
    AudioController m_audioController;
    RadioPlaylist m_playlist;
    
    // Measures progress of current track
    StopWatch m_trackWatch;

    // Measures progress of current station
    StopWatch m_stationWatch;

    int m_skipsLeft; // a value of -1 indicates unlimited skips
    bool m_resumePossible;
    bool m_handshakeAfterNext;

    bool m_broken; // set when we've failed to load plugins
    
    RadioError m_cachedPlaylistErrorCode;
    QString m_cachedPlaylistErrorMessage;

private slots:

    void
    setState( RadioState newState );

    void
    handshakeReturn( Request* req );

    void
    trackToIdReturn( Request* );

    void
    changeStationRequestReturn( Request* req );

    void
    onAudioControllerStateChanged( RadioState newState );
    
    void
    onAudioControllerError( RadioError err,
                            const QString& message );
    
    void
    onTrackChanged( TrackInfo& track,
                    const TrackInfo& previous );
    
    void
    onTrackStarted( const TrackInfo& track );

    void
    onTrackEnded( const TrackInfo& track,
                  int at );

    /*********************************************************************/ /**
        We need this slot when first tuning in so that we don't hand the
        playlist down to the AudioController too early. Once this slot is
        hit, we know there's tracks in the playlist.
    **************************************************************************/
    void
    onPlaylistLoaded( const QString& stationName,
                      int skipsLeft );

    void
    onPlaylistError( RadioError errorCode,
                     const QString& message );

};

namespace The
{
    Radio &radio(); /// defined in Container.cpp
}

#endif // RADIO_H
