/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2006,2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#include <phonon/AudioOutput>
#include <phonon/BackendCapabilities>
#include <phonon/MediaObject>
#include <phonon/VolumeFaderEffect>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QTimer>
#include <QtMath>

#include <KLocalizedString>

#include <thread>


// for slotMetaDataChanged()
typedef QPair<Phonon::MetaData, QString> FieldPair;

namespace The {
    EngineController* engineController() { return EngineController::instance(); }
}

EngineController *
EngineController::instance()
{
    return Amarok::Components::engineController();
}

EngineController::EngineController()
    : m_boundedPlayback( nullptr )
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
    if( m_media )
    {
        m_media->blockSignals(true);
        m_media->stop();
    }

    delete m_boundedPlayback;
    m_boundedPlayback = nullptr;
    delete m_multiPlayback; // need to get a new instance of multi if played again
    m_multiPlayback = nullptr;

    delete m_media.data();
    delete m_audio.data();
    delete m_audioDataOutput.data();
}

void
EngineController::initializePhonon()
{
    DEBUG_BLOCK

    m_path.disconnect();
    m_dataPath.disconnect();

    // QWeakPointers reset themselves to null if the object is deleted
    delete m_media.data();
    delete m_controller.data();
    delete m_audio.data();
    delete m_audioDataOutput.data();
    delete m_preamp.data();
    delete m_fader.data();

    using namespace Phonon;
    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new MediaObject( this );

    // Enable zeitgeist support on linux
    //TODO: make this configurable by the user.
    m_media->setProperty( "PlaybackTracking", true );

    m_audio = new AudioOutput( MusicCategory, this );
    m_audioDataOutput = new AudioDataOutput( this );
    m_audioDataOutput->setDataSize( DATAOUTPUT_DATA_SIZE ); // The number of samples that Phonon sends per signal

    m_path = createPath( m_media.data(), m_audio.data() );

    m_controller = new MediaController( m_media.data() );

    m_equalizerController->initialize( m_path );

    // HACK we turn off replaygain manually on OSX, until the phonon coreaudio backend is fixed.
    // as the default is specified in the .cfg file, we can't just tell it to be a different default on OSX
#ifdef Q_OS_APPLE
    AmarokConfig::setReplayGainMode( AmarokConfig::EnumReplayGainMode::Off );
    AmarokConfig::setFadeoutOnStop( false );
#endif

    // we now try to create pre-amp unconditionally, however we check that it is valid.
    // So now m_preamp is null   equals   not available at all
    QScopedPointer<VolumeFaderEffect> preamp( new VolumeFaderEffect( this ) );
    if( preamp->isValid() )
    {
        m_preamp = preamp.take();
        m_path.insertEffect( m_preamp.data() );
    }

    QScopedPointer<VolumeFaderEffect> fader( new VolumeFaderEffect( this ) );
    if( fader->isValid() )
    {
        fader->setFadeCurve( VolumeFaderEffect::Fade9Decibel );
        m_fader = fader.take();
        m_path.insertEffect( m_fader.data() );
        m_dataPath = createPath( m_fader.data(), m_audioDataOutput.data() );
    }
    else
        m_dataPath = createPath( m_media.data(), m_audioDataOutput.data() );

    m_media.data()->setTickInterval( 100 );
    m_tickInterval = m_media.data()->tickInterval();
    debug() << "Tick Interval (actual): " << m_tickInterval;
    PERF_LOG( "EngineController: loaded phonon objects" )

    // Get the next track when there is 2 seconds left on the current one.
    m_media.data()->setPrefinishMark( 2000 );

    connect( m_media.data(), &MediaObject::finished, this, &EngineController::slotFinished );
    connect( m_media.data(), &MediaObject::aboutToFinish, this, &EngineController::slotAboutToFinish );
    connect( m_media.data(), &MediaObject::metaDataChanged, this, &EngineController::slotMetaDataChanged );
    connect( m_media.data(), &MediaObject::stateChanged, this, &EngineController::slotStateChanged );
    connect( m_media.data(), &MediaObject::tick, this, &EngineController::slotTick );
    connect( m_media.data(), &MediaObject::totalTimeChanged, this, &EngineController::slotTrackLengthChanged );
    connect( m_media.data(), &MediaObject::currentSourceChanged, this, &EngineController::slotNewTrackPlaying );
    connect( m_media.data(), &MediaObject::seekableChanged, this, &EngineController::slotSeekableChanged );
    connect( m_audio.data(), &AudioOutput::volumeChanged, this, &EngineController::slotVolumeChanged );
    connect( m_audio.data(), &AudioOutput::mutedChanged, this, &EngineController::slotMutedChanged );
    connect( m_audioDataOutput.data(), &AudioDataOutput::dataReady, this, &EngineController::audioDataReady );

    connect( m_controller.data(), &MediaController::titleChanged, this, &EngineController::slotTitleChanged );

    // Read the volume from phonon
    m_volume = qBound<qreal>( 0, qRound(m_audio.data()->volume()*100), 100 );

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

    QRegularExpression avFilter( "^(audio|video)/", QRegularExpression::CaseInsensitiveOption);
    m_supportedMimeTypes = Phonon::BackendCapabilities::availableMimeTypes().filter( avFilter );

    // Add whitelist hacks
    // MP4 Audio Books have a different extension that KFileItem/Phonon don't grok
    if( !m_supportedMimeTypes.contains( "audio/x-m4b" ) )
        m_supportedMimeTypes << "audio/x-m4b";

    // technically, "audio/flac" is not a valid mimetype (not on IANA list), but some things expect it
    if( m_supportedMimeTypes.contains( "audio/x-flac" ) && !m_supportedMimeTypes.contains( "audio/flac" ) )
        m_supportedMimeTypes << "audio/flac";

    // technically, "audio/mp4" is the official mime type, but sometimes Phonon returns audio/x-m4a
    if( m_supportedMimeTypes.contains( "audio/x-m4a" ) && !m_supportedMimeTypes.contains( "audio/mp4" ) )
        m_supportedMimeTypes << "audio/mp4";

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
        if( m_currentTrack && m_currentTrack->type() == "stream" )
        {
            debug() << "This is a stream that cannot be resumed after pausing. Restarting instead.";
            play( m_currentTrack );
            return;
        }
        else
        {
            m_pauseTimer->stop();
            if( supportsFadeout() )
                m_fader->setVolume( 1.0 );
            m_media->play();
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

    m_media->stop();

    debug() << "URL: " << url << url.url();
    debug() << "Offset: " << offset;

    m_currentAudioCdTrack = 0;
    if( url.scheme() == "audiocd" )
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
        QString device = QUrlQuery(url).queryItemValue( "device" );

        m_media->setCurrentSource( Phonon::MediaSource( Phonon::Cd, device ) );
        m_currentAudioCdTrack = trackNumber;
    }
    else
    {
        // keep in sync with setNextTrack(), slotPlayableUrlFetched()
        m_media->setCurrentSource( url );
    }

    m_media->clearQueue();

    if( m_currentAudioCdTrack )
    {
        // call to play() is asynchronous and ->setCurrentTitle() can be only called on
        // playing, buffering or paused media.
        m_media->pause();
        DelayedTrackChanger *trackChanger = new DelayedTrackChanger( m_media.data(),
                m_controller.data(), m_currentAudioCdTrack, offset, startPaused );
        connect( trackChanger, &DelayedTrackChanger::trackPositionChanged,
                 this, &EngineController::trackPositionChanged );
    }
    else if( offset )
    {
        // call to play() is asynchronous and ->seek() can be only called on playing,
        // buffering or paused media. Calling play() would lead to audible glitches,
        // so call pause() that doesn't suffer from such problem.
        m_media->pause();
        DelayedSeeker *seeker = new DelayedSeeker( m_media.data(), offset, startPaused );
        connect( seeker, &DelayedSeeker::trackPositionChanged,
                 this, &EngineController::trackPositionChanged );
    }
    else
    {
        if( startPaused )
        {
            m_media->pause();
        }
        else
        {
            m_pauseTimer->stop();
            if( supportsFadeout() )
                m_fader->setVolume( 1.0 );
            m_media->play();
        }
    }
}

void
EngineController::pause() //SLOT
{
    if( supportsFadeout() && AmarokConfig::fadeoutOnPause() )
    {
        m_fader->fadeOut( AmarokConfig::fadeoutLength() );
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
        m_fader->setVolume( 1.0 );

        // Wait a bit before pausing the pipeline. Necessary for the new fader setting to take effect.
        QTimer::singleShot( 1000, m_media.data(), &Phonon::MediaObject::pause );
    }
    else
    {
        m_media->pause();
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
                  && m_media->state() == Phonon::PlayingState
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
        m_media->clearQueue();
    }

    if( doFadeOut )
    {
        m_fadeouter = new Fadeouter( m_media, m_fader, AmarokConfig::fadeoutLength() );
        // even though we don't pass forceInstant, doFadeOut will be false because
        // m_fadeouter will be still valid
        connect( m_fadeouter.data(), &Fadeouter::fadeoutFinished,
                 this, &EngineController::regularStop );
    }
    else
    {
        // empty MediaSource() seems to cause Phonon-VLC to emit a volume of 0, BR 442319, so ignore it
        m_ignoreVolumeChangeObserve = true;
        m_media->stop();
        m_media->setCurrentSource( Phonon::MediaSource() );
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
    return m_media->state() == Phonon::PausedState;
}

bool
EngineController::isPlaying() const
{
    return !isPaused() && !isStopped();
}

bool
EngineController::isStopped() const
{
    return
        m_media->state() == Phonon::StoppedState ||
        m_media->state() == Phonon::LoadingState ||
        m_media->state() == Phonon::ErrorState;
}

void
EngineController::playPause() //SLOT
{
    DEBUG_BLOCK
    debug() << "PlayPause: EngineController state" << m_media->state();

    if( isPlaying() )
        pause();
    else
        play();
}

void
EngineController::seekTo( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( m_media->isSeekable() )
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

        m_media->seek( static_cast<qint64>( seekTo ) );
        Q_EMIT trackPositionChanged( seekTo, true ); /* User seek */
    }
    else
        debug() << "Stream is not seekable.";
}


void
EngineController::seekBy( int ms ) //SLOT
{
    qint64 newPos = m_media->currentTime() + ms;
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
    if ( !m_ignoreVolumeChangeAction && m_audio->volume() != volume )
    {
        m_ignoreVolumeChangeObserve = true;
        m_audio->setVolume( volume );

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
    return m_audio->isMuted();
}

void
EngineController::setMuted( bool mute ) //SLOT
{
    m_audio->setMuted( mute ); // toggle mute
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
    else
        return m_media->totalTime(); //may return -1
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
        m_media->clearQueue();
        // keep in sync with playUrl(), slotPlayableUrlFetched()
        if( url.scheme() != "audiocd" ) // we don't support gapless for CD, bug 305708
            m_media->enqueue( url );
        m_nextTrack = track;
        m_nextUrl = url;
    }
    else
        play( track );
}

bool
EngineController::isStream()
{
    Phonon::MediaSource::Type type = Phonon::MediaSource::Invalid;
    if( m_media )
        // type is determined purely from the MediaSource constructor used in
        // setCurrentSource(). For streams we use the QUrl one, see playUrl()
        type = m_media->currentSource().type();
    // NOTE Broken; local files can be Urls, too. This is used only by WikipediaEngine at the moment
    return type == Phonon::MediaSource::Url || type == Phonon::MediaSource::Stream;
}

bool
EngineController::isSeekable() const
{
    if( m_media )
        return m_media->isSeekable();
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
    return m_media->currentTime();
}

bool
EngineController::supportsFadeout() const
{
    return m_fader;
}

bool EngineController::supportsGainAdjustments() const
{
    return m_preamp;
}

bool EngineController::supportsAudioDataOutput() const
{
    const Phonon::AudioDataOutput out;
    return out.isValid();
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
        debug() << "The queue has: " << m_media->queue().size() << " tracks in it";
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
        else if( m_media->queue().isEmpty() )
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
    else if( m_media->queue().isEmpty() )
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
        m_media->setCurrentSource( Phonon::MediaSource() );
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

static const qreal log10over20 = 0.1151292546497022842; // ln(10) / 20

void
EngineController::slotNewTrackPlaying( const Phonon::MediaSource &source )
{
    DEBUG_BLOCK

    if( source.type() == Phonon::MediaSource::Empty )
    {
        debug() << "Empty MediaSource (engine stop)";
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

    if( m_currentTrack
        && AmarokConfig::replayGainMode() != AmarokConfig::EnumReplayGainMode::Off )
    {
        Meta::ReplayGainTag mode;
        // gain is usually negative (but may be positive)
        mode = ( AmarokConfig::replayGainMode() == AmarokConfig::EnumReplayGainMode::Track)
            ? Meta::ReplayGain_Track_Gain
            : Meta::ReplayGain_Album_Gain;
        qreal gain = m_currentTrack->replayGain( mode );

        // peak is usually positive and smaller than gain (but may be negative)
        mode = ( AmarokConfig::replayGainMode() == AmarokConfig::EnumReplayGainMode::Track)
            ? Meta::ReplayGain_Track_Peak
            : Meta::ReplayGain_Album_Peak;
        qreal peak = m_currentTrack->replayGain( mode );
        if( gain + peak > 0.0 )
        {
            debug() << "Gain of" << gain << "would clip at absolute peak of" << gain + peak;
            gain -= gain + peak;
        }

        if( m_preamp )
        {
            debug() << "Using gain of" << gain << "with relative peak of" << peak;
            // we calculate the volume change ourselves, because m_preamp->setVolumeDecibel is
            // a little confused about minus signs
            m_preamp->setVolume( qExp( gain * log10over20 ) );
        }
        else
            warning() << "Would use gain of" << gain << ", but current Phonon backend"
                      << "doesn't seem to support pre-amplifier (VolumeFaderEffect)";
    }
    else if( m_preamp )
    {
        m_preamp->setVolume( 1.0 );
    }

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
        if( m_currentTrack->type() == "stream" && m_currentTrack->length() == 0 )
            useTrackWithinStreamDetection = true;
    }

    m_lastStreamStampPosition = useTrackWithinStreamDetection ? 0 : -1;
    Q_EMIT trackChanged( m_currentTrack );
    Q_EMIT trackPlaying( m_currentTrack );
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    debug() << "slotStateChanged from" << oldState << "to" << newState;

    static const int maxErrors = 5;
    static int errorCount = 0;

    // Sanity checks:
    if( newState == oldState )
        return;

    if( newState == Phonon::ErrorState )  // If media is borked, skip to next track
    {
        Q_EMIT trackError( m_currentTrack );

        warning() << "Phonon failed to play this URL. Error: " << m_media->errorString();
        warning() << "Forcing phonon engine reinitialization.";

        /* In case of error Phonon MediaObject automatically switches to KioMediaSource,
           which cause problems: runs StopAfterCurrentTrack mode, force PlayPause button to
           reply the track (can't be paused). So we should reinitiate Phonon after each Error.
        */
        initializePhonon();

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
    else if( newState == Phonon::PlayingState )
    {
        errorCount = 0;
        Q_EMIT playbackStateChanged();
    }
    else if( newState == Phonon::StoppedState ||
             newState == Phonon::PausedState )
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
        m_media->clearQueue();
        // keep synced with setNextTrack(), playUrl()
        m_media->enqueue( url );
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
    meta.insert( Meta::Field::URL, m_media->currentSource().url() );
    static const QList<FieldPair> fieldPairs = QList<FieldPair>()
            << FieldPair( Phonon::ArtistMetaData, Meta::Field::ARTIST )
            << FieldPair( Phonon::AlbumMetaData, Meta::Field::ALBUM )
            << FieldPair( Phonon::TitleMetaData, Meta::Field::TITLE )
            << FieldPair( Phonon::GenreMetaData, Meta::Field::GENRE )
            << FieldPair( Phonon::TracknumberMetaData, Meta::Field::TRACKNUMBER )
            << FieldPair( Phonon::DescriptionMetaData, Meta::Field::COMMENT );
    foreach( const FieldPair &pair, fieldPairs )
    {
        QStringList values = m_media->metaData( pair.first );
        if( !values.isEmpty() )
            meta.insert( pair.second, values.first() );
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
            title = "<b>" + prettyTitle + "</b>";

        if( title.isEmpty() )
            title = i18n( "Unknown track" );

        QScopedPointer<Capabilities::SourceInfoCapability> sic( track->create<Capabilities::SourceInfoCapability>() );
        if( sic )
        {
            QString source = sic->sourceName();
            if( !source.isEmpty() )
                title += ' ' + i18nc( "track from source", "from <b>%1</b>", source );
        }

        if( track->length() > 0 )
        {
            QString length = Meta::msToPrettyTime( track->length() ).toHtmlEscaped();
            title += " (";
            if( progress )
                    title += Meta::msToPrettyTime( m_lastTickPosition ).toHtmlEscaped() + '/';
            title += length + ')';
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

