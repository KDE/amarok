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

#include "AudioController.h"

// TODO: implement
AudioController::AudioController() {}
AudioController::~AudioController() {}
void AudioController::setVolume( int vol ) {}
void AudioController::play() {}
void AudioController::play( RadioPlaylist& playlist ) {}
void AudioController::play( const QUrl& trackUrl ) {}
void AudioController::play( const TrackInfo& track ) {}
void AudioController::stop() {}
void AudioController::loadNext() {}

#include "Radio.h"
namespace The
{
    Radio &radio()
    {
        static Radio r(0);
        return r;
    }
}

#if 0

#include "interfaces/InputInterface.h"
#include "interfaces/TranscodeInterface.h"
#include "interfaces/OutputInterface.h"
#include "MooseCommon.h"
#include "LastFmSettings.h"
#include "Radio.h"
#include "logger.h"
#include "ProxyOutput.h"

#include "output/RtAudio/rtaudioplayback.h"

#include <QFileInfo>
#include <QMutexLocker>
#include <QPluginLoader>

// RADIO STREAMING CONSTANTS

// How often the audio decompression loop checks if it needs to do any work (ms).
static const int kPollingInterval = 40;

// The optimal sizes of the packets that we shuffle between input,
// transcode and output can be determined based on the polling interval.
// The compressed audio is 128 kbps = 16 kb/s. So we use the following
// formula to work out the optimal packet size in bytes (the extra
// multiplication at the end gives us a bit of leeway and catch-up ability
// so that we're not always down against the bleeding lower edge of the
// buffers):
static const int kCompressedPacketSize = int((16 * 1024) * ( kPollingInterval / 1000.0f ) * 2.0f);

// The uncompressed audio is 1411.2 kbps = 176.4 kb/s
static const int kUncompressedPacketSize = int((176.4f * 1024 ) * ( kPollingInterval / 1000.0f ) * 2.0f);

// Min size of buffer holding decoded audio data, i.e the size the buffer
// needs to be to not return true from the needsData() call.
static const int kDecodedBufferMinSize = 4 * kUncompressedPacketSize;
//static const int kDecodedBufferMinSize = 400000;

// Min size of buffer in the output, i.e the size the buffer
// needs to be to not return true from the needsData() call.
static const int kOutputBufferMinSize = kDecodedBufferMinSize; 
//static const int kOutputBufferMinSize = 400000; 

// Max number of errors in a row before we abort
static const int k_streamerErrorLimit = 5;

AudioControllerThread::AudioControllerThread( QObject* parent ) :
        QThread( parent ),
        m_input( 0 ),
        m_transcode( 0 ),
        m_output( 0 ),
        m_proxyOutput( 0 ),
        m_state( State_Stopped ),
        m_timeElapsedAtLastPause( 0 ),
        m_automaticBufferHandling( true )
{
}


void
AudioControllerThread::run()
{
    // This weird-looking bit of code is there to make this thread not
    // initialise until the parent thread has entered wait.
    static_cast<AudioController*>( parent() )->m_mutex.lock();
    static_cast<AudioController*>( parent() )->m_mutex.unlock();

    // Initialise our in-thread services
    m_handler = new AudioControllerEventHandler( this );

    if ( !loadPlugins() )
    {
        m_initialised.wakeAll();
        return;
    }

    // Can't use this as parent as the parent object has to live in the
    // same thread. As the only purpose of this is automatic deletion, it
    // should work just as fine using the handler object.
    m_input->setParent( m_handler );
    m_transcode->setParent( m_handler );
    m_output->setParent( m_handler );

    m_timer = new QTimer( m_handler );

    // For some utterly perverse reason, we have to specify DirectConnection
    // here. If I leave it as Auto, the input class will on emit think that
    // it needs to post a message to us and that message somehow ends up
    // getting delivered by the main GUI thread so we get the GUI thread
    // coming into onInputStateChanged which is NOT what we want.
    connect( m_input,  SIGNAL( stateChanged( RadioState ) ),
             this,       SLOT( onInputStateChanged( RadioState ) ),
             Qt::DirectConnection );

    connect( m_input,  SIGNAL( buffering( int, int ) ),
             parent(), SIGNAL( buffering( int, int ) ),
             Qt::QueuedConnection );

    connect( m_input,  SIGNAL( error( int, const QString& ) ),
             parent(), SLOT  ( onStreamingError( int, const QString& ) ),
             Qt::QueuedConnection );

    connect( m_transcode, SIGNAL( streamInitialized( long, int ) ),
             this,        SLOT  ( setStreamData( long, int ) ),
             Qt::DirectConnection );

    connect( m_output,  SIGNAL( error( int, const QString& ) ),
             parent(),  SLOT  ( onStreamingError( int, const QString& ) ),
             Qt::QueuedConnection );

    connect( m_timer, SIGNAL( timeout() ),
             this,    SLOT  ( onTimerTimeout() ),
             Qt::DirectConnection );


    // What is this interval exactly?
    // It's how often we send out timeChanged signals (500 ms)
    m_timer->setInterval( 500 );

    m_initialised.wakeAll();

    LOGL( 3, "AudioController thread initialised, starting event loop" );

    // Start our own event loop
    exec();

    // This makes the loaded services auto-delete
    delete m_handler;
    delete m_proxyOutput;
}


bool
AudioControllerThread::loadPlugins()
{
    // TODO: these shouldn't be hard-coded, load up all available
    // plugins and then flip between them at track play time
    m_input = qobject_cast<InputInterface*>( loadPlugin( "httpinput" ) );
    if ( m_input == 0 )
        return false;

    m_input->setBufferCapacity( MooseConstants::kHttpBufferMinSize );
    m_transcode = qobject_cast<TranscodeInterface*>( loadPlugin( "madtranscode" ) );
    if ( m_transcode == 0 )
        return false;

    m_transcode->setBufferCapacity( kDecodedBufferMinSize );

    #ifdef WIN32
        m_output = qobject_cast<OutputInterface*>( loadPlugin( "rtaudioplayback" ) );
    #elif defined LINUX
        m_output = qobject_cast<OutputInterface*>( loadPlugin( "output_alsa" ) );
    #else
        m_output = qobject_cast<OutputInterface*>( loadPlugin( "output_portaudio" ) );
    #endif

    if ( m_output == 0 )
        return false;

    m_output->setBufferCapacity( kOutputBufferMinSize );
    m_output->setDevice( The::settings().soundCard() );

    m_proxyOutput = new ProxyOutput();

    return true;
}


QObject*
AudioControllerThread::loadPlugin( QString name )
{
    QString path = MooseUtils::servicePath( name );
    QObject* plugin = QPluginLoader( path ).instance();

    qDebug() << path;

    Q_ASSERT( plugin );
    if ( plugin == 0 )
    {
        emit error( Radio_PluginLoadFailed,
            tr( "Couldn't load radio service '%1'. The radio will not work." ).arg( name ) );
    }

    return plugin;
}


void
AudioControllerThread::updateUserHttpBufferSize()
{
    bool newAuto = The::settings().isBufferManagedAutomatically();

    if ( !newAuto && m_automaticBufferHandling )
    {
        // Switched off automatic
        int userSize = The::settings().httpBufferSize();
        m_input->setBufferCapacity( userSize );

        LOGL( 3, "Setting user-defined http buffer size: " << userSize );
    }
    else if ( newAuto && !m_automaticBufferHandling )
    {
        // Switched on automatic
        m_input->setBufferCapacity( MooseConstants::kHttpBufferMinSize );
    }

    m_automaticBufferHandling = newAuto;
}


bool
AudioControllerThread::event( QEvent* e )
{
    if ( e->type() == (QEvent::Type)Event_Play )
    {
        // Must send a stop and wait for trackEnded signal before we send this
        // guy a Play event due to the asynchronous thread communication.
        Q_ASSERT( state() == State_Stopped );
        if ( state() != State_Stopped )
        {
            // Better do nothing than letting madness ensue in release builds
            LOGL( 1, "Somehow got a play event when not in stopped state. This is bad." );
            return true;
        }

        PlayEvent* playEvent = static_cast<PlayEvent*>( e );
        play( playEvent->m_url, playEvent->m_session );
        return true;
    }
    else if ( e->type() == (QEvent::Type)Event_Stop )
    {
        // We don't want to come in and set state to Stopping if we're
        // already stopped as there is no play loop and the state would
        // get stuck at stopping.
        if ( state() != State_Stopped )
        {
            // Setting this tells the play loop to exit, when it does,
            // it calls endTrack which sets the state to stopped.
            setState( State_Stopping );
        }
        else
        {
            // Each stop command must get a response even if we're not playing
            emit stateChanged( State_Stopped );
        }

        return true;
    }
    else if ( e->type() == (QEvent::Type)Event_Volume )
    {
        VolumeEvent* volumeEvent = static_cast<VolumeEvent*>( e );
        m_output->setVolume( volumeEvent->m_volume );
        return true;
    }
    else if ( e->type() == (QEvent::Type)Event_ResetBufferSize )
    {
        if ( m_automaticBufferHandling )
        {
            m_input->setBufferCapacity( MooseConstants::kHttpBufferMinSize );
        }
    }

    return QThread::event( e );
}


void
AudioControllerThread::setStreamData( long /*sampleRate*/, int /*channels*/ )
{
    // EJ: got rid of this because RtAudio occasionally fails to initialise
    // for no particular reason. In our case, we only deal with 44k stereo
    // data so there's no need to reinitialise on each track.
    //m_output->initAudio( sampleRate, channels );

    m_output->startPlayback();
    m_proxyOutput->startPlayback();

    emit trackStarted();
}


void
AudioControllerThread::play( const QString& url, const QString& session )
{
    // The suffix will indicate the filetype (mp3, ogg etc)
    // TODO: not currently used but switch back on when enabling local playback
    //const QString suffix = QFileInfo( track.url().toString() ).suffix().toLower();

    updateUserHttpBufferSize();

    m_input->setSession( session );
    m_input->load( url );
    m_input->startStreaming();

    m_timeElapsedAtLastPause = 0;
    startTimer();

    playLoop();
}


void
AudioControllerThread::playLoop()
{
    forever
    {
        // Check for natural song completion
        if ( m_input->state() == State_Stopped && !m_output->hasData() )
        {
            LOGL( 4, "Natural end detected" );
            setState( State_Stopping );
        }

        // Have we been told to stop?
        if ( state() == State_Stopping )
        {
            LOGL( 4, "Exiting playLoop due to State_Stopping" );
            break;
        }

        // Check http -> transcode step of pipe
        if ( m_transcode->needsData() )
        {
            if ( m_input->hasData() )
            {
                QByteArray data;
                m_input->data( data, kCompressedPacketSize );

                m_proxyOutput->processData( data );
                bool fine = m_transcode->processData( data );

                if ( !fine )
                {
                    LOGL( 2, "MP3 decoding failed, stopping." );
                    setState( State_Stopping );
                    break;
                }
            }
        }

        // Check transcode -> output step of pipe
        if ( m_output->needsData() )
        {
            if ( m_transcode->hasData() )
            {
                QByteArray data;
                m_transcode->data( data, kUncompressedPacketSize );

                if ( !m_proxyOutput || !m_proxyOutput->isActive() )
                    m_output->processData( data );
            }
        }

        emit httpBufferSizeChanged( m_input->bufferSize() );
        emit decodedBufferSizeChanged( m_transcode->bufferSize() );
        emit outputBufferSizeChanged( m_output->bufferSize() );

        // This will process events for the current thread only
        QCoreApplication::processEvents();

        msleep( kPollingInterval );
    }

    endTrack();
}


void
AudioControllerThread::endTrack()
{
    Q_ASSERT( state() == State_Stopping );

    stopTimer();
    m_input->stopStreaming();
    m_output->stopPlayback();
    m_proxyOutput->stopPlayback();

    m_transcode->clearBuffers();
    m_output->clearBuffers();

  #ifdef MONITOR_STREAMING
    emit httpBufferSizeChanged( 0 );
    emit decodedBufferSizeChanged( 0 );
    emit outputBufferSizeChanged( 0 );
  #endif

    int elapsed = ( m_timeElapsedAtLastPause + m_timeSinceLastPause.elapsed() ) / 1000;
    emit trackEnded( elapsed );

    setState( State_Stopped );
}


void
AudioControllerThread::startTimer()
{
    // The only purpose of this timer is to enable sending signals out repeatedly
    m_timer->start();

    // Marks the start of a new time span
    m_timeSinceLastPause.start();
}


void
AudioControllerThread::stopTimer()
{
    // elapsed returns the number of ms since start was called
    m_timeElapsedAtLastPause += m_timeSinceLastPause.elapsed();
    m_timer->stop();
}


void
AudioControllerThread::onTimerTimeout()
{
    emit timeChanged( ( m_timeElapsedAtLastPause + m_timeSinceLastPause.elapsed() ) / 1000 );
}


void
AudioControllerThread::onInputStateChanged( RadioState newState )
{
    if ( state() == State_Stopping )
    {
        // We're in the process of stopping. Don't let the input's
        // state change override this as it might lead to the playLoop
        // not exiting.
    }
    else if ( newState == State_Stopped && m_output->hasData() )
    {
        // We're still streaming, don't change state to stopped until the 
        // output is done streaming. Done in playLoop.

        // TODO: start prebuffering the next track in httpinput
    }
    else if ( newState == State_Stopped )
    {
        // This happens if the input goes to Stopped before we've started
        // streaming. Do nothing as the end detection will be done in playLoop.
    }
    else
    {
        if ( m_automaticBufferHandling &&
             newState == State_Buffering && state() == State_Streaming )
        {
            // We ran out of buffer, increase it
            int newSize = (int)( m_input->bufferCapacity() * 1.5f );
            newSize = qMin( MooseConstants::kHttpBufferMaxSize, newSize );
            m_input->setBufferCapacity( newSize );

            LOGL( 3, "Automatically increased buffer size to: " << newSize );
        }

        setState( newState );
    }
}


void
AudioControllerThread::setState( RadioState newState )
{
    // These are the only valid states
    Q_ASSERT( newState == State_FetchingStream ||
              newState == State_StreamFetched ||
              newState == State_Buffering ||
              newState == State_Streaming ||
              newState == State_Stopping ||
              newState == State_Stopped );

    if ( newState != state() )
    {
        LOGL( 4, "AudioControllerThread state: " << radioState2String( newState ) );

        QMutexLocker locker( &m_mutex );
        m_state = newState;
        locker.unlock();

        emit stateChanged( newState );
    }
}


AudioController::AudioController() :
        m_state( State_Stopped ),
        m_thread( this ),
        m_streamerErrorCounter( 0 ),
        m_retryNextTrack( false )
{
    connect( &m_thread, SIGNAL( stateChanged( RadioState ) ),
             this,      SLOT  ( onThreadStateChanged( RadioState ) ),
             Qt::QueuedConnection );

    connect( &m_thread, SIGNAL( trackStarted() ),
             this,      SLOT  ( onTrackStarted() ),
             Qt::QueuedConnection );

    connect( &m_thread, SIGNAL( trackEnded( int ) ),
             this,      SLOT  ( onTrackEndReached( int ) ),
             Qt::QueuedConnection );

    connect( &m_thread, SIGNAL( error( int, const QString& ) ),
             this,      SLOT  ( onStreamingError( int, const QString& ) ),
             Qt::QueuedConnection );

    // To avoid nasty surprises, let's wait until our worker thread is
    // initialised before we let the GUI thread carry on.
    // We lock m_mutex here and make the thread wait for it to prevent
    // the thread from finishing initialisation before we have had
    // time to enter wait. Otherwise wait could wait forever.
    QMutexLocker locker( &m_mutex );
    m_thread.start( QThread::HighestPriority );

    // Once waiting, the m_mutex will be unlocked
    m_thread.m_initialised.wait( &m_mutex );
    locker.unlock();

    if ( m_thread.m_output != 0 )
    {
        // Safe to copy these values out here
        m_soundSystems = m_thread.m_output->soundSystems();
        m_devices = m_thread.m_output->devices();
    }
}


AudioController::~AudioController()
{
    stop();

    m_thread.exit();
    m_thread.wait( 5000 );
}


bool
AudioController::isPlaying() const
{
    return m_state != State_Stopped;
}


void
AudioController::play()
{
    loadNext();
}


void
AudioController::play( RadioPlaylist& playlist )
{
    m_playlist = &playlist;
    loadNext();

    // We reset the automatically managed buffer size on station change
    AudioControllerThread::ResetBufferSizeEvent* e =
        new AudioControllerThread::ResetBufferSizeEvent();
    QCoreApplication::postEvent( m_thread.eventHandler(), e );
}


void
AudioController::play( const QUrl& url )
{
    m_playlist = 0;

    TrackInfo t;
    t.setPath( url.toString() );
    doPlay( t );
}


void
AudioController::play( const TrackInfo& track )
{
    m_playlist = 0;

    doPlay( track );
}


// All the other play functions hand down to this one.
void
AudioController::doPlay( const TrackInfo& track )
{
    m_retryNextTrack = false;

    if ( !m_pendingTrack.isEmpty() )
    {
        // We will make no attempt at handling a queue of tracks coming in in
        // very quick succession after each other, it leads to really complex
        // code.
        return;
    }

    // Potential difficult cases:
    // 1. The thread has just stopped due to our stopping it but the state change hasn't
    //    propagated up to us yet.
    //      We are skipping or stopping. Pending track will get set and we go to Skipping.
    //      Once onThreadStateChanged is called the Skipping case will get hit and the
    //      pending track will be started.
    // 2. The thread has just stopped naturally but the state change hasn't propagated
    //    up yet.
    //      Our state will be Streaming so we will send a Stop. But the thread has just
    //      returned a state change to Stopped which we haven't received yet. So we will
    //      get two Stopped messages leading to the new track being stopped straight away.
    //      BUG! FIXME!
    // 3. The thread has just started but we're still in Fetching, Fetched or Buffering.
    //      Should be fine, we just send a stop and wait for the response.

    switch ( m_state )
    {
        case State_Stopped:
        {
            // Easy, just let it go ahead
            LOGL( 4, "AudioController requesting play of " << track.toString() );

            m_currentTrackUrl = track.nextPath();

            AudioControllerThread::PlayEvent* e =
                new AudioControllerThread::PlayEvent( m_currentTrackUrl, The::radio().sessionKey() );
            QCoreApplication::postEvent( m_thread.eventHandler(), e );

            TrackInfo previousTrack = m_currentTrack;
            m_currentTrack = track;

            // Ideally, we'd like to tell the listener at the top of this function
            // before we do the actual skip as that would lead to the GUI feeling more
            // responsive. However, if we do that, the trackChanged for the new track
            // will be emitted before the trackEnded for the previous track. By
            // sticking it down here, skipping feels slightly slower but we guarantee
            // that trackEnded always comes before trackChanged.
            emit trackChanged( m_currentTrack, previousTrack );

            setState( State_FetchingStream );
        }
        break;

        case State_FetchingStream:
        case State_StreamFetched:
        case State_Buffering:
        case State_Streaming:
        {
            // We must ask the thread to stop its current track first, the pending track
            // will be resubmitted to this function from onThreadStateChanged.
            m_pendingTrack = track;

            LOGL( 4, "AudioController requesting skip of " << m_currentTrack.toString() );
            AudioControllerThread::StopEvent* e = new AudioControllerThread::StopEvent();
            QCoreApplication::postEvent( m_thread.eventHandler(), e );

            setState( State_Skipping );
        }
        break;

        case State_Skipping:
        {
            Q_ASSERT( !"I wonder if we ever get here seeing as there's a check for empty pending track above" );

            // We have just sent a stop message to the thread without getting
            // a response back yet. Just set the pending track.
            m_pendingTrack = track;
        }
        break;

        case State_Stopping:
        {
            // We have just sent a stop message to the thread without getting
            // a response back yet. Just set the pending track and go to skipping
            // so that the new pending track will kick off after the thread stopped.
            m_pendingTrack = track;
            setState( State_Skipping );
        }
        break;

        default:
            Q_ASSERT( !"Illegal state" );
    }
}


void
AudioController::loadNext()
{
    if ( m_playlist == 0 )
    {
        stop();
    }
    else if ( m_playlist->hasMore() )
    {
        TrackInfo track = m_playlist->nextTrack();
        doPlay( track );
    }
    else
    {
        LOGL( 3, "AudioController ran out of playlist" );

        emit error( Radio_OutOfPlaylist,
            tr( "Sorry, there is no more content available for this station. Please choose a different one." ) );

        // Don't stop until after emitting the error as then the stopping state
        // won't be propagated up to the GUI just before the radio goes to
        // fetching stream state again.
        stop();
    }
}


void
AudioController::stop()
{
    LOGL( 4, "AudioController requesting stop of " << m_currentTrack.toString() );

    AudioControllerThread::StopEvent* e = new AudioControllerThread::StopEvent();
    QCoreApplication::postEvent( m_thread.eventHandler(), e );

    setState( State_Stopping );
}


void
AudioController::setVolume( int vol )
{
    AudioControllerThread::VolumeEvent* e = new AudioControllerThread::VolumeEvent( vol );
    QCoreApplication::postEvent( m_thread.eventHandler(), e );
}


void
AudioController::onTrackStarted()
{
    emit trackStarted( m_currentTrack );
}


void
AudioController::onTrackEndReached( int atPosition )
{
    LOGL( 4, "AudioController::onTrackEndReached" );

    emit trackEnded( m_currentTrack, atPosition );
}


void
AudioController::onThreadStateChanged( RadioState newState )
{
    switch ( m_state )
    {
        // When we're in Skipping or Stopping, all we're interested in is
        // the thread emitting Stopped
        case State_Skipping:
        {
            if ( newState == State_Stopped )
            {
                LOGL( Logger::Debug, "Calling doPlay due to pending track" );

                setState( State_Stopped );

                // We started a new track before the previous one had finished
                TrackInfo temp = m_pendingTrack;
                m_pendingTrack = TrackInfo();
                doPlay( temp );
            }
        }
        break;

        case State_Stopping:
        {
            if ( newState == State_Stopped )
            {
                // We asked thread to stop
                m_currentTrackUrl.clear();
                m_currentTrack = TrackInfo();
                setState( State_Stopped );
            }
        }
        break;

        case State_Stopped:
        break;

        default:
        {
            switch ( newState )
            {
                case State_Stopping:
                {
                    // Don't let this override
                }
                break;

                case State_Stopped:
                {
                    setState( State_Stopped );

                    if ( m_retryNextTrack )
                    {
                        if ( m_currentTrack.hasMorePaths() )
                        {
                            LOGL( Logger::Debug, "Got TrackNotFound and track has more "
                                "locations, trying next one." );
                            doPlay( m_currentTrack );
                        }
                        else
                        {
                            LOGL( Logger::Debug, "Got TrackNotFound and track has no more "
                                "locations, calling loadNext." );
                            loadNext();
                        }
                    }
                    else
                    {
                        LOGL( Logger::Debug, "Calling loadNext, thread emitted Stopped." );
                        loadNext();
                    }
                }
                break;

                default:
                {
                    setState( newState );
                }
            }
        }
    }
}


void
AudioController::onStreamingError( int error,
                                   const QString& reason )
{
    // This slot receives errors from not just the HttpInput, but also the
    // ACThread and the output.

    RadioError err = static_cast<RadioError>( error );

    // These are the errors from the HttpInput on which we want to retry to
    // avoid disrupting radio stations if we can.
    if ( error == Radio_InvalidUrl ||
         error == Radio_InvalidAuth ||
         error == Radio_TrackNotFound ||
         error == Radio_UnknownError ||
         error == Radio_ConnectionRefused )
    {
        if ( ++m_streamerErrorCounter < k_streamerErrorLimit )
        {
            LOGL( Logger::Debug, "Got retryable radio error " << error << " from streamer, setting m_retryNextTrack" );

            // I introduced this in order to fix a bug where the http input
            // emitted a TrackNotFound error and then a state change Stopped
            // error. Preciously I called doPlay/loadNext here but we can't do
            // that as the state change will then come along and do the same
            // thing. Only one slot in this class should be allowed to take
            // controlling actions. So we just note that we got a track not found
            // error here and take appropriate action once we get the state change.
            m_retryNextTrack = true;
            return;
        }
    }

    // If it wasn't a retryable error, tell the world
    stop();
    emit this->error( err, reason );
    m_streamerErrorCounter = 0;
}


void
AudioController::setState( RadioState newState )
{
    // These are the only valid states
    Q_ASSERT( newState == State_FetchingStream ||
              newState == State_StreamFetched ||
              newState == State_Buffering ||
              newState == State_Streaming ||
              newState == State_Skipping ||
              newState == State_Stopping ||
              newState == State_Stopped );

    if ( newState != m_state )
    {
        LOGL( 4, "AudioController state: " << radioState2String( newState ) );

        m_state = newState;

        emit stateChanged( newState );
    }

    if ( m_state == State_Streaming )
    {
        m_streamerErrorCounter = 0;
    }
}

#endif
