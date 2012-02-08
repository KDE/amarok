/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
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
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/capabilities/MultiPlayableCapability.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/interfaces/Logger.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/playlists/PlaylistFormat.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"

#include <KFileItem>
#include <KMessageBox>
#include <KRun>
#include <KServiceTypeTrader>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QTextDocument>
#include <QtCore/qmath.h>

namespace The {
    EngineController* engineController() { return EngineController::instance(); }
}

EngineController*
EngineController::instance()
{
    return Amarok::Components::engineController();
}

void
EngineController::destroy()
{
    //nothing to do?
}

EngineController::EngineController()
    : m_fader( 0 )
    , m_fadeoutTimer( 0 )
    , m_boundedPlayback( 0 )
    , m_multiPlayback( 0 )
    , m_multiSource( 0 )
    , m_playWhenFetched( true )
    , m_volume( 0 )
    , m_currentIsAudioCd( false )
    , m_currentAudioCdTrack( 0 )
    , m_ignoreVolumeChangeAction ( false )
    , m_ignoreVolumeChangeObserve ( false )
    , m_tickInterval( 0 )
    , m_lastTickPosition( -1 )
    , m_lastTickCount( 0 )
    , m_mutex( QMutex::Recursive )
{
    DEBUG_BLOCK

}

EngineController::~EngineController()
{
    DEBUG_BLOCK //we like to know when singletons are destroyed

    // don't do any of the after-processing that normally happens when
    // the media is stopped - that's what endSession() is for
    m_media.data()->blockSignals(true);
    m_media.data()->stop();

    delete m_boundedPlayback;
    m_boundedPlayback = 0;
    delete m_multiPlayback; // need to get a new instance of multi if played again
    m_multiPlayback = 0;
    delete m_multiSource;
    m_multiSource = 0;

    delete m_media.data();
    delete m_audio.data();
    delete m_audioDataOutput.data();
}

void
EngineController::createFadeoutEffect()
{
    QMutexLocker locker( &m_mutex );
    if( !m_fader )
    {
        m_fader = new Phonon::VolumeFaderEffect( this );
        m_path.insertEffect( m_fader );
        m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );

        m_fadeoutTimer = new QTimer( this );
        m_fadeoutTimer->setSingleShot( true );
        connect( m_fadeoutTimer, SIGNAL( timeout() ), SLOT( slotStopFadeout() ) );
    }
}

void
EngineController::initializePhonon()
{
    DEBUG_BLOCK

    m_path.disconnect();
    m_dataPath.disconnect();
    delete m_media.data();
    delete m_controller.data();
    delete m_audio.data();
    delete m_audioDataOutput.data();
    delete m_preamp.data();
    delete m_equalizer.data();

    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new Phonon::MediaObject( this );

    // Enable zeitgeist support on linux
    m_media.data()->setProperty("PlaybackTracking", true);

    m_audio = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    m_audioDataOutput = new Phonon::AudioDataOutput( this );

    m_dataPath = Phonon::createPath( m_media.data(), m_audioDataOutput.data() );
    m_path = Phonon::createPath( m_media.data(), m_audio.data() );

    m_controller = new Phonon::MediaController( m_media.data() );

    //Add an equalizer effect if available
    QList<Phonon::EffectDescription> mEffectDescriptions = Phonon::BackendCapabilities::availableAudioEffects();
    foreach ( const Phonon::EffectDescription &mDescr, mEffectDescriptions ) {
        if ( mDescr.name() == QLatin1String( "KEqualizer" ) ) {
            m_equalizer = new Phonon::Effect( mDescr, this );
            eqUpdate();
            }
    }

    // HACK we turn off replaygain manually on OSX, until the phonon coreaudio backend is fixed.
    // as the default is specified in the .cfg file, we can't just tell it to be a different default on OSX
#ifdef Q_WS_MAC
    AmarokConfig::setReplayGainMode( AmarokConfig::EnumReplayGainMode::Off );
    AmarokConfig::setFadeout( false );
#endif

    // only create pre-amp if we have replaygain on, VolumeFaderEffect can cause phonon issues
    if( AmarokConfig::replayGainMode() != AmarokConfig::EnumReplayGainMode::Off )
    {
        m_preamp = new Phonon::VolumeFaderEffect( this );
        m_path.insertEffect( m_preamp.data() );
    }

    m_media.data()->setTickInterval( 100 );
    m_tickInterval = m_media.data()->tickInterval();
    debug() << "Tick Interval (actual): " << m_tickInterval;
    PERF_LOG( "EngineController: loaded phonon objects" )

    // Get the next track when there is 2 seconds left on the current one.
    m_media.data()->setPrefinishMark( 2000 );

    connect( m_media.data(), SIGNAL( finished() ), SLOT( slotFinished() ) );
    connect( m_media.data(), SIGNAL( aboutToFinish() ), SLOT( slotAboutToFinish() ) );
    connect( m_media.data(), SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );
    connect( m_media.data(), SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( slotStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_media.data(), SIGNAL( tick( qint64 ) ), SLOT( slotTick( qint64 ) ) );
    connect( m_media.data(), SIGNAL( totalTimeChanged( qint64 ) ), SLOT( slotTrackLengthChanged( qint64 ) ) );
    connect( m_media.data(), SIGNAL( currentSourceChanged( const Phonon::MediaSource & ) ), SLOT( slotNewTrackPlaying( const Phonon::MediaSource & ) ) );
    connect( m_media.data(), SIGNAL( seekableChanged( bool ) ), SLOT( slotSeekableChanged( bool ) ) );
    connect( m_audio.data(), SIGNAL( volumeChanged( qreal ) ), SLOT( slotVolumeChanged( qreal ) ) );
    connect( m_audio.data(), SIGNAL( mutedChanged( bool ) ), SLOT( slotMutedChanged( bool ) ) );
    connect( m_audioDataOutput.data(), SIGNAL(dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)), this, SIGNAL(audioDataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)));

    connect( m_controller.data(), SIGNAL( titleChanged( int ) ), SLOT( slotTitleChanged( int ) ) );

    // Read the volume from phonon
    m_volume = qBound<qreal>( 0, qRound(m_audio.data()->volume()*100), 100 );

    if( AmarokConfig::trackDelayLength() > -1 )
        m_media.data()->setTransitionTime( AmarokConfig::trackDelayLength() ); // Also Handles gapless.
    else if( AmarokConfig::crossfadeLength() > 0 )  // TODO: Handle the possible options on when to crossfade.. the values are not documented anywhere however
        m_media.data()->setTransitionTime( -AmarokConfig::crossfadeLength() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

bool
EngineController::canDecode( const KUrl &url ) //static
{
   //NOTE this function must be thread-safe

    // We can't use playlists in the engine
    if( Playlists::isPlaylist( url ) )
        return false;

    KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url );
    // If file has 0 bytes, ignore it and return false
    if( !item.size() )
        return false;

    // We can't play directories, regardless of what the engine says.
    if( item.isDir() )
        return false;

    // Accept non-local files, since we can't test them for validity at this point
    if( !item.isLocalFile() )
        return true;

    // Phonon::BackendCapabilities::isMimeTypeAvailable is too simplistic for our purposes
    // FIXME: this variable should be updated when
    // Phonon::BackendCapabilities::notifier()'s capabilitiesChanged signal is emitted
    static QStringList mimeTable = supportedMimeTypes();

    const KMimeType::Ptr mimeType = item.mimeTypePtr();

    bool valid = false;
    foreach( const QString &type, mimeTable )
    {
        if( mimeType->is( type ) )
        {
            valid = true;
            break;
        }
    }

    return valid;
}

QStringList
EngineController::supportedMimeTypes() //static
{
    //NOTE this function must be thread-safe

    // Filter the available mime types to only include audio and video, as amarok does not intend to play photos
    static QRegExp avFilter( "^(audio|video)/", Qt::CaseInsensitive );
    // NB: we can't make this static, as we edit it later in the method; however, this method
    //     should not be called too often.
    QStringList mimeTable = Phonon::BackendCapabilities::availableMimeTypes().filter( avFilter );

    // Add whitelist hacks

    // MP4 Audio Books have a different extension that KFileItem/Phonon don't grok
    if( !mimeTable.contains( "audio/x-m4b" ) )
        mimeTable << "audio/x-m4b";

    // technically, "audio/flac" is not a valid mimetype (not on IANA list), but some things expect it
    if( mimeTable.contains( "audio/x-flac" ) && !mimeTable.contains( "audio/flac" ) )
        mimeTable << "audio/flac";

    // We special case this, as otherwise the users would hate us
    // Again, "audio/mp3" is not a valid mimetype, but is widely used
    // (the proper one is "audio/mpeg", but that is also for .mp1 and .mp2 files)
    if( !mimeTable.contains( "audio/mp3" ) && !mimeTable.contains( "audio/x-mp3" ) )
    {
        if ( !installDistroCodec() )
        {
            Amarok::Components::logger()->longMessage(
                    i18n( "<p>Phonon claims it <b>cannot</b> play MP3 files. You may want to examine "
                          "the installation of the backend that phonon uses.</p>"
                          "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok Handbook</i>.</p>" ), Amarok::Logger::Error );
        }
        mimeTable << "audio/mp3" << "audio/x-mp3";
    }

    return mimeTable;
}

bool
EngineController::installDistroCodec()
{
    KService::List services = KServiceTypeTrader::self()->query( "Amarok/CodecInstall"
        , QString( "[X-KDE-Amarok-codec] == 'mp3' and [X-KDE-Amarok-engine] == 'phonon-%1'").arg( "xine" ) );
    //todo - figure out how to query Phonon for the current backend loaded
    if( !services.isEmpty() )
    {
        KService::Ptr service = services.first(); //list is not empty
        QString installScript = service->exec();
        if( !installScript.isNull() ) //just a sanity check
        {
            KGuiItem installButton( i18n( "Install MP3 Support" ) );
            if(KMessageBox::questionYesNo( The::mainWindow()
            , i18n("Amarok currently cannot play MP3 files. Do you want to install support for MP3?")
            , i18n( "No MP3 Support" )
            , installButton
            , KStandardGuiItem::no()
            , "codecInstallWarning" ) == KMessageBox::Yes )
            {
                    KRun::runCommand(installScript, 0);
                    return true;
            }
        }
    }

    return false;
}

void
EngineController::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if( AmarokConfig::resumePlayback() )
    {
        const KUrl url = AmarokConfig::resumeTrack();
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

        // Only give a resume time for local files, because resuming remote protocols can have weird side effects.
        // See: http://bugs.kde.org/show_bug.cgi?id=172897
        if( url.isLocalFile() )
            play( track, AmarokConfig::resumeTime() );
        else
            play( track );
    }
}

void
EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() && m_currentTrack )
    {
        emit stopped( trackPositionMs(), m_currentTrack->length() );
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        emit trackChanged( Meta::TrackPtr( 0 ) );
    }
    emit sessionEnded( AmarokConfig::resumePlayback() && m_currentTrack );
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

    resetFadeout();

    if( isPaused() )
    {
        if( m_currentTrack && m_currentTrack->type() == "stream" ) // SHOUTcast does not support resuming, so we restart it.
        {
            debug() << "This is a stream that cannot be resumed after pausing. Restarting instead.";
            play( m_currentTrack );
            return;
        }
        else
        {
            m_media.data()->play();
            emit trackPlaying( m_currentTrack );
            return;
        }
    }

    The::playlistActions()->play();
}

void
EngineController::play( Meta::TrackPtr track, uint offset )
{
    DEBUG_BLOCK

    if( !track ) // Guard
        return;

    // clear the current track without sending playbackEnded or trackChangeNotify yet
    // we want to continue but ::stop() doesn't know that
    if( m_currentTrack )
    {
        const qint64 pos = trackPositionMs();
        const qint64 length = m_currentTrack->length();
        m_currentTrack->finishedPlaying( double(pos)/double(length) );
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        m_currentTrack = 0;
        m_currentAlbum = 0;
    }

    stop( true );

    m_currentTrack = track;
    debug() << "play: bounded is "<<m_boundedPlayback<<"current"<<m_currentTrack->name();
    m_boundedPlayback = m_currentTrack->create<Capabilities::BoundedPlaybackCapability>();
    m_multiPlayback = m_currentTrack->create<Capabilities::MultiPlayableCapability>();
    m_multiSource = m_currentTrack->create<Capabilities::MultiSourceCapability>();

    m_currentTrack->prepareToPlay();

    if( m_multiPlayback )
    {
        connect( m_multiPlayback, SIGNAL( playableUrlFetched( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        m_multiPlayback->fetchFirst();
    }
    else if( m_multiSource )
    {
        debug() << "Got a MultiSource Track with " <<  m_multiSource->sources().count() << " sources";
        connect( m_multiSource, SIGNAL( urlChanged( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        playUrl( m_currentTrack->playableUrl(), 0 );
    }
    else if( m_boundedPlayback )
    {
        debug() << "Starting bounded playback of url " << m_currentTrack->playableUrl() << " at position " << m_boundedPlayback->startPosition();
        playUrl( m_currentTrack->playableUrl(), m_boundedPlayback->startPosition() );
    }
    else
    {
        debug() << "Just a normal, boring track... :-P";
        playUrl( m_currentTrack->playableUrl(), offset );
    }
}

void
EngineController::replay() // slot
{
    DEBUG_BLOCK

    seek( 0 );
    emit trackPositionChanged( 0, true );
}

void
EngineController::playUrl( const KUrl &url, uint offset )
{
    DEBUG_BLOCK

    m_media.data()->stop();
    resetFadeout();

    debug() << "URL: " << url << url.url();
    debug() << "Offset: " << offset;

    if( url.url().startsWith( "audiocd:/" ) )
    {
        m_currentIsAudioCd = true;
        //disconnect this signal for now or it will cause a loop that will cause a mutex lockup
        disconnect( m_controller.data(), SIGNAL( titleChanged( int ) ), this, SLOT( slotTitleChanged( int ) ) );

        debug() << "play track from cd";
        QString trackNumberString = url.url();
        trackNumberString = trackNumberString.remove( "audiocd:/" );

        const QString devicePrefix( "?device=" );
        QString device("");
        if( trackNumberString.contains( devicePrefix ) )
        {
            int pos = trackNumberString.indexOf( devicePrefix );
            device = QUrl::fromPercentEncoding(
                        trackNumberString.mid( pos + devicePrefix.size() ).toLocal8Bit() );
            trackNumberString = trackNumberString.left( pos );
        }

        QStringList parts = trackNumberString.split( '/' );

        if ( parts.count() != 2 )
            return;

        QString discId = parts.at( 0 );

        //we really only want to play it if it is the disc that is currently present.
        //In the case of CDs for which we don't have any id, any "unknown" CDs will
        //be considered equal.


        //FIXME:
        //if ( MediaDeviceMonitor::instance()->currentCdId() != discId )
        //    return;


        int trackNumber = parts.at( 1 ).toInt();

        debug() << "Old device: " << m_media.data()->currentSource().deviceName();

        Phonon::MediaSource::Type type = m_media.data()->currentSource().type();
        if( type != Phonon::MediaSource::Disc
                || m_media.data()->currentSource().deviceName() != device )
        {
            m_media.data()->clear();
            debug() << "New device: " << device;
            m_media.data()->setCurrentSource( Phonon::MediaSource( Phonon::Cd, device ) );
        }

        debug() << "Track: " << trackNumber;
        m_currentAudioCdTrack = trackNumber;
        m_controller.data()->setCurrentTitle( trackNumber );
        debug() << "no boom?";

        if( type == Phonon::MediaSource::Disc )
        {
            // The track has changed but the slot may not be called,
            // because it may be still the same media source, which means
            // we need to do it explicitly.
            slotNewTrackPlaying( m_media.data()->currentSource() );
        }

        //reconnect it
        connect( m_controller.data(), SIGNAL( titleChanged( int ) ), SLOT( slotTitleChanged( int ) ) );

    }
    else
    {
        if ( url.toLocalFile().isEmpty() )
        {
            m_media.data()->setCurrentSource( url );
        }
        else
        {
            m_media.data()->setCurrentSource( url.toLocalFile() );
        }
    }

    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media.data()->clearQueue();

    if( offset )
    {
        debug() << "seeking to " << offset;
        m_media.data()->pause();
        m_media.data()->seek( offset );
    }
    m_media.data()->play();
    emit trackPositionChanged( offset, true );

    debug() << "track pos after play: " << trackPositionMs();
}

void
EngineController::pause() //SLOT
{
    m_media.data()->pause();
    emit paused();
}

void
EngineController::stop( bool forceInstant ) //SLOT
{
    DEBUG_BLOCK

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack )
    {
        const qint64 pos = trackPositionMs();
        const qint64 length = m_currentTrack->length();
        m_currentTrack->finishedPlaying( double(pos)/double(length) );
        emit stopped( pos, length );

        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );

        m_currentTrack = 0;
        m_currentAlbum = 0;
        emit trackChanged( m_currentTrack );
    }

    {
        QMutexLocker locker( &m_mutex );
        m_currentIsAudioCd = false;
        delete m_boundedPlayback;
        m_boundedPlayback = 0;
        delete m_multiPlayback; // need to get a new instance of multi if played again
        m_multiPlayback = 0;
        delete m_multiSource;
        m_multiSource = 0;

        m_nextTrack.clear();
        m_nextUrl.clear();
        m_media.data()->clearQueue();
    }

    // Stop instantly if fadeout is already running, or the media is not playing
    if( ( m_fadeoutTimer && m_fadeoutTimer->isActive() ) ||
        m_media.data()->state() != Phonon::PlayingState )
    {
        forceInstant = true;
    }

    if( AmarokConfig::fadeout() && AmarokConfig::fadeoutLength() && !forceInstant )
    {
        // WARNING: this can cause a gap in playback in GStreamer
        createFadeoutEffect();
        m_fader->fadeOut( AmarokConfig::fadeoutLength() );
        m_fadeoutTimer->start( AmarokConfig::fadeoutLength() + 1000 ); //add 1s for good measure, otherwise seems to cut off early (buffering..)
    }
    else
    {
        m_media.data()->stop();
        m_media.data()->setCurrentSource( Phonon::MediaSource() );
    }
}

bool
EngineController::isPaused() const
{
    return m_media.data()->state() == Phonon::PausedState;
}

bool
EngineController::isPlaying() const
{
    return !isPaused() && !isStopped();
}

bool
EngineController::isStopped() const
{
    return (m_media.data()->state() == Phonon::StoppedState) ||
        (m_media.data()->state() == Phonon::LoadingState) ||
        (m_media.data()->state() == Phonon::ErrorState) ||
        (m_fadeoutTimer && m_fadeoutTimer->isActive());
}

void
EngineController::playPause() //SLOT
{
    DEBUG_BLOCK
    debug() << "PlayPause: EngineController state" << m_media.data()->state();

    if( isPlaying() )
        pause();
    else
        play();
}

void
EngineController::seek( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( m_media.data()->isSeekable() )
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

        m_media.data()->seek( static_cast<qint64>( seekTo ) );
        emit trackPositionChanged( seekTo, true ); /* User seek */
    }
    else
        debug() << "Stream is not seekable.";
}


void
EngineController::seekRelative( int ms ) //SLOT
{
    qint64 newPos = m_media.data()->currentTime() + ms;
    seek( newPos <= 0 ? 0 : newPos );
}

void
EngineController::seekForward( int ms )
{
    seekRelative( ms );
}

void
EngineController::seekBackward( int ms )
{
    seekRelative( -ms );
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
EngineController::setVolume( int percent ) //SLOT
{
    percent = qBound<qreal>( 0, percent, 100 );
    m_volume = percent;

    const qreal volume =  percent / 100.0;
    if ( !m_ignoreVolumeChangeAction && m_audio.data()->volume() != volume )
    {
        m_ignoreVolumeChangeObserve = true;
        m_audio.data()->setVolume( volume );

        AmarokConfig::setMasterVolume( percent );
        emit volumeChanged( percent );
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
    return m_audio.data()->isMuted();
}

void
EngineController::setMuted( bool mute ) //SLOT
{
    m_audio.data()->setMuted( mute ); // toggle mute
    if( !isMuted() )
        setVolume( m_volume );

    AmarokConfig::setMuteState( mute );
    emit muteStateChanged( mute );
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
    if( m_currentTrack && m_currentTrack->length() > 0 )   //When starting a last.fm stream, Phonon still shows the old track's length--trust Meta::Track over Phonon
        return m_currentTrack->length();
    else
        return m_media.data()->totalTime(); //may return -1
}

void
EngineController::setNextTrack( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    debug() << "locking mutex";
    QMutexLocker locker( &m_mutex );
    debug() << "locked!";

    if( !track )
        return;

    track->prepareToPlay();
    if( track->playableUrl().isEmpty() )
        return;

    if( isPlaying() )
    {
        m_media.data()->clearQueue();
        if( track->playableUrl().isLocalFile() )
            m_media.data()->enqueue( track->playableUrl() );
        m_nextTrack = track;
        m_nextUrl = track->playableUrl();
    }
    else
    {
        play( track );
    }
}

/*
*/
bool
EngineController::isStream()
{
    DEBUG_BLOCK

    if( m_media )
        return m_media.data()->currentSource().type() == Phonon::MediaSource::Stream;
    return false;
}

bool
EngineController::isSeekable() const
{
    if( m_media )
        return m_media.data()->isSeekable();
    return false;
}

int
EngineController::trackPosition() const
{
//NOTE: there was a bunch of last.fm logic removed from here
//pretty sure it's irrelevant, if not, look back to mid-March 2008
    return static_cast<int>( m_media.data()->currentTime() / 1000 );
}

int
EngineController::trackPositionMs() const
{
    return m_media.data()->currentTime();
}

bool
EngineController::isEqSupported() const
{
    // If effect was created it means we have equalizer support
    return ( m_equalizer );
}

double
EngineController::eqMaxGain() const
{
   if( !m_equalizer )
       return 100;
   QList<Phonon::EffectParameter> mEqPar = m_equalizer.data()->parameters();
   if( mEqPar.isEmpty() )
       return 100.0;
   double mScale;
   mScale = ( qAbs(mEqPar.at(0).maximumValue().toDouble() ) +  qAbs( mEqPar.at(0).minimumValue().toDouble() ) );
   mScale /= 2.0;
   return mScale;
}

QStringList
EngineController::eqBandsFreq() const
{
    // This will extract the bands frequency values from effect parameter name
    // as long as they follow the rules:
    // eq-preamp parameter will contain 'pre-amp' string
    // bands parameters are described using schema 'xxxHz'
    QStringList mBandsFreq;
    if( !m_equalizer )
       return mBandsFreq;
    QList<Phonon::EffectParameter> mEqPar = m_equalizer.data()->parameters();
    if( mEqPar.isEmpty() )
       return mBandsFreq;
    QRegExp rx( "\\d+(?=Hz)" );
    foreach( const Phonon::EffectParameter &mParam, mEqPar )
    {
        if( mParam.name().contains( QString( "pre-amp" ) ) )
        {
            mBandsFreq << i18n( "Preamp" );
        }
        else if ( mParam.name().contains( rx ) )
        {
            if( rx.cap( 0 ).toInt() < 1000 )
            {
                mBandsFreq << i18n( "%0\nHz" ).arg( rx.cap( 0 ) );
            }
            else
            {
                mBandsFreq << i18n( "%0\nkHz" ).arg( QString::number( rx.cap( 0 ).toInt()/1000 ) );
            }
        }
    }
    return mBandsFreq;
}

void
EngineController::eqUpdate() //SLOT
{
    // if equalizer not present simply return
    if( !m_equalizer )
        return;
    // check if equalizer should be disabled ??
    if( AmarokConfig::equalizerMode() <= 0 )
    {
        // Remove effect from path
//         if( m_path.effects().indexOf( m_equalizer.data() ) != -1 )
//             m_path.removeEffect( m_equalizer.data() );

        if( m_path.effects().indexOf( m_equalizer.data() ) != -1 )
            m_path.removeEffect( m_equalizer.data() );
    }
    else
    {
        // Set equalizer parameter according to the gains from settings
        QList<Phonon::EffectParameter> mEqPar = m_equalizer.data()->parameters();
        QList<int> mEqParCfg = AmarokConfig::equalizerGains();

        QListIterator<int> mEqParNewIt( mEqParCfg );
        double scaledVal; // Scaled value to set from universal -100 - 100 range to plugin scale
        foreach( const Phonon::EffectParameter &mParam, mEqPar )
        {
            scaledVal = mEqParNewIt.hasNext() ? mEqParNewIt.next() : 0;
            scaledVal *= ( qAbs(mParam.maximumValue().toDouble() ) +  qAbs( mParam.minimumValue().toDouble() ) );
            scaledVal /= 200.0;
            m_equalizer.data()->setParameterValue( mParam, scaledVal );
        }
        // Insert effect into path if needed
        if( m_path.effects().indexOf( m_equalizer.data() ) == -1 )
        {
            if( !m_path.effects().isEmpty() )
            {
                m_path.insertEffect( m_equalizer.data(), m_path.effects().first() );
            }
            else
            {
                m_path.insertEffect( m_equalizer.data() );
            }
        }
    }
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
        emit trackPositionChanged( static_cast<long>( position - m_boundedPlayback->startPosition() ), false );

        // Calculate a better position.  Sometimes the position doesn't update
        // with a good resolution (for example, 1 sec for TrueAudio files in the
        // Xine-1.1.18 backend).  This tick function, in those cases, just gets
        // called multiple times with the same position.  We count how many
        // times this has been called prior, and adjust for it.
        if ( position == m_lastTickPosition )
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
        emit trackPositionChanged( static_cast<long>( position ), false ); //it expects milliseconds
    }
}

void
EngineController::slotAboutToFinish()
{
    DEBUG_BLOCK
    debug() << "Track finished completely, updating statistics";

    if( m_currentTrack ) // not sure why this should not be the case, but sometimes happens. don't crash.
    {
        m_currentTrack->finishedPlaying( 1.0 ); // If we reach aboutToFinish, the track is done as far as we are concerned.
    }
    if( m_multiPlayback )
    {
        DEBUG_LINE_INFO
        m_mutex.lock();
        m_playWhenFetched = false;
        m_mutex.unlock();
        m_multiPlayback->fetchNext();
        debug() << "The queue has: " << m_media.data()->queue().size() << " tracks in it";
    }
    else if( m_multiSource )
    {
        debug() << "source finished, lets get the next one";
        KUrl nextSource = m_multiSource->next();

        if ( !nextSource.isEmpty() )
        { //more sources
            m_mutex.lock();
            m_playWhenFetched = false;
            m_mutex.unlock();
            debug() << "playing next source: " << nextSource;
            slotPlayableUrlFetched( nextSource );
        }
        else if( m_media.data()->queue().isEmpty() )
        { //go to next track
            The::playlistActions()->requestNextTrack();
            debug() << "no more sources, skip to next track";
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
    else if( m_currentTrack && m_currentTrack->playableUrl().url().startsWith( "audiocd:/" ) )
    {
        debug() << "finished a CD track, don't care if queue is not empty, just get new track...";

        The::playlistActions()->requestNextTrack();
        slotFinished();
    }
    else if( m_media.data()->queue().isEmpty() )
        The::playlistActions()->requestNextTrack();
}

void
EngineController::slotFinished()
{
    DEBUG_BLOCK

    if( m_currentTrack && !m_multiPlayback && !m_multiSource )
    {
        if( !m_nextTrack && m_nextUrl.isEmpty() )
            emit stopped( trackPositionMs(), m_currentTrack->length() );
        unsubscribeFrom( m_currentTrack );
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        m_currentTrack = 0;
        m_currentAlbum = 0;
        if( !m_nextTrack && m_nextUrl.isEmpty() ) // we will the trackChanged signal later
            emit trackChanged( m_currentTrack );
        m_media.data()->setCurrentSource( Phonon::MediaSource() );
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
        // possibly we are waiting for a fetch
        m_playWhenFetched = true;

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

    // the new track was taken from the queue, so clear these fields
    if( m_nextTrack )
    {
        m_currentTrack = m_nextTrack;
        m_nextTrack.clear();
    }

    if( !m_nextUrl.isEmpty() )
        m_nextUrl.clear();

    if ( m_currentTrack && AmarokConfig::replayGainMode() != AmarokConfig::EnumReplayGainMode::Off )
    {
        if( !m_preamp ) // replaygain was just turned on, and amarok was started with it off
        {
            m_preamp = new Phonon::VolumeFaderEffect( this );
            m_path.insertEffect( m_preamp.data() );
        }

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
        if ( gain + peak > 0.0 )
        {
            debug() << "Gain of" << gain << "would clip at absolute peak of" << gain + peak;
            gain -= gain + peak;
        }
        debug() << "Using gain of" << gain << "with relative peak of" << peak;
        // we calculate the volume change ourselves, because m_preamp.data()->setVolumeDecibel is
        // a little confused about minus signs
        m_preamp.data()->setVolume( qExp( gain * log10over20 ) );
        m_preamp.data()->fadeTo( qExp( gain * log10over20 ), 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879
    }
    else if( m_preamp )
    {
        m_preamp.data()->setVolume( 1.0 );
        m_preamp.data()->fadeTo( 1.0, 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879
    }

    if( m_currentTrack )
    {
        subscribeTo( m_currentTrack );
        Meta::AlbumPtr m_currentAlbum = m_currentTrack->album();
        if( m_currentAlbum )
            subscribeTo( m_currentAlbum );
    }
    emit trackChanged( m_currentTrack );
    emit trackPlaying( m_currentTrack );
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    DEBUG_BLOCK

    static const int maxErrors = 5;
    static int errorCount = 0;

    // Sanity checks:
    if( newState == oldState )
        return;

    if( newState == Phonon::ErrorState )  // If media is borked, skip to next track
    {
        emit trackError( m_currentTrack );

        warning() << "Phonon failed to play this URL. Error: " << m_media.data()->errorString();
        warning() << "Forcing phonon engine reinitialization.";

        /* In case of error Phonon MediaObject automaticaly switches to KioMediaSource,
           which cause problems: runs StopAfterCurrentTrack mode, force PlayPause button to
           reply the track (can't be paused). So we should reinitiate Phonon after each Error.
        */
        initializePhonon();

        errorCount++;
        if ( errorCount >= maxErrors )
        {
            // reset error count
            errorCount = 0;

            Amarok::Components::logger()->longMessage(
                            i18n( "Too many errors encountered in playlist. Playback stopped." ),
                            Amarok::Logger::Warning
                        );
            error() << "Stopping playlist.";
        }
        else
        {
            // and start the next song
            slotAboutToFinish();
        }

    }
    else if( newState == Phonon::PlayingState )
    {
        errorCount = 0;
        emit playbackStateChanged();
    }
    else if( newState == Phonon::StoppedState ||
             newState == Phonon::PausedState )
    {
        emit playbackStateChanged();
    }
}

void
EngineController::slotPlayableUrlFetched( const KUrl &url )
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
        m_media.data()->clearQueue();
        if( url.isLocalFile() )
            m_media.data()->enqueue( url );
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
    DEBUG_BLOCK

    emit trackLengthChanged( ( !m_multiPlayback || !m_boundedPlayback ) ? trackLength() : milliseconds );
}

void
EngineController::slotMetaDataChanged()
{
    DEBUG_BLOCK

    QVariantMap meta;

    meta.insert( Meta::Field::URL, m_media.data()->currentSource().url().toString() );

    QStringList artist = m_media.data()->metaData( "ARTIST" );
    debug() << "Artist     : " << artist;
    if( !artist.isEmpty() )
        meta.insert( Meta::Field::ARTIST, artist.first() );

    QStringList album = m_media.data()->metaData( "ALBUM" );
    debug() << "Album      : " << album;
    if( !album.isEmpty() )
        meta.insert( Meta::Field::ALBUM, album.first() );

    QStringList title = m_media.data()->metaData( "TITLE" );
    debug() << "Title      : " << title;
    if( !title.isEmpty() )
        meta.insert( Meta::Field::TITLE, title.first() );

    QStringList genre = m_media.data()->metaData( "GENRE" );
    debug() << "Genre      : " << genre;
    if( !genre.isEmpty() )
        meta.insert( Meta::Field::GENRE, genre.first() );

    QStringList tracknum = m_media.data()->metaData( "TRACKNUMBER" );
    debug() << "Tracknumber: " << tracknum;
    if( !tracknum.isEmpty() )
        meta.insert( Meta::Field::TRACKNUMBER, tracknum.first() );

    QStringList length = m_media.data()->metaData( "LENGTH" );
    debug() << "Length     : " << length;
    if( !length.isEmpty() )
        meta.insert( Meta::Field::LENGTH, length.first() );

    bool trackChanged = false;
    if( m_lastTrack != m_currentTrack )
    {
        trackChanged = true;
        m_lastTrack = m_currentTrack;
    }
    debug() << "Track changed: " << trackChanged << "current:" << m_currentTrack.data() << "url"<<m_media.data()->currentSource().url().toString();

    if( isMetadataSpam( meta ) )
        return;

    debug() << "no spam";
    emit currentMetadataChanged( meta );
}

void
EngineController::slotStopFadeout() //SLOT
{
    DEBUG_BLOCK

    m_media.data()->stop();
    m_media.data()->setCurrentSource( Phonon::MediaSource() );
    resetFadeout();
}

void
EngineController::resetFadeout()
{
    if( m_fader )
    {
        m_fader->setVolume( 1.0 );
        m_fader->fadeTo( 1.0, 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879

        m_fadeoutTimer->stop();
    }
}

void
EngineController::slotSeekableChanged( bool seekable )
{
    emit seekableChanged( seekable );
}

void EngineController::slotTitleChanged( int titleNumber )
{
    DEBUG_BLOCK
    if ( titleNumber != m_currentAudioCdTrack )
    {
        slotAboutToFinish();
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
        emit volumeChanged( percent );
    }
    else
        m_volume = percent;

    m_ignoreVolumeChangeObserve = false;
}

void EngineController::slotMutedChanged( bool mute )
{
    AmarokConfig::setMuteState( mute );
    emit muteStateChanged( mute );
}

void
EngineController::metadataChanged( Meta::TrackPtr track )
{
    Meta::AlbumPtr album = m_currentTrack->album();
    if( m_currentAlbum != album ) {
        if( m_currentAlbum )
            unsubscribeFrom( m_currentAlbum );
        m_currentAlbum = album;
        if( m_currentAlbum )
            subscribeTo( m_currentAlbum );
    }
    emit trackMetadataChanged( track );
}

void
EngineController::metadataChanged( Meta::AlbumPtr album )
{
    emit albumMetadataChanged( album );
}


bool EngineController::isPlayingAudioCd()
{
    return m_currentIsAudioCd;
}

QString EngineController::prettyNowPlaying( bool progress ) const
{
    Meta::TrackPtr track = currentTrack();

    if( track )
    {
        QString title       = Qt::escape( track->name() );
        QString prettyTitle = Qt::escape( track->prettyName() );
        QString artist      = track->artist() ? Qt::escape( track->artist()->name() ) : QString();
        QString album       = track->album() ? Qt::escape( track->album()->name() ) : QString();

        // ugly because of translation requirements
        if ( !title.isEmpty() && !artist.isEmpty() && !album.isEmpty() )
            title = i18nc( "track by artist on album", "<b>%1</b> by <b>%2</b> on <b>%3</b>", title, artist, album );

        else if ( !title.isEmpty() && !artist.isEmpty() )
            title = i18nc( "track by artist", "<b>%1</b> by <b>%2</b>", title, artist );

        else if ( !album.isEmpty() )
            // we try for pretty title as it may come out better
            title = i18nc( "track on album", "<b>%1</b> on <b>%2</b>", prettyTitle, album );
        else
            title = "<b>" + prettyTitle + "</b>";

        if ( title.isEmpty() )
            title = i18n( "Unknown track" );

        QScopedPointer<Capabilities::SourceInfoCapability> sic( track->create<Capabilities::SourceInfoCapability>() );
        if ( sic )
        {
            QString source = sic->sourceName();
            if ( !source.isEmpty() )
                title += ' ' + i18nc( "track from source", "from <b>%1</b>", source );
        }

        if ( track->length() > 0 ) {
            QString length = Qt::escape( Meta::msToPrettyTime( track->length() ) );
            title += " (";
            if ( progress )
                    title+= Qt::escape( Meta::msToPrettyTime( m_lastTickPosition ) ) + "/";
            title += length + ")";
        }

        return title;
    }
    else
        return i18n( "No track playing" );
}

QVariantMap
EngineController::trackData( Meta::TrackPtr track )
{
    QVariantMap meta;

    meta.insert( Meta::Field::URL, track->playableUrl() );
    if( track->artist() )
        meta.insert( Meta::Field::ARTIST, track->artist()->prettyName() );
    if( track->album() )
        meta.insert( Meta::Field::ALBUM, track->album()->name() );
    meta.insert( Meta::Field::TITLE, track->prettyName() );
    if( track->genre() )
        meta.insert( Meta::Field::GENRE, track->genre()->prettyName() );
    meta.insert( Meta::Field::TRACKNUMBER, track->trackNumber() );
    meta.insert( Meta::Field::LENGTH, track->length() );

    return meta;
}

bool
EngineController::isMetadataSpam( QVariantMap meta )
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


#include "EngineController.moc"
