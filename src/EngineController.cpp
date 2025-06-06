/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2006,2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (C) 2025 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#define DEBUG_PREFIX "EngineController"

#include "EngineController.h"

#include "MainWindow.h"
#include "EngineGstPipeline.h"
#include "amarokconfig.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/capabilities/MultiPlayableCapability.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "playback/DelayedDoers.h"
#include "playback/Fadeouter.h"
#include "playback/PowerManager.h"
#include "playlist/PlaylistActions.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QTimer>
#include <QtMath>

#include <KLocalizedString>

#include <thread>


namespace The {
    EngineController* engineController() { return EngineController::instance(); }
}

EngineController *
EngineController::instance()
{
    return Amarok::Components::engineController();
}

EngineController::EngineController()
    : m_pipeline( nullptr )
    , m_seekablePipeline( false )
    , m_fader( false )
    , m_boundedPlayback( nullptr )
    , m_multiPlayback( nullptr )
    , m_multiSource( nullptr )
    , m_playWhenFetched( true )
    , m_volume( 0 )
    , m_currentAudioCdTrack( 0 )
    , m_pauseTimer( new QTimer( this ) )
    , m_lastStreamStampPosition( -1 )
    , m_ignoreVolumeChangeAction ( false )
    , m_ignoreVolumeChangeObserve ( false )
    , m_tickInterval( 0 )
    , m_lastTickPosition( -1 )
    , m_lastTickCount( 0 )
{
    DEBUG_BLOCK
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    connect( this, &EngineController::fillInSupportedMimeTypes, this, &EngineController::slotFillInSupportedMimeTypes );
    connect( this, &EngineController::trackFinishedPlaying, this, &EngineController::slotTrackFinishedPlaying );

    new PowerManager( this ); // deals with inhibiting suspend etc.

    m_pauseTimer->setSingleShot( true );
    connect( m_pauseTimer, &QTimer::timeout, this, &EngineController::slotPause );
    m_equalizerController = new EqualizerController( this );
}

EngineController::~EngineController()
{
    DEBUG_BLOCK //we like to know when singletons are destroyed

    // don't do any of the after-processing that normally happens when
    // the media is stopped - that's what endSession() is for
    if( m_pipeline )
    {
        m_pipeline->blockSignals(true);
        m_pipeline->stop();
    }

    delete m_boundedPlayback;
    m_boundedPlayback = nullptr;
    delete m_multiPlayback; // need to get a new instance of multi if played again
    m_multiPlayback = nullptr;

    delete m_pipeline;
    m_pipeline = nullptr;
}

void
EngineController::initializeBackend()
{
    DEBUG_BLOCK

    PERF_LOG( "EngineController: loading gst objects" )
    gst_init (nullptr, nullptr);

    /*
    m_equalizerController->initialize( m_path );*/ //TODO equalizer

    //debug() << "Tick Interval (actual): " << m_tickInterval;
    PERF_LOG( "EngineController: loaded gst objects" )
    if( !m_pipeline )
    {
        m_pipeline = new EngineGstPipeline;

        connect( m_pipeline, &EngineGstPipeline::finished, this, &EngineController::slotFinished );
        connect( m_pipeline, &EngineGstPipeline::aboutToFinish, this, &EngineController::slotAboutToFinish, Qt::DirectConnection );
        connect( m_pipeline, &EngineGstPipeline::metaDataChanged, this, &EngineController::slotMetaDataChanged );
        connect( m_pipeline, &EngineGstPipeline::stateChanged, this, &EngineController::slotStateChanged );
        connect( m_pipeline, &EngineGstPipeline::tick, this, &EngineController::slotTick );
        connect( m_pipeline, &EngineGstPipeline::durationChanged, this, &EngineController::slotTrackLengthChanged );
        connect( m_pipeline, &EngineGstPipeline::currentSourceChanged, this, &EngineController::slotNewTrackPlaying );
        connect( m_pipeline, &EngineGstPipeline::seekableChanged, this, &EngineController::slotSeekableChanged );
        connect( m_pipeline, &EngineGstPipeline::volumeChanged, this, &EngineController::slotVolumeChanged );
        connect( m_pipeline, &EngineGstPipeline::mutedChanged, this, &EngineController::slotMutedChanged );

        m_seekablePipeline = m_pipeline->isSeekable();
        //connect( m_audioDataOutput.data(), &AudioDataOutput::dataReady, this, &EngineController::audioDataReady ); //TODO analyzer
    }

    if( m_currentTrack )
    {
        unsubscribeFrom( m_currentTrack );
        m_currentTrack.clear();
    }
    if( m_currentAlbum )
    {
        unsubscribeFrom( m_currentAlbum );
        m_currentAlbum.clear();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

QStringList
EngineController::supportedMimeTypes()
{
    // this ensures that slotFillInSupportedMimeTypes() is called in the main thread. It
    // will be called directly if we are called in the main thread (so that no deadlock
    // can occur) and indirectly if we are called in non-main thread.
    Q_EMIT fillInSupportedMimeTypes();

    // ensure slotFillInSupportedMimeTypes() called above has already finished:
    m_supportedMimeTypesSemaphore.acquire();
    return m_supportedMimeTypes;
}

void
EngineController::slotFillInSupportedMimeTypes()
{
    // we assume non-empty == already filled in
    if( !m_supportedMimeTypes.isEmpty() )
    {
        // unblock waiting for the semaphore in supportedMimeTypes():
        m_supportedMimeTypesSemaphore.release();
        return;
    }

    QRegularExpression avFilter( QStringLiteral("^(audio|video)/"), QRegularExpression::CaseInsensitiveOption);
    m_supportedMimeTypes = m_pipeline->availableMimeTypes().filter( avFilter );

    // Add whitelist hacks
    // MP4 Audio Books have a different extension that KFileItem/Phonon don't grok
    if( !m_supportedMimeTypes.contains( QStringLiteral("audio/x-m4b") ) )
        m_supportedMimeTypes << QStringLiteral("audio/x-m4b");

    // technically, "audio/flac" is not a valid mimetype (not on IANA list), but some things expect it
    if( m_supportedMimeTypes.contains( QStringLiteral("audio/x-flac") ) && !m_supportedMimeTypes.contains( QStringLiteral("audio/flac") ) )
        m_supportedMimeTypes << QStringLiteral("audio/flac");

    // technically, "audio/mp4" is the official mime type, but sometimes Phonon returns audio/x-m4a
    if( m_supportedMimeTypes.contains( QStringLiteral("audio/x-m4a") ) && !m_supportedMimeTypes.contains( QStringLiteral("audio/mp4") ) )
        m_supportedMimeTypes << QStringLiteral("audio/mp4");

    // unblock waiting for the semaphore in supportedMimeTypes(). We can over-shoot
    // resource number so that next call to supportedMimeTypes won't have to
    // wait for main loop; this is however just an optimization and we could have safely
    // released just one resource. Note that this code-path is reached only once, so
    // overflow cannot happen.
    m_supportedMimeTypesSemaphore.release( 100000 );
}

void
EngineController::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if( AmarokConfig::resumePlayback() )
    {
        const QUrl url = QUrl::fromUserInput(AmarokConfig::resumeTrack());
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

        // Only give a resume time for local files, because resuming remote protocols can have weird side effects.
        // See: https://bugs.kde.org/show_bug.cgi?id=172897
        if( url.isLocalFile() )
            play( track, AmarokConfig::resumeTime(), AmarokConfig::resumePaused() );
        else
            play( track, 0, AmarokConfig::resumePaused() );
    }
}

void
EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() && m_currentTrack )
    {
        Q_EMIT stopped( trackPositionMs(), m_currentTrack->length() );
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        Q_EMIT trackChanged( Meta::TrackPtr( nullptr ) );
    }
    Q_EMIT sessionEnded( AmarokConfig::resumePlayback() && m_currentTrack );
}

EqualizerController*
EngineController::equalizerController() const
{
    return m_equalizerController;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::play() //SLOT
{
    DEBUG_BLOCK

    if( isPlaying() )
        return;

    if( isPaused() )
    {
        if( m_currentTrack && m_currentTrack->type() == QStringLiteral("stream") )
        {
            debug() << "This is a stream that cannot be resumed after pausing. Restarting instead.";
            play( m_currentTrack );
            return;
        }
        else
        {
            m_pauseTimer->stop();
//            if( supportsFadeout() )
  //              m_fader->setVolume( 1.0 ); //TODO
            m_pipeline->play();
            Q_EMIT trackPlaying( m_currentTrack );
            return;
        }
    }

    The::playlistActions()->play();
}

void
EngineController::play( Meta::TrackPtr track, uint offset, bool startPaused )
{
    DEBUG_BLOCK

    if( !track ) // Guard
        return;

    // clear the current track without sending playbackEnded or trackChangeNotify yet
    stop( /* forceInstant */ true, /* playingWillContinue */ true );

    // we grant exclusive access to setting new m_currentTrack to newTrackPlaying()
    m_nextTrack = track;
    debug() << "play: bounded is "<<m_boundedPlayback<<"current"<<track->name();
    m_boundedPlayback = track->create<Capabilities::BoundedPlaybackCapability>();
    m_multiPlayback = track->create<Capabilities::MultiPlayableCapability>();

    track->prepareToPlay();
    m_nextUrl = track->playableUrl();

    if( m_multiPlayback )
    {
        connect( m_multiPlayback, &Capabilities::MultiPlayableCapability::playableUrlFetched,
                 this, &EngineController::slotPlayableUrlFetched );
        m_multiPlayback->fetchFirst();
    }
    else if( m_boundedPlayback )
    {
        debug() << "Starting bounded playback of url " << track->playableUrl() << " at position " << m_boundedPlayback->startPosition();
        playUrl( track->playableUrl(), m_boundedPlayback->startPosition(), startPaused );
    }
    else
    {
        debug() << "Just a normal, boring track... :-P";
        playUrl( track->playableUrl(), offset, startPaused );
    }
}

void
EngineController::replay() // slot
{
    DEBUG_BLOCK

    seekTo( 0 );
    Q_EMIT trackPositionChanged( 0, true );
}

void
EngineController::playUrl( const QUrl &url, uint offset, bool startPaused )
{
    DEBUG_BLOCK

    m_pipeline->stop();

    debug() << "URL: " << url << url.url();
    debug() << "Offset: " << offset;

    m_currentAudioCdTrack = 0;
    if( url.scheme() == QStringLiteral("audiocd") )
    {
        QStringList pathItems = url.path().split( QLatin1Char('/'), Qt::KeepEmptyParts );
        if( pathItems.count() != 3 )
        {
            error() << __PRETTY_FUNCTION__ << url.url() << "is not in expected format";
            return;
        }
        bool ok = false;
        int trackNumber = pathItems.at( 2 ).toInt( &ok );
        if( !ok || trackNumber <= 0 )
        {
            error() << __PRETTY_FUNCTION__ << "failed to get positive track number from" << url.url();
            return;
        }
        QString device = QUrlQuery(url).queryItemValue( QStringLiteral("device") );

        m_pipeline->setSource( url );
        m_currentAudioCdTrack = trackNumber;
    }
    else
    {
        // keep in sync with setNextTrack(), slotPlayableUrlFetched()
        m_pipeline->setSource( url );
    }

    m_pipeline->clearPlaybackQueue();

    if( m_currentAudioCdTrack )
    {
        m_pipeline->play();
    }
    else if( offset )
    {
        // the comment below originates from phonon times, but also applies with gstreamer:
        // call to play() is asynchronous and ->seek() can be only called on playing,
        // buffering or paused media. Calling play() would lead to audible glitches,
        // so call pause() that doesn't suffer from such problem.
        connect( m_pipeline, &EngineGstPipeline::internalStateChanged, this,
                [offset, m_pipeline = m_pipeline] ( GstState o, GstState n )
                {
                    if( n == GST_STATE_PAUSED )
                    {
                        m_pipeline->seekToMSec( offset );
                        m_pipeline->play();
                    }
                    else
                        debug() << "Unexpected state change with offset play: " << o << "to" << n;
                }, Qt::SingleShotConnection );
        m_pipeline->pause();
    }
    else
    {
        if( startPaused )
        {
            m_pipeline->pause();
        }
        else
        {
            m_pauseTimer->stop();
       //     if( supportsFadeout() )
// TODO                m_fader->setVolume( 1.0 );
            updateReplayGainSetting( (bool) m_nextTrack ); // read gain from next track if available
            m_pipeline->play();
        }
    }
}

void
EngineController::pause() //SLOT
{
    if( supportsFadeout() && AmarokConfig::fadeoutOnPause() )
    {
//        m_fader->fadeOut( AmarokConfig::fadeoutLength() ); //TODO
        m_pauseTimer->start( AmarokConfig::fadeoutLength() + 500 );
        return;
    }

    slotPause();
}

void
EngineController::slotPause()
{
    if( supportsFadeout() && AmarokConfig::fadeoutOnPause() )
    {
        // Reset VolumeFaderEffect to full volume
// TODO        m_fader->setVolume( 1.0 );

        // Wait a bit before pausing the pipeline. Necessary for the new fader setting to take effect.
// TODO        QTimer::singleShot( 1000, m_media.data(), &Phonon::MediaObject::pause );
    }
    else
    {
        m_pipeline->pause();
    }

    Q_EMIT paused();
}

void
EngineController::stop( bool forceInstant, bool playingWillContinue ) //SLOT
{
    DEBUG_BLOCK

    /* Only do fade-out when all conditions are met:
     * a) instant stop is not requested
     * b) we aren't already in a fadeout
     * c) we are currently playing (not paused etc.)
     * d) Amarok is configured to fadeout at all
     * e) configured fadeout length is positive
     * f) Phonon fader to do it is actually available
     */
    bool doFadeOut = !forceInstant
                  && !m_fadeouter
                  && m_pipeline->state() == GST_STATE_PLAYING
                  && AmarokConfig::fadeoutOnStop()
                  && AmarokConfig::fadeoutLength() > 0
                  && m_fader;

    // let Amarok know that the previous track is no longer playing; if we will fade-out
    // ::stop() is called after the fade by Fadeouter.
    if( m_currentTrack && !doFadeOut )
    {
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        qint64 pos = trackPositionMs();
        // Phonon-vlc media progress reporting gets stuck when preparing a gapless track change,
        // with some formats, it seems, causing this function to get called after m_media starts
        // reporting a current time of 0. This function is stop, so if engine claims that position
        // is 0, be very suspicious and try to use own value if that seems more sensible. BUG 337849
        if( pos == 0 )
            pos = m_lastTickPosition;
        // updateStreamLength() intentionally not here, we're probably in the middle of a track
        const qint64 length = trackLength();
        Q_EMIT trackFinishedPlaying( m_currentTrack, pos / qMax<double>( length, pos ) );

        m_currentTrack = nullptr;
        m_currentAlbum = nullptr;
        if( !playingWillContinue )
        {
            Q_EMIT stopped( pos, length );
            Q_EMIT trackChanged( m_currentTrack );
        }
    }

    {
        QMutexLocker locker( &m_mutex );
        delete m_boundedPlayback;
        m_boundedPlayback = nullptr;
        delete m_multiPlayback; // need to get a new instance of multi if played again
        m_multiPlayback = nullptr;
        m_multiSource.reset();

        m_nextTrack.clear();
        m_nextUrl.clear();
        m_pipeline->clearPlaybackQueue();
    }

    if( doFadeOut )
    {
       // TODO m_fadeouter = new Fadeouter( m_media, m_fader, AmarokConfig::fadeoutLength() );
        // even though we don't pass forceInstant, doFadeOut will be false because
        // m_fadeouter will be still valid
        /*connect( m_fadeouter.data(), &Fadeouter::fadeoutFinished,
                 this, &EngineController::regularStop ); */ //TODO
    }
    else
    {
        // empty MediaSource() seems to cause Phonon-VLC to emit a volume of 0, BR 442319, so ignore it
        m_ignoreVolumeChangeObserve = true;
        m_pipeline->stop();
        m_pipeline->setSource( QUrl() );
        QTimer::singleShot( 0, [this]() { m_ignoreVolumeChangeObserve = false; } );
    }
}

void
EngineController::regularStop()
{
    stop( false, false );
}

bool
EngineController::isPaused() const
{
    if( !m_pipeline )
        return false;
    return m_pipeline->state() == GST_STATE_PAUSED;
}

bool
EngineController::isPlaying() const
{
    return !isPaused() && !isStopped();
}

bool
EngineController::isStopped() const
{
    if( !m_pipeline )
        return true;
    return m_pipeline->state() != GST_STATE_PLAYING &&
        m_pipeline->state() != GST_STATE_VOID_PENDING;
}

void
EngineController::playPause() //SLOT
{
    DEBUG_BLOCK
    debug() << "PlayPause: Pipeline state" << m_pipeline->state();

    if( isPlaying() )
        pause();
    else
        play();
}

void
EngineController::seekTo( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( isSeekable() )
    {

        debug() << "seek to: " << ms;
        int seekTo;

        if( m_boundedPlayback )
        {
            seekTo = m_boundedPlayback->startPosition() + ms;
            if( seekTo < m_boundedPlayback->startPosition() )
                seekTo = m_boundedPlayback->startPosition();
            else if( seekTo > m_boundedPlayback->startPosition() + trackLength() )
                seekTo = m_boundedPlayback->startPosition() + trackLength();
        }
        else
            seekTo = ms;

        m_pipeline->seekToMSec( static_cast<qint64>( seekTo ) );
        Q_EMIT trackPositionChanged( seekTo, true ); /* User seek */
    }
    else
        debug() << "Stream is not seekable.";
}


void
EngineController::seekBy( int ms ) //SLOT
{
    qint64 newPos = trackPositionMs() + ms;
    seekTo( newPos <= 0 ? 0 : newPos );
}

int
EngineController::increaseVolume( int ticks ) //SLOT
{
    return setVolume( volume() + ticks );
}

int
EngineController::decreaseVolume( int ticks ) //SLOT
{
    return setVolume( volume() - ticks );
}

int
EngineController::regularIncreaseVolume() //SLOT
{
    return increaseVolume();
}

int
EngineController::regularDecreaseVolume() //SLOT
{
    return decreaseVolume();
}

int
EngineController::setVolume( int percent ) //SLOT
{
    percent = qBound<qreal>( 0, percent, 100 );
    m_volume = percent;

    const qreal volume =  percent / 100.0;
    if ( !m_ignoreVolumeChangeAction )
    {
        m_ignoreVolumeChangeObserve = true;
        m_pipeline->setVolume( volume );

        AmarokConfig::setMasterVolume( percent );
        Q_EMIT volumeChanged( percent );
    }
    m_ignoreVolumeChangeAction = false;

    return percent;
}

int
EngineController::volume() const
{
    return m_volume;
}

bool
EngineController::isMuted() const
{
    return m_pipeline->isMuted();
}

void
EngineController::setMuted( bool mute ) //SLOT
{
    m_pipeline->setMuted( mute ); // toggle mute
    if( !isMuted() )
        setVolume( m_volume );

    AmarokConfig::setMuteState( mute );
    Q_EMIT muteStateChanged( mute );
}

void
EngineController::toggleMute() //SLOT
{
    setMuted( !isMuted() );
}

Meta::TrackPtr
EngineController::currentTrack() const
{
    return m_currentTrack;
}

qint64
EngineController::trackLength() const
{
    //When starting a last.fm stream, Phonon still shows the old track's length--trust
    //Meta::Track over Phonon
    if( m_currentTrack && m_currentTrack->length() > 0 )
        return m_currentTrack->length();
    else if( m_pipeline )
        return m_pipeline->totalDuration();
    else
        return -1;
}

void
EngineController::setNextTrack( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if( !track )
        return;

    track->prepareToPlay();
    QUrl url = track->playableUrl();
    if( url.isEmpty() )
        return;

    QMutexLocker locker( &m_mutex );
    if( isPlaying() )
    {
        m_pipeline->clearPlaybackQueue();
        // keep in sync with playUrl(), slotPlayableUrlFetched()
        m_pipeline->enqueuePlayback( url );
        m_nextTrack = track;
        m_nextUrl = url;
    }
    else
        play( track );
}

bool
EngineController::isSeekable() const
{
    if( m_pipeline )
        return m_seekablePipeline;
    return false;
}

int
EngineController::trackPosition() const
{
    return trackPositionMs() / 1000;
}

qint64
EngineController::trackPositionMs() const
{
    return m_pipeline->position();
}

bool
EngineController::supportsFadeout() const
{
    return m_fader;
}

bool EngineController::supportsGainAdjustments() const
{
    return m_pipeline && m_pipeline->isReplayGainReady();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::slotTick( qint64 position )
{
    if( m_boundedPlayback )
    {
        qint64 newPosition = position;
        Q_EMIT trackPositionChanged(
                    static_cast<long>( position - m_boundedPlayback->startPosition() ),
                    false
                );

        // Calculate a better position.  Sometimes the position doesn't update
        // with a good resolution (for example, 1 sec for TrueAudio files in the
        // Xine-1.1.18 backend).  This tick function, in those cases, just gets
        // called multiple times with the same position.  We count how many
        // times this has been called prior, and adjust for it.
        if( position == m_lastTickPosition )
            newPosition += ++m_lastTickCount * m_tickInterval;
        else
            m_lastTickCount = 0;

        m_lastTickPosition = position;

        //don't go beyond the stop point
        if( newPosition >= m_boundedPlayback->endPosition() )
        {
            slotAboutToFinish();
        }
    }
    else
    {
        m_lastTickPosition = position;
        Q_EMIT trackPositionChanged( static_cast<long>( position ), false );
    }
}

void
EngineController::slotAboutToFinish()
{
    DEBUG_BLOCK

    if( m_fadeouter )
    {
        debug() << "slotAboutToFinish(): a fadeout is in progress, don't queue new track";
        return;
    }

    if( m_multiPlayback )
    {
        DEBUG_LINE_INFO
        m_mutex.lock();
        m_playWhenFetched = false;
        m_mutex.unlock();
        m_multiPlayback->fetchNext();
        debug() << "The queue has: " << m_pipeline->playbackQueueLength() << " tracks in it";
    }
    else if( m_multiSource )
    {
        debug() << "source finished, lets get the next one";
        QUrl nextSource = m_multiSource->nextUrl();

        if( !nextSource.isEmpty() )
        { //more sources
            m_mutex.lock();
            m_playWhenFetched = false;
            m_mutex.unlock();
            debug() << "playing next source: " << nextSource;
            slotPlayableUrlFetched( nextSource );
        }
        else if( m_pipeline->isPlaybackQueueEmpty() )
        {
            debug() << "no more sources, skip to next track";
            m_multiSource.reset(); // don't confuse slotFinished
            The::playlistActions()->requestNextTrack();
        }
    }
    else if( m_boundedPlayback )
    {
        debug() << "finished a track that consists of part of another track, go to next track even if this url is technically not done yet";

        //stop this track, now, as the source track might go on and on, and
        //there might not be any more tracks in the playlist...
        stop( true );
        The::playlistActions()->requestNextTrack();
    }
    else if( m_pipeline->isPlaybackQueueEmpty() )
        The::playlistActions()->requestNextTrack();
}

void
EngineController::slotFinished()
{
    DEBUG_BLOCK

    // paranoia checking, m_currentTrack shouldn't really be null
    if( m_currentTrack )
    {
        debug() << "Track finished completely, updating statistics";
        unsubscribeFrom( m_currentTrack ); // don't bother with trackMetadataChanged()
        stampStreamTrackLength(); // update track length in stream for accurate scrobbling
        Q_EMIT trackFinishedPlaying( m_currentTrack, 1.0 );
        subscribeTo( m_currentTrack );
    }

    if( !m_multiPlayback && !m_multiSource )
    {
        // again. at this point the track is finished so it's trackPositionMs is 0
        if( !m_nextTrack && m_nextUrl.isEmpty() )
            Q_EMIT stopped( m_currentTrack ? m_currentTrack->length() : 0,
                          m_currentTrack ? m_currentTrack->length() : 0 );
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        m_currentTrack = nullptr;
        m_currentAlbum = nullptr;
        if( !m_nextTrack && m_nextUrl.isEmpty() ) // we will the trackChanged signal later
            Q_EMIT trackChanged( Meta::TrackPtr() );
        m_pipeline->setSource( QUrl() );
    }

    m_mutex.lock(); // in case setNextTrack is being handled right now.

    // Non-local urls are not enqueued so we must play them explicitly.
    if( m_nextTrack )
    {
        DEBUG_LINE_INFO
        play( m_nextTrack );
    }
    else if( !m_nextUrl.isEmpty() )
    {
        DEBUG_LINE_INFO
        playUrl( m_nextUrl, 0 );
    }
    else
    {
        DEBUG_LINE_INFO
        // possibly we are waiting for a fetch
        m_playWhenFetched = true;
    }

    m_mutex.unlock();
}

void
EngineController::slotNewTrackPlaying( const QUrl &source )
{
    DEBUG_BLOCK

    if( source.isEmpty() )
    {
        debug() << "Empty source (engine stop)";
        return;
    }

    if( m_currentTrack )
    {
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
    }
    // only update stats if we are called for something new, some phonon back-ends (at
    // least phonon-gstreamer-4.6.1) call slotNewTrackPlaying twice with the same source
    if( m_currentTrack && ( m_nextTrack || !m_nextUrl.isEmpty() ) )
    {
        debug() << "Previous track finished completely, updating statistics";
        stampStreamTrackLength(); // update track length in stream for accurate scrobbling
        Q_EMIT trackFinishedPlaying( m_currentTrack, 1.0 );

        if( m_multiSource )
            // advance source of a multi-source track
            m_multiSource->setSource( m_multiSource->current() + 1 );
    }
    m_nextUrl.clear();

    if( m_nextTrack )
    {
        // already unsubscribed
        m_currentTrack = m_nextTrack;
        m_nextTrack.clear();

        m_multiSource.reset( m_currentTrack->create<Capabilities::MultiSourceCapability>() );
        if( m_multiSource )
        {
            debug() << "Got a MultiSource Track with" <<  m_multiSource->sources().count() << "sources";
            connect( m_multiSource.data(), &Capabilities::MultiSourceCapability::urlChanged,
                     this, &EngineController::slotPlayableUrlFetched );
        }
    }

    updateReplayGainSetting( false );

    bool useTrackWithinStreamDetection = false;
    if( m_currentTrack )
    {
        subscribeTo( m_currentTrack );
        Meta::AlbumPtr m_currentAlbum = m_currentTrack->album();
        if( m_currentAlbum )
            subscribeTo( m_currentAlbum );
        /** We only use detect-tracks-in-stream for tracks that have stream type
         * (exactly, we purposely exclude stream/lastfm) *and* that don't have length
         * already filled in. Bug 311852 */
        if( m_currentTrack->type() == QStringLiteral("stream") && m_currentTrack->length() == 0 )
            useTrackWithinStreamDetection = true;
    }

    m_lastStreamStampPosition = useTrackWithinStreamDetection ? 0 : -1;
    Q_EMIT trackChanged( m_currentTrack );
    Q_EMIT trackPlaying( m_currentTrack );
}

void
EngineController::slotStateChanged( int oldState, int newState ) //SLOT
{
    debug() << "slotStateChanged from" << oldState << "to" << newState;

    static const int maxErrors = 5;
    static int errorCount = 0;

    // Sanity checks:
    if( newState == oldState )
        return;

    if( newState == 0 || newState == 1 ) //void pending or null  // If media is borked, skip to next track
    {
        Q_EMIT trackError( m_currentTrack );

//        warning() << "Phonon failed to play this URL. Error: " << m_media->errorString();
//        warning() << "Forcing audio backend engine reinitialization.";

        /* In case of error Phonon MediaObject automatically switches to KioMediaSource,
           which cause problems: runs StopAfterCurrentTrack mode, force PlayPause button to
           reply the track (can't be paused). So we should reinitiate Phonon after each Error.
        */
//        initializeBackend();

        errorCount++;
        if ( errorCount >= maxErrors )
        {
            // reset error count
            errorCount = 0;

            Amarok::Logger::longMessage(
                            i18n( "Too many errors encountered in playlist. Playback stopped." ),
                            Amarok::Logger::Warning
                        );
            error() << "Stopping playlist.";
        }
        else
            // and start the next song, even if the current failed to start playing
            The::playlistActions()->requestUserNextTrack();

    }
    else if( newState == 4 ) //playing
    {
        errorCount = 0;
        Q_EMIT playbackStateChanged();
    }
    else if( newState == 2 || //stopped
             newState == 3 ) //paused
    {
        Q_EMIT playbackStateChanged();
    }
}

void
EngineController::slotPlayableUrlFetched( const QUrl &url )
{
    DEBUG_BLOCK
    debug() << "Fetched url: " << url;
    if( url.isEmpty() )
    {
        DEBUG_LINE_INFO
        The::playlistActions()->requestNextTrack();
        return;
    }

    if( !m_playWhenFetched )
    {
        DEBUG_LINE_INFO
        m_mutex.lock();
        m_pipeline->clearPlaybackQueue();
        // keep synced with setNextTrack(), playUrl()
        m_pipeline->enqueuePlayback( url );
        m_nextTrack.clear();
        m_nextUrl = url;
        debug() << "The next url we're playing is: " << m_nextUrl;
        // reset this flag each time
        m_playWhenFetched = true;
        m_mutex.unlock();
    }
    else
    {
        DEBUG_LINE_INFO
        m_mutex.lock();
        playUrl( url, 0 );
        m_mutex.unlock();
    }
}

void
EngineController::slotTrackLengthChanged( qint64 milliseconds )
{
    debug() << "slotTrackLengthChanged(" << milliseconds << ")";
    Q_EMIT trackLengthChanged( ( !m_multiPlayback || !m_boundedPlayback )
                             ? trackLength() : milliseconds );
}

void
EngineController::slotMetaDataChanged()
{
    QVariantMap meta;
    meta.insert( Meta::Field::URL, m_pipeline->currentSource() );
    for (const auto &key : m_pipeline->metaData().keys()) {
        const QStringList values = m_pipeline->metaData().values( key );
        if( values.isEmpty() )
            continue;
        else if( key == QLatin1String("ARTIST") )
            meta.insert( Meta::Field::ARTIST, values.first() );
        else if( key == QLatin1String("ALBUM") )
            meta.insert( Meta::Field::ALBUM, values.first() );
        else if( key == QLatin1String("TITLE") )
            meta.insert( Meta::Field::TITLE, values.first() );
        else if( key == QLatin1String("GENRE") )
            meta.insert( Meta::Field::GENRE, values.first() );
        else if( key == QLatin1String("TRACKNUMBER") )
            meta.insert( Meta::Field::TRACKNUMBER, values.first() );
        else if( key == QLatin1String("DESCRIPTION") )
            meta.insert( Meta::Field::COMMENT, values.first() );
    }

    // note: don't rely on m_currentTrack here. At least some Phonon backends first Q_EMIT
    // totalTimeChanged(), then metaDataChanged() and only then currentSourceChanged()
    // which currently sets correct m_currentTrack.
    if( isInRecentMetaDataHistory( meta ) )
    {
        // slotMetaDataChanged() triggered by phonon, but we've already seen
        // exactly the same metadata recently. Ignoring for now.
        return;
    }

    // following is an implementation of song end (and length) within a stream detection.
    // This normally fires minutes after the track has started playing so m_currentTrack
    // should be accurate
    if( m_currentTrack && m_lastStreamStampPosition >= 0 )
    {
        stampStreamTrackLength();
        Q_EMIT trackFinishedPlaying( m_currentTrack, 1.0 );

        // update track length to 0 because length emitted by stampStreamTrackLength()
        // is for the previous song
        meta.insert( Meta::Field::LENGTH, 0 );
    }

    debug() << "slotMetaDataChanged(): new meta-data:" << meta;
    Q_EMIT currentMetadataChanged( meta );
}

void
EngineController::slotSeekableChanged( bool seekable )
{
    m_seekablePipeline = seekable;
    Q_EMIT seekableChanged( seekable );
}

void
EngineController::slotTitleChanged( int titleNumber )
{
    DEBUG_BLOCK
    if ( titleNumber != m_currentAudioCdTrack )
    {
        The::playlistActions()->requestNextTrack();
        slotFinished();
    }
}

void EngineController::slotVolumeChanged( qreal newVolume )
{
    // phonon-vlc tends to change volume to nan when nothing is playing or track is changing
    // this nan check should prevent unnecessary volume change notifications
    if( newVolume != newVolume )
        return;
    int percent = qBound<qreal>( 0, qRound(newVolume * 100), 100 );

    if ( !m_ignoreVolumeChangeObserve && m_volume != percent )
    {
        m_ignoreVolumeChangeAction = true;

        m_volume = percent;
        AmarokConfig::setMasterVolume( percent );
        Q_EMIT volumeChanged( percent );
    }
    else
        m_volume = percent;

    m_ignoreVolumeChangeObserve = false;
}

void EngineController::slotMutedChanged( bool mute )
{
    AmarokConfig::setMuteState( mute );
    Q_EMIT muteStateChanged( mute );
}

void
EngineController::slotTrackFinishedPlaying( Meta::TrackPtr track, double playedFraction )
{
    Q_ASSERT( track );
    debug() << "slotTrackFinishedPlaying("
            << ( track->artist() ? track->artist()->name() : QStringLiteral( "[no artist]" ) )
            << "-" << ( track->album() ? track->album()->name() : QStringLiteral( "[no album]" ) )
            << "-" << track->name()
            << "," << playedFraction << ")";

    // Track::finishedPlaying is thread-safe and can take a long time to finish.
    std::thread thread( &Meta::Track::finishedPlaying, track, playedFraction );
    thread.detach();
}

void
EngineController::metadataChanged( const Meta::TrackPtr &track )
{
    Meta::AlbumPtr album = m_currentTrack->album();
    if( m_currentAlbum != album ) {
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        m_currentAlbum = album;
        if( m_currentAlbum )
            subscribeTo( m_currentAlbum );
    }
    Q_EMIT trackMetadataChanged( track );
}

void
EngineController::metadataChanged( const Meta::AlbumPtr &album )
{
    Q_EMIT albumMetadataChanged( album );
}

QString EngineController::prettyNowPlaying( bool progress ) const
{
    Meta::TrackPtr track = currentTrack();

    if( track )
    {
        QString title       = track->name().toHtmlEscaped();
        QString prettyTitle = track->prettyName().toHtmlEscaped();
        QString artist      = track->artist() ? track->artist()->name().toHtmlEscaped() : QString();
        QString album       = track->album() ? track->album()->name().toHtmlEscaped() : QString();

        // ugly because of translation requirements
        if( !title.isEmpty() && !artist.isEmpty() && !album.isEmpty() )
            title = i18nc( "track by artist on album", "<b>%1</b> by <b>%2</b> on <b>%3</b>", title, artist, album );
        else if( !title.isEmpty() && !artist.isEmpty() )
            title = i18nc( "track by artist", "<b>%1</b> by <b>%2</b>", title, artist );
        else if( !album.isEmpty() )
            // we try for pretty title as it may come out better
            title = i18nc( "track on album", "<b>%1</b> on <b>%2</b>", prettyTitle, album );
        else
            title = QStringLiteral("<b>") + prettyTitle + QStringLiteral("</b>");

        if( title.isEmpty() )
            title = i18n( "Unknown track" );

        QScopedPointer<Capabilities::SourceInfoCapability> sic( track->create<Capabilities::SourceInfoCapability>() );
        if( sic )
        {
            QString source = sic->sourceName();
            if( !source.isEmpty() )
                title += QLatin1Char(' ') + i18nc( "track from source", "from <b>%1</b>", source );
        }

        if( track->length() > 0 )
        {
            QString length = Meta::msToPrettyTime( track->length() ).toHtmlEscaped();
            title += QLatin1String(" (");
            if( progress )
                    title += Meta::msToPrettyTime( m_lastTickPosition ).toHtmlEscaped() + QLatin1Char('/');
            title += length + QLatin1Char(')');
        }

        return title;
    }
    else
        return i18n( "No track playing" );
}

bool
EngineController::isInRecentMetaDataHistory( const QVariantMap &meta )
{
    // search for Metadata in history
    for( int i = 0; i < m_metaDataHistory.size(); i++)
    {
        if( m_metaDataHistory.at( i ) == meta ) // we already had that one -> spam!
        {
            m_metaDataHistory.move( i, 0 ); // move spam to the beginning of the list
            return true;
        }
    }

    if( m_metaDataHistory.size() == 12 )
        m_metaDataHistory.removeLast();

    m_metaDataHistory.insert( 0, meta );
    return false;
}

void
EngineController::stampStreamTrackLength()
{
    if( m_lastStreamStampPosition < 0 )
        return;

    qint64 currentPosition = trackPositionMs();
    debug() << "stampStreamTrackLength(): m_lastStreamStampPosition:" << m_lastStreamStampPosition
            << "currentPosition:" << currentPosition;
    if( currentPosition == m_lastStreamStampPosition )
        return;
    qint64 length = qMax( currentPosition - m_lastStreamStampPosition, qint64( 0 ) );
    updateStreamLength( length );

    m_lastStreamStampPosition = currentPosition;
}

void
EngineController::updateStreamLength( qint64 length )
{
    if( !m_currentTrack )
    {
        warning() << __PRETTY_FUNCTION__ << "called with cull m_currentTrack";
        return;
    }

    // Last.fm scrobbling needs to know track length before it can scrobble:
    QVariantMap lengthMetaData;
    // we cannot use m_media->currentSource()->url() here because it is already empty, bug 309976
    lengthMetaData.insert( Meta::Field::URL, QUrl( m_currentTrack->playableUrl() ) );
    lengthMetaData.insert( Meta::Field::LENGTH, length );
    debug() << "updateStreamLength(): emitting currentMetadataChanged(" << lengthMetaData << ")";
    Q_EMIT currentMetadataChanged( lengthMetaData );
}

void
EngineController::updateReplayGainSetting( bool next )
{
    Meta::TrackPtr track = ( next ? m_nextTrack : m_currentTrack );
    if( supportsGainAdjustments() && track
        && AmarokConfig::replayGainMode() != AmarokConfig::EnumReplayGainMode::Off )
    {
        qreal fallbackGain = -3;
        qreal preGain = 3;
        Meta::ReplayGainTag mode;
        // gain is usually negative (but may be positive)
        mode = ( AmarokConfig::replayGainMode() == AmarokConfig::EnumReplayGainMode::Track)
            ? Meta::ReplayGain_Track_Gain
            : Meta::ReplayGain_Album_Gain;
        qreal gain = track->replayGain( mode );

        // peak is usually positive and smaller than gain (but may be negative)
        mode = ( AmarokConfig::replayGainMode() == AmarokConfig::EnumReplayGainMode::Track)
            ? Meta::ReplayGain_Track_Peak
            : Meta::ReplayGain_Album_Peak;
        qreal peak = track->replayGain( mode );
        if( gain + peak + preGain > 0.0 )
        {
            debug() << "Gain of" << gain << "would clip at absolute peak of" << gain + peak + preGain;
            gain -= gain + peak + preGain;
        }

        if( gain == 0.0 && peak == 0.0 )
        {
            debug() << "Replaygain enabled but no gain information for track (type"
                    << typeid( *track.data() ).name() << "), using fallback" << fallbackGain <<"dB";
            gain = fallbackGain;
        }
        else
        {
            debug() << "Using pre-gain" << preGain <<" and gain of" << gain << "with relative peak of" << peak;
            gain+=preGain;
        }
        m_pipeline->setGain ( gain );
    }
    else
    {
        m_pipeline->setGain( 0 );
    }
}
