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

#include "core/support/Amarok.h"
#include "amarokconfig.h"
#include "collection/CollectionManager.h"
#include "core/support/Components.h"
#include "statusbar/StatusBar.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "MediaDeviceMonitor.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/MultiPlayableCapability.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "playlist/PlaylistActions.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"
#include "core/plugins/PluginManager.h"

#include <KFileItem>
#include <KIO/Job>
#include <KMessageBox>
#include <KRun>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QTextDocument>
#include <QTimer>

#include <cmath>

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
    : m_playWhenFetched( true )
    , m_fadeoutTimer( new QTimer( this ) )
    , m_volume( 0 )
    , m_currentIsAudioCd( false )
{
    DEBUG_BLOCK

    m_fadeoutTimer->setSingleShot( true );

    connect( m_fadeoutTimer, SIGNAL( timeout() ), SLOT( slotStopFadeout() ) );
}

EngineController::~EngineController()
{
    DEBUG_BLOCK //we like to know when singletons are destroyed

    // don't do any of the after-processing that normally happens when
    // the media is stopped - that's what endSession() is for
    m_media->blockSignals(true);
    m_media->stop();

    delete m_media;
    delete m_audio;
}

void
EngineController::createFadeoutEffect()
{
    m_fader = new Phonon::VolumeFaderEffect( this );
    m_path.insertEffect( m_fader );
    m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );
}

void
EngineController::initializePhonon()
{
    DEBUG_BLOCK

    m_path.disconnect();
    delete m_media;
    delete m_controller;
    delete m_audio;
    delete m_preamp;
    delete m_equalizer;
    delete m_fader;

    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new Phonon::MediaObject( this );
    m_audio = new Phonon::AudioOutput( Phonon::MusicCategory, this );

    m_path = Phonon::createPath( m_media, m_audio );

    m_controller = new Phonon::MediaController( m_media );

    //Add an equalizer effect if available
    QList<Phonon::EffectDescription> mEffectDescriptions = Phonon::BackendCapabilities::availableAudioEffects();
    foreach ( const Phonon::EffectDescription &mDescr, mEffectDescriptions ) {
        if ( mDescr.name() == QLatin1String( "KEqualizer" ) ) {
            m_equalizer = new Phonon::Effect( mDescr );
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
        m_path.insertEffect( m_preamp );
    }

    // only create fader if we have fadeout on, VolumeFaderEffect can cause phonon issues
    if( AmarokConfig::fadeout() && AmarokConfig::fadeoutLength() )
    {
        createFadeoutEffect();
    }

    m_media->setTickInterval( 100 );
    debug() << "Tick Interval (actual): " << m_media->tickInterval();
    PERF_LOG( "EngineController: loaded phonon objects" )

    // Get the next track when there is 2 seconds left on the current one.
    m_media->setPrefinishMark( 2000 );

    connect( m_media, SIGNAL( finished() ), SLOT( slotQueueEnded() ) );
    connect( m_media, SIGNAL( aboutToFinish()), SLOT( slotAboutToFinish() ) );
    connect( m_media, SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );
    connect( m_media, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( slotStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_media, SIGNAL( tick( qint64 ) ), SLOT( slotTick( qint64 ) ) );
    connect( m_media, SIGNAL( totalTimeChanged( qint64 ) ), SLOT( slotTrackLengthChanged( qint64 ) ) );
    connect( m_media, SIGNAL( currentSourceChanged( const Phonon::MediaSource & ) ), SLOT( slotNewTrackPlaying( const Phonon::MediaSource & ) ) );

    connect( m_controller, SIGNAL( titleChanged( int ) ), SLOT( slotTitleChanged( int ) ) );


    if( AmarokConfig::trackDelayLength() > -1 )
        m_media->setTransitionTime( AmarokConfig::trackDelayLength() ); // Also Handles gapless.
    else if( AmarokConfig::crossfadeLength() > 0 )  // TODO: Handle the possible options on when to crossfade.. the values are not documented anywhere however
        m_media->setTransitionTime( -AmarokConfig::crossfadeLength() );
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

    // Filter the available mime types to only include audio and video, as amarok does not intend to play photos
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
EngineController::supportedMimeTypes()
{
    //NOTE this function must be thread-safe
    // Filter the available mime types to only include audio and video, as amarok does not intend to play photos
    static QStringList mimeTable = Phonon::BackendCapabilities::availableMimeTypes().filter( "audio/", Qt::CaseInsensitive ) +
                                   Phonon::BackendCapabilities::availableMimeTypes().filter( "video/", Qt::CaseInsensitive );

    // Add whitelist hacks
    mimeTable << "audio/x-m4b"; // MP4 Audio Books have a different extension that KFileItem/Phonon don't grok

    // We special case this, as otherwise the users would hate us
    if( ( !mimeTable.contains( "audio/mp3" ) && !mimeTable.contains( "audio/x-mp3" ) ) && !installDistroCodec() )
    {
        The::statusBar()->longMessage(
                i18n( "<p>Phonon claims it <b>cannot</b> play MP3 files. You may want to examine "
                      "the installation of the backend that phonon uses.</p>"
                      "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok Handbook</i>.</p>" ), StatusBar::Error );
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

        // Only resume local files, because resuming remote protocols can have weird side effects.
        // See: http://bugs.kde.org/show_bug.cgi?id=172897
        if( url.isLocalFile() )
        {
            Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
            play( track, AmarokConfig::resumeTime() );
        }
    }
}

void
EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() && m_currentTrack )
    {
        playbackEnded( trackPositionMs(), m_currentTrack->length(), Engine::EngineObserver::EndedQuit );
        trackChangedNotify( Meta::TrackPtr( 0 ) );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::play() //SLOT
{
    DEBUG_BLOCK

    // FIXME: what should we do in buffering state?
    if( state() == Phonon::PlayingState )
        return;

    resetFadeout();

    if ( state() == Phonon::PausedState )
    {
        m_media->play();
    }
    else
    {
        The::playlistActions()->play();
    }
}

void
EngineController::play( const Meta::TrackPtr& track, uint offset )
{
    DEBUG_BLOCK

    if( !track ) // Guard
        return;

    m_currentTrack = track;
    m_currentIsAudioCd = false;
    delete m_boundedPlayback;
    delete m_multiPlayback;
    delete m_multiSource;
    m_boundedPlayback = m_currentTrack->create<Capabilities::BoundedPlaybackCapability>();
    m_multiPlayback = m_currentTrack->create<Capabilities::MultiPlayableCapability>();
    m_multiSource  = m_currentTrack->create<Capabilities::MultiSourceCapability>();


    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();

    m_currentTrack->prepareToPlay();

    if( m_multiPlayback )
    {
        m_media->stop();
        connect( m_multiPlayback, SIGNAL( playableUrlFetched( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        m_multiPlayback->fetchFirst();
    }
    else if ( m_multiSource )
    {
        m_media->stop();
        debug() << "Got a MultiSource Track with " <<  m_multiSource->sources().count() << " sources";
        connect( m_multiSource, SIGNAL( urlChanged( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        playUrl( m_currentTrack->playableUrl(), 0 );
    }
    else if ( m_boundedPlayback )
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
EngineController::playUrl( const KUrl &url, uint offset )
{
    DEBUG_BLOCK

    m_media->stop();
    resetFadeout();

    debug() << "URL: " << url.url();
    debug() << "offset: " << offset;

    if ( url.url().startsWith( "audiocd:/" ) )
    {

        m_currentIsAudioCd = true;
        //disconnect this signal for now or it will cause a loop that will cause a mutex lockup
        disconnect( m_controller, SIGNAL( titleChanged( int ) ), this, SLOT( slotTitleChanged( int ) ) );

        debug() << "play track from cd";
        QString trackNumberString = url.url();
        trackNumberString = trackNumberString.remove( "audiocd:/" );

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

        debug() << "3.2.1...";

        Phonon::MediaSource::Type type = m_media->currentSource().type();
        if( type != Phonon::MediaSource::Disc )
        {
            m_media->clear();
            m_media->setCurrentSource( Phonon::Cd );
        }

        debug() << "boom?";
        m_controller->setCurrentTitle( trackNumber );
        debug() << "no boom?";

        if( type == Phonon::MediaSource::Disc )
        {
            // The track has changed but the slot will not be called,
            // because it's still the same media source, which means
            // we need to do it explicitly.
            slotNewTrackPlaying( Phonon::Cd );
        }

        //reconnect it
        connect( m_controller, SIGNAL( titleChanged( int ) ), SLOT( slotTitleChanged( int ) ) );

    }
    else
    {
        if ( url.toLocalFile().isEmpty() )
        {
            m_media->setCurrentSource( url );
        }
        else
        {
            m_media->setCurrentSource( url.toLocalFile() );
        }
    }

    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();

    if( offset )
    {
        debug() << "seeking to " << offset;
        m_media->pause();
        m_media->seek( offset );
    }
    m_media->play();

    debug() << "track pos after play: " << trackPositionMs();


}

void
EngineController::pause() //SLOT
{
    if( m_currentTrack && m_currentTrack->type() == "stream" ) // SHOUTcast doesn't support pausing, so we stop.
    {
        debug() << "This is a stream that cannot be paused. Stopping instead.";
        stop();
        return;
    }

    m_media->pause();
}

void
EngineController::stop( bool forceInstant ) //SLOT
{
    DEBUG_BLOCK

    m_currentIsAudioCd = false;
    // need to get a new instance of multi if played again
    delete m_multiPlayback;
    delete m_multiSource;

    m_mutex.lock();
    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();
    m_mutex.unlock();

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack )
    {
        debug() << "m_currentTrack != 0";
        const qint64 pos = trackPositionMs();
        const qint64 length = m_currentTrack->length();
        m_currentTrack->finishedPlaying( double(pos)/double(length) );
        playbackEnded( pos, length, Engine::EngineObserver::EndedStopped );
        trackChangedNotify( Meta::TrackPtr( 0 ) );
    }

    // Stop instantly if fadeout is already running, or the media is not playing
    if( m_fadeoutTimer->isActive() || m_media->state() != Phonon::PlayingState )
    {
        forceInstant = true;
    }

    if( AmarokConfig::fadeout() && AmarokConfig::fadeoutLength() && !forceInstant )
    {
        // WARNING: this can cause a gap in playback in GStreamer
        if (! m_fader )
            createFadeoutEffect();

        m_fader->fadeOut( AmarokConfig::fadeoutLength() );

        m_fadeoutTimer->start( AmarokConfig::fadeoutLength() + 1000 ); //add 1s for good measure, otherwise seems to cut off early (buffering..)

        stateChangedNotify( Phonon::StoppedState, m_media->state() ); //immediately disable Stop action
    }
    else
    {
        m_media->stop();
        m_media->setCurrentSource( Phonon::MediaSource() );
    }

    m_currentTrack = 0;
}

bool
EngineController::isPaused() const
{
    return state() == Phonon::PausedState;
}

void
EngineController::playPause() //SLOT
{
    DEBUG_BLOCK

    //this is used by the TrayIcon, PlayPauseAction and DBus
    debug() << "PlayPause: EngineController state" << state();

    switch ( state() )
    {
        case Phonon::PausedState:
        case Phonon::StoppedState:

        case Phonon::LoadingState:
            play();
            break;

        default:
            pause();
            break;
    }
}

void
EngineController::seek( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( m_media->isSeekable() )
    {

        debug() << "seek to: " << ms;
        int seekTo;

        if ( m_boundedPlayback )
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
        // FIXME: is this correct for bounded playback?
        trackPositionChangedNotify( seekTo, true ); /* User seek */
    }
    else
        debug() << "Stream is not seekable.";
}


void
EngineController::seekRelative( int ms ) //SLOT
{
    qint64 newPos = m_media->currentTime() + ms;
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
    m_audio->setVolume( volume );

    AmarokConfig::setMasterVolume( percent );
    volumeChangedNotify( percent );

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

    AmarokConfig::setMuteState( mute );
    muteStateChangedNotify( mute );
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
    const qint64 phononLength = m_media->totalTime(); //may return -1

    if( m_currentTrack && m_currentTrack->length() > 0 )   //When starting a last.fm stream, Phonon still shows the old track's length--trust Meta::Track over Phonon
        return m_currentTrack->length();
    else
        return phononLength;
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

    if( state() == Phonon::PlayingState ||
        state() == Phonon::BufferingState )
    {
        m_media->clearQueue();
        if( track->playableUrl().isLocalFile() )
            m_media->enqueue( track->playableUrl() );
        m_nextTrack = track;
        m_nextUrl = track->playableUrl();
    }
    else
    {
        play( track );
    }
}

Phonon::State
EngineController::state() const
{
    if ( m_fadeoutTimer->isActive() )
        return Phonon::StoppedState;
    else
        return phononMediaObject()->state();
}

bool
EngineController::isStream()
{
    DEBUG_BLOCK

    if( m_media )
        return m_media->currentSource().type() == Phonon::MediaSource::Stream;
    return false;
}

int
EngineController::trackPosition() const
{
//NOTE: there was a bunch of last.fm logic removed from here
//pretty sure it's irrelevant, if not, look back to mid-March 2008
    return static_cast<int>( m_media->currentTime() / 1000 );
}

int
EngineController::trackPositionMs() const
{
    return m_media->currentTime();
}

bool
EngineController::isEqSupported() const
{
    // If effect was created it means we have equalizer support
    return ( !m_equalizer.isNull() );
}

double
EngineController::eqMaxGain() const
{
   if( m_equalizer.isNull() )
       return 100;
   QList<Phonon::EffectParameter> mEqPar = m_equalizer->parameters();
   if( mEqPar.isEmpty() )
       return 100.0;
   double mScale;
   mScale = ( fabs(mEqPar.at(0).maximumValue().toDouble() ) +  fabs( mEqPar.at(0).minimumValue().toDouble() ) );
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
    if( m_equalizer.isNull() )
       return mBandsFreq;
    QList<Phonon::EffectParameter> mEqPar = m_equalizer->parameters();
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
                mBandsFreq << QString( rx.cap( 0 )).append( "\nHz" );
            }
            else
            {
                mBandsFreq << QString::number( rx.cap( 0 ).toInt()/1000 ).append( "\nkHz" );
            }
        }
    }
    return mBandsFreq;
}

void
EngineController::eqUpdate() //SLOT
{
    // if equalizer not present simply return
    if( m_equalizer.isNull() )
        return;
    // check if equalizer should be disabled ??
    if( AmarokConfig::equalizerMode() <= 0 )
    {
        // Remove effect from path
        if( m_path.effects().indexOf( m_equalizer ) != -1 )
            m_path.removeEffect( m_equalizer );
    }
    else
    {
        // Set equalizer parameter according to the gains from settings
        QList<Phonon::EffectParameter> mEqPar = m_equalizer->parameters();
        QList<int> mEqParCfg = AmarokConfig::equalizerGains();

        QListIterator<int> mEqParNewIt( mEqParCfg );
        double scaledVal; // Scaled value to set from universal -100 - 100 range to plugin scale
        foreach( const Phonon::EffectParameter &mParam, mEqPar )
        {
            scaledVal = mEqParNewIt.hasNext() ? mEqParNewIt.next() : 0;
            scaledVal *= ( fabs(mParam.maximumValue().toDouble() ) +  fabs( mParam.minimumValue().toDouble() ) );
            scaledVal /= 200.0;
            m_equalizer->setParameterValue( mParam, scaledVal );
        }
        // Insert effect into path if needed
        if( m_path.effects().indexOf( m_equalizer ) == -1 )
        {
            if( !m_path.effects().isEmpty() )
            {
                m_path.insertEffect( m_equalizer, m_path.effects().first() );
            }
            else
            {
                m_path.insertEffect( m_equalizer );
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
    if ( m_boundedPlayback )
    {
        trackPositionChangedNotify( static_cast<long>( position - m_boundedPlayback->startPosition() ), false );

        //don't go beyond the stop point
        if ( position >= m_boundedPlayback->endPosition() )
        {
            slotAboutToFinish();
        }
    }
    else
    {
        trackPositionChangedNotify( static_cast<long>( position ), false ); //it expects milliseconds
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
        trackFinishedNotify( m_currentTrack );
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
        KUrl nextSource = m_multiSource->next();

        if ( !nextSource.isEmpty() )
        { //more sources
            m_mutex.lock();
            m_playWhenFetched = false;
            m_mutex.unlock();
            debug() << "playing next source: " << nextSource;
            slotPlayableUrlFetched( nextSource );
        }
        else if( m_media->queue().isEmpty() )
        { //go to next track
            The::playlistActions()->requestNextTrack();
            debug() << "no more sources, skip to next track";
        }
    }
    else if ( m_boundedPlayback )
    {
        debug() << "finished a track that consists of part of another track, go to next track even if this url is technically not done yet";

        //stop this track, now, as the source track might go on and on, and
        //there might not be any more tracks in the playlist...
        stop( true );
        The::playlistActions()->requestNextTrack();
        slotQueueEnded();
    }
    else if ( m_currentTrack && m_currentTrack->playableUrl().url().startsWith( "audiocd:/" ) )
    {
        debug() << "finished a CD track, don't care if queue is not empty, just get new track...";

        The::playlistActions()->requestNextTrack();
        slotQueueEnded();
    }
    else if( m_media->queue().isEmpty() )
        The::playlistActions()->requestNextTrack();
}

void
EngineController::slotQueueEnded()
{
    DEBUG_BLOCK

    if( m_currentTrack && !m_multiPlayback && !m_multiSource )
    {
        m_media->setCurrentSource( Phonon::MediaSource() );
        playbackEnded( trackPositionMs(), m_currentTrack->length(), Engine::EngineObserver::EndedStopped );
        m_currentTrack = 0;
        trackChangedNotify( m_currentTrack );
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
            m_path.insertEffect( m_preamp );
        }

        Meta::Track::ReplayGainMode mode = ( AmarokConfig::replayGainMode() == AmarokConfig::EnumReplayGainMode::Track)
                                         ? Meta::Track::TrackReplayGain
                                         : Meta::Track::AlbumReplayGain;
        // gain is usually negative (but may be positive)
        qreal gain = m_currentTrack->replayGain( mode );
        // peak is usually positive and smaller than gain (but may be negative)
        qreal peak = m_currentTrack->replayPeakGain( mode );
        if ( gain + peak > 0.0 )
        {
            debug() << "Gain of" << gain << "would clip at absolute peak of" << gain + peak;
            gain -= gain + peak;
        }
        debug() << "Using gain of" << gain << "with relative peak of" << peak;
        // we calculate the volume change ourselves, because m_preamp->setVolumeDecibel is
        // a little confused about minus signs
        m_preamp->setVolume( exp( gain * log10over20 ) );
        m_preamp->fadeTo( exp( gain * log10over20 ), 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879
    }
    else if( m_preamp )
    {
        m_preamp->setVolume( 1.0 );
        m_preamp->fadeTo( 1.0, 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879
    }

    trackChangedNotify( m_currentTrack );
    newTrackPlaying();
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    DEBUG_BLOCK

    // Sanity checks:
    if( newState == oldState )
        return;

    if( newState == Phonon::ErrorState )  // If media is borked, skip to next track
    {
        warning() << "Phonon failed to play this URL. Error: " << m_media->errorString();
        if( m_multiPlayback )
        {
            DEBUG_LINE_INFO
            m_mutex.lock();
            m_playWhenFetched = true;
            m_mutex.unlock();
            m_multiPlayback->fetchNext();
            debug() << "The queue has: " << m_media->queue().size() << " tracks in it";
        }
        else if( m_multiSource )
        {
            debug() << "source error, lets get the next one";
            KUrl nextSource = m_multiSource->next();

            if ( !nextSource.isEmpty() )
            { //more sources
                m_mutex.lock();
                m_playWhenFetched = false;
                m_mutex.unlock();
                debug() << "playing next source: " << nextSource;
                slotPlayableUrlFetched( nextSource );
            }
            else if( m_media->queue().isEmpty() )
                The::playlistActions()->requestNextTrack();
        }

        else if( m_media->queue().isEmpty() )
            The::playlistActions()->requestNextTrack();
    }

    if ( m_fadeoutTimer->isActive() )
    {
        // We've stopped already as far as the rest of Amarok is concerned
        if ( oldState == Phonon::PlayingState )
            oldState = Phonon::StoppedState;

        if ( oldState == newState )
            return;
    }

    stateChangedNotify( newState, oldState );
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
        m_media->clearQueue();
        if( url.isLocalFile() )
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
    DEBUG_BLOCK

    trackLengthChangedNotify( ( m_multiPlayback || m_boundedPlayback ) ? trackLength() : milliseconds );
}

void
EngineController::slotMetaDataChanged()
{
    DEBUG_BLOCK

    QHash<qint64, QString> meta;

    meta.insert( Meta::valUrl, m_media->currentSource().url().toString() );

    QStringList artist = m_media->metaData( "ARTIST" );
    debug() << "Artist     : " << artist;
    if( !artist.isEmpty() )
        meta.insert( Meta::valArtist, artist.first() );

    QStringList album = m_media->metaData( "ALBUM" );
    debug() << "Album      : " << album;
    if( !album.isEmpty() )
        meta.insert( Meta::valAlbum, album.first() );

    QStringList title = m_media->metaData( "TITLE" );
    debug() << "Title      : " << title;
    if( !title.isEmpty() )
        meta.insert( Meta::valTitle, title.first() );

    QStringList genre = m_media->metaData( "GENRE" );
    debug() << "Genre      : " << genre;
    if( !genre.isEmpty() )
        meta.insert( Meta::valGenre, genre.first() );

    QStringList tracknum = m_media->metaData( "TRACKNUMBER" );
    debug() << "Tracknumber: " << tracknum;
    if( !tracknum.isEmpty() )
        meta.insert( Meta::valTrackNr, tracknum.first() );

    QStringList length = m_media->metaData( "LENGTH" );
    debug() << "Length     : " << length;
    if( !length.isEmpty() )
        meta.insert( Meta::valLength, length.first() );

    bool trackChanged = false;
    if( m_lastTrack != m_currentTrack )
    {
        trackChanged = true;
        m_lastTrack = m_currentTrack;
    }
    debug() << "Track changed: " << trackChanged;
    newMetaDataNotify( meta, trackChanged );
}

void
EngineController::slotStopFadeout() //SLOT
{
    DEBUG_BLOCK

    m_media->stop();
    m_media->setCurrentSource( Phonon::MediaSource() );
    resetFadeout();
}

void
EngineController::resetFadeout()
{
    m_fadeoutTimer->stop();
    if ( m_fader )
    {
        m_fader->setVolume( 1.0 );
        m_fader->fadeTo( 1.0, 0 ); // HACK: we use fadeTo because setVolume is b0rked in Phonon Xine before r1028879
    }
}

void EngineController::slotTitleChanged( int titleNumber )
{
    DEBUG_BLOCK
    Q_UNUSED( titleNumber );

    slotAboutToFinish();
}

bool EngineController::isPlayingAudioCd()
{
    return m_currentIsAudioCd;
}

QString EngineController::prettyNowPlaying()
{
    Meta::TrackPtr track = currentTrack();

    if( track )
    {
        QString title       = Qt::escape( track->name() );
        QString prettyTitle = Qt::escape( track->prettyName() );
        QString artist      = track->artist() ? Qt::escape( track->artist()->name() ) : QString();
        QString album       = track->album() ? Qt::escape( track->album()->name() ) : QString();
        QString length      = Qt::escape( Meta::msToPrettyTime( track->length() ) );

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

        Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
        if ( sic )
        {
            QString source = sic->sourceName();
            if ( !source.isEmpty() )
                title += ' ' + i18nc( "track from source", "from <b>%1</b>", source );

            delete sic;
        }

        if ( length.length() > 1 )
            title += " (" + length + ')';

        return title;
    }
    else
        return i18n( "No track playing" );
}

#include "EngineController.moc"
