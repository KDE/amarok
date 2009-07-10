/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_ENGINECONTROLLER_H
#define AMAROK_ENGINECONTROLLER_H

#include "meta/capabilities/BoundedPlaybackCapability.h"
#include "EngineObserver.h"
#include "meta/Meta.h"

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QPointer>

#include <Phonon/Path>
#include <Phonon/MediaController>
#include <Phonon/MediaSource> //Needed for the slot

class QTimer;

namespace KIO { class Job; }
namespace Meta { class MultiPlayableCapability; class MultiSourceCapability; }
namespace Phonon { class AudioOutput; class MediaObject; class VolumeFaderEffect; }

/**
 * A thin wrapper around Phonon that implements Amarok-specific funtionality like
 * replay gain, fade-out on stop and various track capabilities that affect
 * playback.
 */
class AMAROK_EXPORT EngineController : public QObject, public EngineSubject
{
    Q_OBJECT

public:
    /**
     * Returns the global EngineController instance
     */
    static EngineController* instance();
    /**
     * Destroys the global EngineController instance
     */
    static void destroy();

    /**
     * Loads and plays the track that was playing when endSession() was last
     * called (ie: when Amarok was quit)
     */
    void restoreSession();
    /**
     * Saves the currently playing track and the playing/paused/stopped state
     */
    void endSession();

    /**
     * Checks whether the media file at the specified URL can be decoded
     */
    static bool canDecode( const KUrl& );

    /** @return track position (elapsed time) in seconds */
    int trackPosition() const;

    /** @return track position (elapsed time) in milliseconds */
    int trackPositionMs() const;

    /**
     * Returns the current track that is loaded into the engine.
     * @return a Meta::TrackPtr which is either the track, or empty if phonon
     * has a state of Phonon::ErrorState or Phonon::StoppedState
     */
    Meta::TrackPtr currentTrack() const;
    /**
     * @return the length of the current track
     */
    int trackLength() const;

    /**
     * Used to enqueue a track before it starts to play, for gapless playback.
     *
     * This will clear any tracks currently in the queue.  If no track is playing,
     * @p track will be played immediately.
     */
    void setNextTrack( Meta::TrackPtr track );

    /**
     * The state of the engine
     */
    Phonon::State state() const { return phononMediaObject()->state(); }

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    /*AMAROK_EXPORT*/ static bool installDistroCodec();

    /**
     * Provides access to the Phonon MediaObject for components that need more information
     */
    // const so that it can only be used for info
    const Phonon::MediaObject* phononMediaObject() const { return m_media; }

    /**
     * Gets the volume
     * @return the volume as a percentage
     */
    int volume() const;

    /**
     * @return @c true if sound output is disabled, @false otherwise
     */
    bool isMuted() const;

    /**
     * @return @c true if Amarok is paused, @c false if it is stopped or playing
     */
    bool isPaused() const;

    /**
     * Streams sometimes have to be treated specially.  For example, it is typically
     * not possible to rewind a stream (at least, not without returning to the
     * start of it).
     *
     * @return @c true if the current track is a stream, @c false otherwise
     */
    bool isStream();

    // used by signals
    enum PlayerStatus
    {
        Playing  = 0,
        Paused   = 1,
        Stopped  = 2,
        Error    = -1
    };

public slots:
    /**
     * Plays the current track, if there is one
     *
     * This happens asynchronously.  Use EngineObserver to find out when it actually happens.
     */
    void play();
    /**
     * Plays the specified track
     *
     * This happens asynchronously.  Use EngineObserver to find out when it actually happens.
     */
    void play( const Meta::TrackPtr&, uint offset = 0 );
    /**
     * Pauses the current track
     *
     * This happens asynchronously.  Use EngineObserver to find out when it actually happens.
     */
    void pause();
    /**
     * Stops playing
     *
     * This happens asynchronously.  Use EngineObserver to find out when it actually happens.
     */
    void stop( bool forceInstant = false );
    /**
     * Pauses if Amarok is currently playing, plays if Amarok is stopped or paused
     *
     * This happens asynchronously.  Use EngineObserver to find out when it actually happens.
     */
    void playPause(); //pauses if playing, plays if paused or stopped

    /**
     * Seeks to a position in the track
     *
     * If the media is not seekable, or the state is something other than
     * PlayingState, BufferingState or PausedState, has no effect.
     *
     * Deals correctly with tracks that have the BoundedPlayback capability.
     *
     * @param ms the position in milliseconds (counting from the start of the track)
     */
    void seek( int ms );
    /**
     * Seeks forward or backward in the track
     *
     * If the media is not seekable, or the state is something other than
     * PlayingState, BufferingState or PausedState, has no effect.
     *
     * Deals correctly with tracks that have the BoundedPlayback capability.
     *
     * A negative value seeks backwards, a positive value seeks forwards.
     *
     * If the value of @p ms would move the position to before the start of the track,
     * the position is moved to the start of the track.
     *
     * @param ms the offset from the current position in milliseconds
     */
    void seekRelative( int ms );
    /**
     * Seeks forward in the track
     *
     * Same as seekRelative()
     */
    void seekForward( int ms = 10000 );
    /**
     * Seeks backward in the track
     *
     * Works identically to seekRelative(), but seeks in the opposite direction.
     */
    void seekBackward( int ms = 10000 );

    /**
     * Increases the volume
     *
     * @param ticks the amount to increase the volume by, given as a percentage of the
     * maximum possible volume (ie: the same units as for setVolume()).
     */
    int increaseVolume( int ticks = 100/25 );
    /**
     * Decreases the volume
     *
     * @param ticks the amount to decrease the volume by, given as a percentage of the
     * maximum possible volume (ie: the same units as for setVolume()).
     */
    int decreaseVolume( int ticks = 100/25 );
    /**
     * Sets the volume
     *
     * @param percent the new volume as a percentage of the maximum possible volume.
     */
    // this amplifier does not go up to 11
    int setVolume( int percent );

    /**
     * Mutes or unmuted playback
     *
     * @param mute if @c true, audio output will be disabled; if @c false, audio output
     * will be enabled.
     */
    void setMuted( bool mute );
    /**
     * Toggles mute
     *
     * Works like setMuted( !isMuted() );
     */
    void toggleMute();

signals:
    // this stuff seems to be for the scripting support
    void trackPlayPause( int ); //Playing: 0, Paused: 1
    void trackFinished();
    void trackChanged( Meta::TrackPtr );
    void trackSeeked( int ); //return relative time in million second
    void volumeChanged( int );
    void muteStateChanged( bool );

private slots:
    /**
     * Sets up the Phonon system
     */
    void initializePhonon();
    void slotQueueEnded();
    void slotAboutToFinish();
    void slotNewTrackPlaying( const Phonon::MediaSource &source);
    void slotStateChanged( Phonon::State newState, Phonon::State oldState);
    void slotPlayableUrlFetched(const KUrl&);
    void slotTick( qint64 );
    void slotTrackLengthChanged( qint64 );
    void slotMetaDataChanged();
    void slotStopFadeout(); //called after the fade-out has finished

    /**
     *  Notify the engine that a new title has been reached when playing a cd. This
     *  is needed as a cd counts as basically one lone track, and we want to be able
     *  to play something else once one track has finished 
     */
    void slotTitleChanged( int titleNumber );

private:
    /**
     * Plays the media at a specified URL
     *
     * @param url the URL of the media
     * @param offset the position in the media to start at in milliseconds
     */
    void playUrl( const KUrl &url, uint offset );

    static EngineController* s_instance;
    EngineController();
    ~EngineController();

    Q_DISABLE_COPY( EngineController )

    QPointer<Phonon::MediaObject>       m_media;
    QPointer<Phonon::VolumeFaderEffect> m_preamp;
    QPointer<Phonon::AudioOutput>       m_audio;
    QPointer<Phonon::VolumeFaderEffect> m_fader;
    QPointer<Phonon::MediaController>   m_controller;
    Phonon::Path                        m_path;

    Meta::TrackPtr  m_currentTrack;
    Meta::TrackPtr  m_lastTrack;
    Meta::TrackPtr  m_nextTrack;
    KUrl            m_nextUrl;
    QPointer<Meta::BoundedPlaybackCapability> m_boundedPlayback;
    QPointer<Meta::MultiPlayableCapability> m_multiPlayback;
    QPointer<Meta::MultiSourceCapability> m_multiSource;
    bool m_playWhenFetched;
    QTimer* m_fadeoutTimer;
    int m_volume;

    QMutex m_mutex;
};

namespace The {
    AMAROK_EXPORT EngineController* engineController();
}


#endif
