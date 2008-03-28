/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *             (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2006, 2008 Ian Monroe <ian@monroe.nu>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "EngineController"

#include "EngineController.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "collection/CollectionManager.h"
#include "ContextStatusBar.h"
#include "debug.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "meta/MultiPlayableCapability.h"
#include "playlist/PlaylistModel.h"
#include "PluginManager.h"
#include "TheInstances.h"

#include <KApplication>
#include <KFileItem>
#include <KIO/Job>
#include <KMessageBox>
#include <KRun>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QByteArray>
#include <QFile>
#include <QTimer>


EngineController::ExtensionCache EngineController::s_extensionCache;

EngineController*
EngineController::instance()
{
    //will only be instantiated the first time this function is called
    //will work with the inline directive
    static EngineController Instance;

    return &Instance;
}

EngineController::EngineController()
    : m_media( 0 )
    , m_audio( 0 )
{
    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new Phonon::MediaObject( this );
    m_audio = new Phonon::AudioOutput( Phonon::MusicCategory, this );

    m_path = Phonon::createPath(m_media, m_audio);

    m_media->setTickInterval( 1000 );
    PERF_LOG( "EngineController: loaded phonon objects" )

    connect( m_media, SIGNAL( finished() ), SLOT( slotTrackEnded() ) );
    connect( m_media, SIGNAL( aboutToFinish() ), SLOT( slotAboutToFinish() ) );
    connect( m_media, SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );
    connect( m_media, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
                      SLOT( slotStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_media, SIGNAL( tick( qint64 ) ), SLOT( slotTick( qint64 ) ) );
    connect( m_media, SIGNAL( totalTimeChanged( qint64 ) ), SLOT( slotTrackLengthChanged( qint64 ) ) );
    connect( m_media, SIGNAL( currentSourceChanged( const Phonon::MediaSource & ) ),
                       SLOT( slotNewTrackPlaying( const Phonon::MediaSource & ) ) );
}

EngineController::~EngineController()
{
    DEBUG_FUNC_INFO //we like to know when singletons are destroyed
    delete m_media;
    delete m_audio;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

bool
EngineController::canDecode( const KUrl &url ) //static
{
   //NOTE this function must be thread-safe

    const QString fileName = url.fileName();
    const QString ext = Amarok::extension( fileName );

    //Port 2.0
//     if ( PlaylistFile::isPlaylistFile( fileName ) ) return false;

    // Ignore protocols "fetchcover" and "musicbrainz", they're not local but we don't really want them in the playlist :)
    if ( url.protocol() == "fetchcover" || url.protocol() == "musicbrainz" ) return false;

    // Accept non-local files, since we can't test them for validity at this point
    // TODO actually, only accept unconditionally http stuff
    // TODO this actually makes things like "Blarrghgjhjh:!!!" automatically get inserted
    // into the playlist
    // TODO remove for Amarok 1.3 and above silly checks, instead check for http type servers
    if ( !url.isLocalFile() ) return true;

    // If extension is already in the cache, return cache result
    if ( extensionCache().contains( ext ) )
        return s_extensionCache[ext];

    // If file has 0 bytes, ignore it and return false, not to infect the cache with corrupt files.
    // TODO also ignore files that are too small?
    if ( !QFileInfo(url.path()).size() )
        return false;

    bool valid;
    {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url );
        valid = Phonon::BackendCapabilities::isMimeTypeAvailable( item.mimetype() );
    }

    //we special case this as otherwise users hate us
    if ( !valid && ext.toLower() == "mp3" && !installDistroCodec() )
        Amarok::ContextStatusBar::instance()->longMessageThreadSafe(
                i18n( "<p>The %1 claims it <b>cannot</b> play MP3 files."
                    "<p>You may want to choose a different engine from the <i>Configure Dialog</i>, or examine "
                    "the installation of the multimedia-framework that the current engine uses. "
                    "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok HandBook</i>.", AmarokConfig::soundSystem() ), KDE::StatusBar::Error );

    // Cache this result for the next lookup
    if ( !ext.isEmpty() )
        extensionCache().insert( ext, valid );
    return valid;
}

bool
EngineController::installDistroCodec()
{
    KService::List services = KServiceTypeTrader::self()->query( "Amarok/CodecInstall"
        , QString("[X-KDE-Amarok-codec] == 'mp3' and [X-KDE-Amarok-engine] == 'phonon-%1'").arg("xine") );
    //todo - figure out how to query Phonon for the current backend loaded
    if( !services.isEmpty() )
    {
        KService::Ptr service = services.first(); //list is not empty
        QString installScript = service->exec();
        if( !installScript.isNull() ) //just a sanity check
        {
            KGuiItem installButton("Install MP3 Support");
            if(KMessageBox::questionYesNo(MainWindow::self()
            , i18n("Amarok currently cannot play MP3 files.")
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

    if( !AmarokConfig::resumeTrack().isEmpty() )
    {
        const KUrl url = AmarokConfig::resumeTrack();
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
            play( track, AmarokConfig::resumeTime() );
    }
}

void
EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() && m_currentTrack )
    {
        trackEnded( trackPosition(), m_currentTrack->length() * 1000, "quit" );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::play() //SLOT
{
    DEBUG_BLOCK
    if( m_fader )
        m_fader->deleteLater();

    if ( m_media->state() == Phonon::PausedState )
    {
        m_media->play();
    }
    else play( The::playlistModel()->activeTrack() );
}

void
EngineController::play( const Meta::TrackPtr& track, uint offset )
{
    DEBUG_BLOCK

    if( !track ) // Guard
        return;

    delete m_multi;
    m_currentTrack = track;
    m_multi = m_currentTrack->as<Meta::MultiPlayableCapability>();

    if( m_multi )
    {
        connect( m_multi, SIGNAL( playableUrlFetched( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        m_multi->fetchFirst();
    }
    else
    {
        playUrl( track->playableUrl(), offset );
    }
}

void
EngineController::playUrl( const KUrl &url, uint offset )
{
    DEBUG_BLOCK
    m_isStream = ( url.protocol() == "http" || url.protocol() == "rtsp" );
    if( m_media->state() == Phonon::PlayingState )  //TODO: This should handle crossfading at some point.
        stop( true /*Don't fade out*/ );
    m_media->setCurrentSource( url );
    if( offset != 0 )
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

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack )
        trackEnded( trackPosition(), m_currentTrack->length() * 1000, "stop" );

    if( m_fader )
        m_fader->deleteLater();

    if( AmarokConfig::fadeoutLength() && !forceInstant ) {
        stateChangedNotify( Phonon::StoppedState, Phonon::PlayingState ); //immediately disable Stop action

        m_fader = new Phonon::VolumeFaderEffect( this );
        m_path.insertEffect( m_fader );
        m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );
        m_fader->fadeOut( AmarokConfig::fadeoutLength() );

        QTimer::singleShot( AmarokConfig::fadeoutLength() + 1000, this, SLOT( slotReallyStop() ) ); //add 1s for good measure, otherwise seems to cut off early (buffering..)
    }
    else
        m_media->stop();
}

void
EngineController::playPause() //SLOT
{
    //this is used by the TrayIcon, PlayPauseAction and DCOP

    if( m_media->state() == Phonon::PlayingState )
    {
        pause();
    }
    else
        play();
}

void
EngineController::seek( int ms ) //SLOT
{
    if( m_media->isSeekable() )
    {
        m_media->seek( static_cast<qint64>( ms ) );
        trackPositionChangedNotify( ms, true ); /* User seek */
    }
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

    qreal newVolume = percent / 100.0; //Phonon's volume is 0.0 - 1.0
    m_audio->setVolume( newVolume );
    AmarokConfig::setMasterVolume( percent );
    volumeChangedNotify( percent );

    return percent;
}

int
EngineController::volume() const
{
    return static_cast<int>( m_audio->volume() * 100.0 );
}

void
EngineController::mute() //SLOT
{
    m_audio->setMuted( !m_audio->isMuted() );
}

Meta::TrackPtr
EngineController::currentTrack() const
{
    Phonon::State state = m_media->state();
    return state == Phonon::ErrorState ? Meta::TrackPtr() : m_currentTrack;
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

bool
EngineController::getAudioCDContents(const QString &device, KUrl::List &urls)
{
//since Phonon doesn't know anything about CD listings, there's actually no reason for this functionality to be here
//kept to keep things compiling, probably should be in its own class.
    return false;
}

bool
EngineController::isStream()
{
    return m_isStream;
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
    if( m_multi )
    {
        m_multi->fetchNext();
    }
    else
    {
        trackEnded( m_media->currentTime(), m_media->totalTime(), i18n( "Previous track finished" ) );
        slotTrackEnded(); // Phonon does not alert us that a track has finished if there is another source in the queue.
        m_currentTrack = The::playlistModel()->nextTrack();
        if( m_currentTrack )
            m_media->enqueue( m_currentTrack->playableUrl() );
    }
}

void
EngineController::slotTrackEnded()
{
    DEBUG_BLOCK
    if( m_currentTrack ) // I have no idea why this can be null, but apparently it can..
    {
        m_currentTrack->finishedPlaying( 1.0 );
        emit trackFinished();
    }
}

void
EngineController::slotNewTrackPlaying( const Phonon::MediaSource &source )
{
    DEBUG_BLOCK
    Q_UNUSED( source );
    newTrackPlaying();
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    // Sanity checks
    if( newState == oldState )
        return;
    if( newState == Phonon::BufferingState ) //Ignore this for now, it's causing trouble;
        return;
    stateChangedNotify( newState, oldState );
}

void
EngineController::slotPlayableUrlFetched( const KUrl &url )
{
    if( url.isEmpty() )
    {
        play( The::playlistModel()->nextTrack() );
    }
    else
    {
        playUrl( url, 0 );
    }
}

void
EngineController::slotTrackLengthChanged( qint64 milliseconds )
{
    if( milliseconds != 0 ) //don't notify for 0 seconds, it's probably just a stream
        trackLengthChangedNotify( static_cast<long>( milliseconds ) / 1000 );
}

void
EngineController::slotMetaDataChanged()
{
    QHash<qint64, QString> meta;
    {
        QStringList data = m_media->metaData( "ARTIST" );
        if( !data.isEmpty() )
            meta.insert( Meta::valArtist, data.first() );
    }
    {
        QStringList data = m_media->metaData( "ALBUM" );
        if( !data.isEmpty() )
            meta.insert( Meta::valAlbum, data.first() );
    }
    {
        QStringList data = m_media->metaData( "TITLE" );
        if( !data.isEmpty() )
            meta.insert( Meta::valTitle, data.first() );
    }
    {
        QStringList data = m_media->metaData( "GENRE" );
        if( !data.isEmpty() )
            meta.insert( Meta::valGenre, data.first() );
    }
    {
        QStringList data = m_media->metaData( "TRACKNUMBER" );
        if( !data.isEmpty() )
            meta.insert( Meta::valTrackNr, data.first() );
    }
    {
        QStringList data = m_media->metaData( "LENGTH" );
        if( !data.isEmpty() )
            meta.insert( Meta::valLength, data.first() );
    }
    bool trackChanged = false;
    if( m_lastTrack != m_currentTrack )
    {
        trackChanged = true;
        m_lastTrack = m_currentTrack;
    }
    newMetaDataNotify( meta, trackChanged);
}

void
EngineController::slotReallyStop() //SLOT
{
    DEBUG_BLOCK

    if ( m_fader ) {
        m_fader->deleteLater();
        m_media->stop();
    }
}

EngineController*
The::engineController()
{
    return EngineController::instance(); //port amarok to the The:: style...
}

#include "EngineController.moc"

