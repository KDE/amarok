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
 * This class captures Amarok specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 */

class AMAROK_EXPORT EngineController : public QObject, public EngineSubject
{
    Q_OBJECT

public:
    static EngineController* instance();
    static void destroy();

    static bool canDecode( const KUrl& );

    /* returns track position (elapsed time) in seconds */
    int trackPosition() const;

    /* returns track position (elapsed time) in milliseconds */
    int trackPositionMs() const;

    /**
     * Returns the current track that is loaded into the engine.
     * @return a Meta::TrackPtr which is either the track, or empty if phonon
     * has a state of Phonon::ErrorState or Phonon::StoppedState
     */
    Meta::TrackPtr currentTrack() const;
    int trackLength() const;

    /**
     * Used to enqueue a track before it starts to play, for gapless playback.
     */
    void setNextTrack( Meta::TrackPtr );

    void restoreSession();
    void endSession();

    Phonon::State state() const { return phononMediaObject()->state(); }

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 150;

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    /*AMAROK_EXPORT*/ static bool installDistroCodec();

    const Phonon::MediaObject* phononMediaObject() const { return m_media; } //!const so that it's only used by DBus for info
    int volume() const;
    bool isMuted() const;
    bool isPaused() const;
    bool loaded() { return phononMediaObject()->state() != Phonon::StoppedState; }
    bool isStream();
    enum PlayerStatus
    {
        Playing  = 0,
        Paused   = 1,
        Stopped  = 2,
        Error    = -1
    };

public slots:
    void play();
    void play( const Meta::TrackPtr&, uint offset = 0 );
    void pause();
    void stop( bool forceInstant = false );
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int ms );
    void seekRelative( int ms );
    void seekForward( int ms = 10000 );
    void seekBackward( int ms = 10000 );

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void setMuted( bool mute );
    void toggleMute();

signals:
    void trackPlayPause( int ); //Playing: 0, Paused: 1
    void trackFinished();
    void trackChanged( Meta::TrackPtr );
    void trackSeeked( int ); //return relative time in million second
    void volumeChanged( int );
    void muteStateChanged( bool );

protected:
    void playUrl( const KUrl &url, uint offset );
    void trackDone();

private slots:
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
