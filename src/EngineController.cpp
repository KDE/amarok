/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *             (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2006, 2008 Ian Monroe <ian@monroe.nu>                   *
 *             (C) 2008 Jason A. Donenfeld <Jason@zx2c4.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "EngineController"

#include "EngineController.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "collection/CollectionManager.h"
#include "statusbar/StatusBar.h"
#include "Debug.h"
#include "MainWindow.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "meta/capabilities/MultiPlayableCapability.h"
#include "playlist/PlaylistActions.h"
#include "playlistmanager/PlaylistManager.h"
#include "PluginManager.h"

#include <KFileItem>
#include <KIO/Job>
#include <KMessageBox>
#include <KRun>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QTimer>

#include <cmath>

namespace The {
    EngineController* engineController() { return EngineController::instance(); }
}

EngineController* EngineController::s_instance = 0;

EngineController*
EngineController::instance()
{
    return s_instance ? s_instance : new EngineController();
}

void
EngineController::destroy()
{
    delete s_instance;
    s_instance = 0;
}

EngineController::EngineController()
    : m_playWhenFetched( true )
    , m_fadeoutTimer( new QTimer( this ) )
{
    DEBUG_BLOCK

    initializePhonon();

    m_fadeoutTimer->setSingleShot( true );

    connect( m_fadeoutTimer, SIGNAL( timeout() ), SLOT( slotStopFadeout() ) );

    s_instance = this;
}

EngineController::~EngineController()
{
    DEBUG_BLOCK //we like to know when singletons are destroyed

    m_media->stop();

    delete m_media;
    delete m_audio;
}

void
EngineController::initializePhonon()
{
    DEBUG_BLOCK

    delete m_media;
    delete m_audio;
    delete m_preamp;

    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new Phonon::MediaObject( this );
    m_audio = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    m_preamp = new Phonon::VolumeFaderEffect( this );

    m_path = Phonon::createPath( m_media, m_audio );
    m_path.insertEffect( m_preamp );

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

    
    //TODO: The xine engine does not support crossfading. Cannot get the gstreamer engine to work, will test this once I do.
#if 0
    if( AmarokConfig::trackDelayLength() > -1 )
        m_media->setTransitionTime( AmarokConfig::trackDelayLength() ); // Also Handles gapless.
    else if( AmarokConfig::crossfadeLength() > 0 )  // TODO: Handle the possible options on when to crossfade.. the values are not documented anywhere however
        m_media->setTransitionTime( -AmarokConfig::crossfadeLength() );
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

bool
EngineController::canDecode( const KUrl &url ) //static
{
   //NOTE this function must be thread-safe

    // We can't use playlists in the engine
    if( PlaylistManager::isPlaylist( url ) )
        return false;

    KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url );
    // If file has 0 bytes, ignore it and return false
    if( !item.size() )
        return false;

    // We can't play directories, reguardless of what the engine says.
    if( item.isDir() )
        return false;

    // Accept non-local files, since we can't test them for validity at this point
    if( !item.isLocalFile() )
        return true;

    // Filter the available mime types to only include audio and video, as amarok does not intend to play photos
    static QStringList mimeTable = Phonon::BackendCapabilities::availableMimeTypes().filter( "audio/", Qt::CaseInsensitive ) +
                                   Phonon::BackendCapabilities::availableMimeTypes().filter( "video/", Qt::CaseInsensitive );

    const QString mimeType = item.mimetype();
    const bool valid = mimeTable.contains( mimeType, Qt::CaseInsensitive );

    //we special case this as otherwise users hate us
    if ( !valid && ( mimeType == "audio/mp3" || mimeType == "audio/x-mp3" ) && !installDistroCodec() )
        The::statusBar()->longMessage(
                i18n( "<p>Phonon claims it <b>cannot</b> play MP3 files. You may want to examine "
                      "the installation of the backend that phonon uses.</p>"
                      "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok Handbook</i>.</p>" ), StatusBar::Error );

    return valid;
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
        playbackEnded( trackPosition(), m_currentTrack->length(), EngineObserver::EndedQuit );
        emit trackChanged( Meta::TrackPtr( 0 ) );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::play() //SLOT
{
    DEBUG_BLOCK

    if( m_media->state() == Phonon::PlayingState )
        return;

    if( m_fader )
        m_fader->deleteLater();

    if ( m_media->state() == Phonon::PausedState )
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
    delete m_multi;
    m_multi = m_currentTrack->as<Meta::MultiPlayableCapability>();

    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();

    m_currentTrack->prepareToPlay();

    if( m_multi )
    {
        m_media->stop();
        connect( m_multi, SIGNAL( playableUrlFetched( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        m_multi->fetchFirst();
    }
    else
    {
        playUrl( m_currentTrack->playableUrl(), offset );
    }
}

void
EngineController::playUrl( const KUrl &url, uint offset )
{
    DEBUG_BLOCK

    slotStopFadeout();

    debug() << "URL: " << url.url();
    m_media->setCurrentSource( url );

    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();

    if( offset )
    {
        m_media->pause();
        m_media->seek( offset );
    }
    m_media->play();
}

void
EngineController::pause() //SLOT
{
    m_media->pause();
}

void
EngineController::stop( bool forceInstant ) //SLOT
{
    DEBUG_BLOCK

    // need to get a new instance of multi if played again
    delete m_multi;

    m_mutex.lock();
    m_nextTrack.clear();
    m_nextUrl.clear();
    m_media->clearQueue();
    m_mutex.unlock();

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack )
    {
        debug() << "m_currentTrack != 0";
        const int pos = trackPosition();
        const int length = m_currentTrack->length();
        m_currentTrack->finishedPlaying( double(pos)/double(length) );
        playbackEnded( pos, length, EngineObserver::EndedStopped );
        emit trackChanged( Meta::TrackPtr( 0 ) );
    }

    // Stop instantly if fadeout is already running, or the media is paused (i.e. pressing Stop twice)
    if( m_fader || m_media->state() == Phonon::PausedState )
    {
        forceInstant = true;
    }

    if( AmarokConfig::fadeout() && AmarokConfig::fadeoutLength() && !forceInstant )
    {
        stateChangedNotify( Phonon::StoppedState, Phonon::PlayingState ); //immediately disable Stop action

        m_fader = new Phonon::VolumeFaderEffect( this );
        m_path.insertEffect( m_fader );
        m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );
        m_fader->fadeOut( AmarokConfig::fadeoutLength() );

        m_fadeoutTimer->start( AmarokConfig::fadeoutLength() + 1000 ); //add 1s for good measure, otherwise seems to cut off early (buffering..)
    }
    else
        m_media->stop();

    emit trackFinished();
    m_currentTrack = 0;
}

bool
EngineController::isPaused() const
{
    return m_media->state() == Phonon::PausedState;
}

void
EngineController::playPause() //SLOT
{
    DEBUG_BLOCK

    //this is used by the TrayIcon, PlayPauseAction and DBus
    debug() << "PlayPause: phonon state" << m_media->state();

    if( m_media->state() == Phonon::PausedState ||
        m_media->state() == Phonon::StoppedState ||
        m_media->state() == Phonon::LoadingState )
        play();
    else
        pause();
}

void
EngineController::seek( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( m_media->isSeekable() )
    {
        m_media->seek( static_cast<qint64>( ms ) );
        trackPositionChangedNotify( ms, true ); /* User seek */
        emit trackSeeked( ms );
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
    if( percent < 0 ) percent = 0;
    if( percent > 100 ) percent = 100;

    // If we're explicitly setting the volume, then I think it's safe
    // to assume that we don't want the audio stream muted
    m_audio->setMuted( false );

    qreal newVolume = percent / 100.0; //Phonon's volume is 0.0 - 1.0
    m_audio->setVolume( newVolume );
    AmarokConfig::setMasterVolume( percent );
    volumeChangedNotify( percent );

    emit volumeChanged( percent );
    return percent;
}

int
EngineController::volume() const
{
    return static_cast<int>( m_audio->volume() * 100.0 );
}

bool
EngineController::isMuted() const
{
    return m_audio->isMuted();
}

void
EngineController::mute() //SLOT
{
    // if it's already muted then we restore to previous value
    int newPercent = m_audio->isMuted() ? volume() : 0;

    m_audio->setMuted( !isMuted() ); // toggle mute

    AmarokConfig::setMasterVolume( newPercent );
    volumeChangedNotify( newPercent );
    emit volumeChanged( newPercent );
}

Meta::TrackPtr
EngineController::currentTrack() const
{
    return m_currentTrack;
}

int
EngineController::trackLength() const
{
    const qint64 phononLength = m_media->totalTime(); //may return -1
    if( phononLength <= 0 && m_currentTrack ) //this is useful for stuff like last.fm streams
        return m_currentTrack->length();      //where Meta::Track knows the length, but phonon doesn't
    else
        return static_cast<int>( phononLength / 1000 );
}

void
EngineController::setNextTrack( Meta::TrackPtr track )
{
    QMutexLocker locker( &m_mutex );

    if( !track )
        return;

    track->prepareToPlay();
    if( track->playableUrl().isEmpty() )
        return;

    if( m_media->state() == Phonon::PlayingState ||
        m_media->state() == Phonon::BufferingState )
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


bool
EngineController::getAudioCDContents(const QString &device, KUrl::List &urls)
{
    Q_UNUSED( device ); Q_UNUSED( urls );
//since Phonon doesn't know anything about CD listings, there's actually no reason for this functionality to be here
//kept to keep things compiling, probably should be in its own class.
    return false;
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

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::slotTick( qint64 position )
{
    trackPositionChangedNotify( static_cast<long>( position ), false ); //it expects milliseconds
}


void
EngineController::slotAboutToFinish()
{
    DEBUG_BLOCK
    debug() << "Track finished completely, updating statistics";
    m_currentTrack->finishedPlaying( 1.0 ); // If we reach aboutToFinish, the track is done as far as we are concerned.
    if( m_multi )
    {
        DEBUG_LINE_INFO
        m_mutex.lock();
        m_playWhenFetched = false;
        m_mutex.unlock();
        m_multi->fetchNext();
        debug() << "The queue has: " << m_media->queue().size() << " tracks in it";
    }

    else if( m_media->queue().isEmpty() )
        The::playlistActions()->requestNextTrack();
}

void
EngineController::slotQueueEnded()
{
    DEBUG_BLOCK

    if( m_currentTrack && !m_multi )
    {
        m_currentTrack->finishedPlaying( 1.0 );
        emit trackFinished();
        playbackEnded( trackPosition(), m_currentTrack->length(), EngineObserver::EndedStopped );
        m_currentTrack = 0;
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
    Q_UNUSED( source );

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
    }
    else
        m_preamp->setVolumeDecibel( 0.0 );

    // state never changes if tracks are queued, but we need this to update the caption
    stateChangedNotify( m_media->state(), m_media->state() );

    emit trackChanged( m_currentTrack );
    newTrackPlaying();
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    DEBUG_BLOCK
    // Sanity checks
    if( newState == oldState || newState == Phonon::BufferingState )
        return;

    if( newState == Phonon::ErrorState )  // If media is borked, skip to next track
    {
        warning() << "Phonon failed to play this URL. Error: " << m_media->errorString();
        if( m_multi )
        {
            DEBUG_LINE_INFO
            m_mutex.lock();
            m_playWhenFetched = false;
            m_mutex.unlock();
            m_multi->fetchNext();
            debug() << "The queue has: " << m_media->queue().size() << " tracks in it";
        }

        else if( m_media->queue().isEmpty() )
            The::playlistActions()->requestNextTrack();
    }

    stateChangedNotify( newState, oldState );

    if( newState == Phonon::PlayingState )
        emit trackPlayPause( Playing );
    else if( newState == Phonon::PausedState )
        emit trackPlayPause( Paused );
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
    if( milliseconds != 0 ) //don't notify for 0 seconds, it's probably just a stream
        trackLengthChangedNotify( static_cast<long>( milliseconds ) / 1000 );
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

    // Make sure the timer won't call this method again
    m_fadeoutTimer->stop();

    if ( m_fader ) {
        m_fader->deleteLater();
        m_media->stop();
    }
}

#include "EngineController.moc"

