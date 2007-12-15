/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

#ifndef AUDIOCONTROLLER_H
#define AUDIOCONTROLLER_H

#include "RadioPlaylist.h"
#include "RadioEnums.h"
//#include "interfaces/OutputInterface.h"

#include "MooseCommon.h"
#include "TrackInfo.h"

#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QPointer>
#include <QWaitCondition>

class InputInterface;
class TranscodeInterface;
class AudioControllerEventHandler;
class ProxyOutput;
class OutputInterface;

#if 0
/*************************************************************************/ /**
    Thread class that handles the audio streaming via a separate thread.
    Controls the streaming, transcoding and playback plugins.
    
    (I would have made this a nested private class of the AudioController
    but Qt doesn't allow signals and slots in nested classes.)
    
    This is a worker thread with its own event loop which deals with the
    streaming separate from the main GUI event loop. The audio controller
    sends commands to this thread via asynchronous posted events. This might
    be a clumsy approach, I don't know, but signals and slots just didn't
    feel right as the communication is only between the AudioController
    and its owned thread and therefore it shouldn't be emitting signals
    to all and sundry just to control its tightly coupled worker thread.
    
    This thread only plays one track at a time and has no concept of
    skipping. It is either preparing to stream, streaming, stopping or
    stopped. When a track stops, it emits a stopped signal to the
    AudioController which then serves it the next track if there are
    more in the playlist or stops if there are none.
    
    It's an error to send a Play event to the thread while it's playing.
    You must send a Stop event and wait for completion before sending a new
    Play.
******************************************************************************/
class AudioControllerThread : public QThread
{
    Q_OBJECT

public:

    friend class AudioController;

    // Events needed for async communication with audio thread
    enum EventType
    {
        Event_Play = QEvent::User,
        Event_Stop,
        Event_Volume,
		Event_ResetBufferSize
    };

    /*********************************************************************/ /**
        Send to thread to request it to start streaming a URL.
    **************************************************************************/
    class PlayEvent : public QEvent
    {
    public:
        PlayEvent( QString url, QString sesh ) :
            QEvent( (QEvent::Type)Event_Play ),
            m_url( url ), m_session( sesh ) {}
        QString m_url;
        QString m_session;
    };

    /*********************************************************************/ /**
        Send to thread to stop streaming.
    **************************************************************************/
    class StopEvent : public QEvent
    {
        public: StopEvent() : QEvent( (QEvent::Type)Event_Stop ) {}
    };

    /*********************************************************************/ /**
        Send to thread to request it to change the volume.
    **************************************************************************/
    class VolumeEvent : public QEvent
    {
    public:
        VolumeEvent( int vol ) : QEvent( (QEvent::Type)Event_Volume ), m_volume( vol ) {}
        int m_volume;
    };

    /*********************************************************************/ /**
        Send to thread to reset http buffer size.
    **************************************************************************/
    class ResetBufferSizeEvent : public QEvent
    {
        public: ResetBufferSizeEvent() : QEvent( (QEvent::Type)Event_ResetBufferSize ) {}
    };

	/*********************************************************************/ /**
        Ctor
    **************************************************************************/
    AudioControllerThread( QObject* parent );

    /*********************************************************************/ /**
        Get in-thread object for posting events to.
    **************************************************************************/
    AudioControllerEventHandler*
    eventHandler() { return m_handler; }
    
    /*********************************************************************/ /**
        Get state.
    **************************************************************************/
    RadioState
    state() { QMutexLocker locker( &m_mutex ); return m_state; }

    /*********************************************************************/ /**
        Asynchronous events from main thread are received through this
        method.
    **************************************************************************/
    virtual bool
    event( QEvent* e );

signals:
    /** \brief Gets emitted frequently during playback to signal the current position in the stream.
      * \param seconds How many seconds elapsed since this stream started. */
    void timeChanged( int seconds );
    
    /** \brief Gets emitted when the playback of a track starts. */
    void trackStarted();

    /** \brief Gets emitted when the playback of a track stops.
      * \param atPosition How many seconds of the song were played before it ended. */
    void trackEnded( int atPosition );

    void stateChanged( RadioState newState );

    void error( int error, const QString& reason );

    void httpBufferSizeChanged( int size );
    void decodedBufferSizeChanged( int size );
    void outputBufferSizeChanged( int size );

protected:
    // Qt thread function
    virtual void run();

private:
    void play( const QString& url, const QString& session );
    void playLoop();
    void stop();
    void endTrack();

    bool loadPlugins();
    QObject* loadPlugin( QString name );
    void updateUserHttpBufferSize();
    
    InputInterface* m_input;
    TranscodeInterface* m_transcode;
    OutputInterface* m_output;
    ProxyOutput* m_proxyOutput;

    AudioControllerEventHandler* m_handler;
    
    // Owner in main thread will wait for this at startup
    QWaitCondition m_initialised;

    QTimer* m_timer;

    // Technically, these objects "live" in the main thread as they
    // are created at construct time, but it shouldn't matter as they're
    // only accessed by the worker thread.
    RadioState m_state;
    QTime m_timeSinceLastPause;
    int m_timeElapsedAtLastPause;
    
    bool m_automaticBufferHandling;

    QMutex m_mutex;

private slots:

    /** \brief Sets the stream's samplerate and channel information.
      * \param sampleRate Stream's samplerate (e.g. 44100)
      * \param channels Amount of channels that are used (e.g. 2 for stereo)
      *
      * Transcode plugins must call this method to set
      * the current stream's metadata like its samplerate
      * and how many channels are used */
    void setStreamData( long sampleRate, int channels );
    
    void startTimer();
    void stopTimer();

    void onTimerTimeout();

    void onInputStateChanged( RadioState state );
    void setState( RadioState state );

};


/*************************************************************************/ /**
    This is pretty ugly but this class is needed to be able to pass events
    to the worker thread. The AudioControllerThread can't be used as the
    receiver as that object itself lives in the main thread (constructed
    in AudioController) and the events would end up getting handled as part
    of the main thread which is not what we want.
******************************************************************************/
class AudioControllerEventHandler : public QObject
{

public:
    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
    AudioControllerEventHandler( AudioControllerThread* parent ) :
            m_parent( parent ) {}

    /*********************************************************************/ /**
        Will just forward event on the the AudioControllerThread.
    **************************************************************************/
    virtual bool
    event( QEvent* e ) { return m_parent->event( e ); }

private:    
    AudioControllerThread* m_parent;
};
#endif

/*************************************************************************/ /**
    The AudioController manages playback of audio, it can take individual
    tracks or a playlist and manage the changing of tracks automatically.
    
    Encapsulates the AudioControllerThread and deals with all the
    asynchronous communication.
    
    It's perfectly fine to call play on this class without having sent a
    stop first, the details are taken care of internally.
******************************************************************************/
class AudioController : public QObject
{
    Q_OBJECT

public:

    friend class AudioControllerThread;
    friend class DiagnosticsDialog;

    AudioController();
    ~AudioController();

    /** \brief Returns true if the playback is running. */
    bool isPlaying() const;

    /** \brief Returns the currently playing track. */
    const TrackInfo& currentTrack() const { return m_currentTrack; }

    /** \brief Returns the streamer URL of the currently playing track. */
    const QString& currentTrackUrl() const { return m_currentTrackUrl; }

public slots:
    /** \brief Loads and starts this file instantly.
      * \param url The url that you want to load. Can be a file path or
      *            an Internet URL for example.
      *
      * This method initialises a stream from url and starts
      * playback. If the playback queue contains a track with
      * this url, it will be removed automatically. */
    void play( const QUrl& trackUrl );

    /** \brief Loads and starts this track instantly.
      * \param track The track that you want to load.
      *
      * This method initialises a stream for this track and
      * starts playback. If the playback queue contains this
      * track, it will be removed automatically. */
    void play( const TrackInfo& track );

    /** \brief Starts playback from a playlist.
      * \param playlist Playlist used to retrieve tracks to play.
      
      * This method initialises a stream for and starts playback. */
    void play( RadioPlaylist& playlist );

    /** \brief Starts playback from current playlist if we have one. */
    void play();

    /** \brief Loads and starts the next song in the queue.
      *
      * This method starts playback of the next song in the queue. */
    void loadNext();

    /** \brief Stops playback. */
    void stop();

    /** \brief Pause/resume the playback. */
    //void togglePause();

    void setVolume( int vol );

    QStringList soundSystems() { return m_soundSystems; }
    QStringList devices() { return m_devices; }

signals:
    /** \brief Gets emitted when our state changes. */
    void stateChanged( RadioState newState );

    /** \brief Gets emitted when the playback of a track stops.
      * \param track The track that just has ended.
      * \param atPosition How many seconds of the song were played before it ended. */
    void trackEnded( const TrackInfo& track, int atPosition );

    /** \brief Gets emitted when a new track is about to start.
      * \param track The track about to start playing.
      * \param track The previous track that just finished playing. */
    void trackChanged( TrackInfo& track, const TrackInfo& previousTrack );

    /** \brief Gets emitted when a new track has been buffered and actually starts streaming
      * \param track The track starting. */
    void trackStarted( const TrackInfo& track );

    void error( RadioError error, const QString& reason );

    /*********************************************************************/ /**
        See HttpInput header.
    **************************************************************************/
    void
    buffering( int size,
               int total );    

private:

    //void
    //doPlay( const TrackInfo& track );

    //RadioState m_state;

    //QPointer<RadioPlaylist> m_playlist;

    //AudioControllerThread m_thread;
    TrackInfo m_currentTrack;
//    TrackInfo m_pendingTrack;
//
    QString m_currentTrackUrl;
//    
//    int m_streamerErrorCounter;
//    bool m_retryNextTrack;
//    
    QStringList m_soundSystems;
    QStringList m_devices;
//    
//    QMutex m_mutex;
//    
//private slots:
//
//    void onTrackStarted();
//
//    /** Called when a track finishes naturally, i.e. is not stopped
//      * or skipped. */
//    void onTrackEndReached( int atPosition );
//
//    /** We need to do some translation of the thread's state wrt to skipping etc */
//    void onThreadStateChanged( RadioState newState );
//
//    void onStreamingError( int error, const QString& reason );
//
//    void setState( RadioState state );

};

#endif // AUDIOCONTROLLER_H


