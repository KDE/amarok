/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *             (C) 2004, 2005 Mark Kretschmann <kretschmann@kde.org>       *
 *             (C) 2006, 2008 Ian Monroe <ian@monroe.nu>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "controller"

#include "enginecontroller.h"

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
#include "pluginmanager.h"
#include "TheInstances.h"

#include <KApplication>
#include <KFileItem>
#include <KIO/Job>
#include <KMessageBox>
#include <KRun>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>

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

    m_media->setTickInterval( 250 ); //small tick interval to make progress slider smoother
    PERF_LOG( "EngineController: loaded phonon objects" )

    connect( m_media, SIGNAL( finished() ), SLOT( slotTrackEnded() ) );
    connect( m_media, SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );
    connect( m_media, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( slotStateChanged() ) );
    connect( m_media, SIGNAL( tick( qint64 ) ), SLOT( slotTick( qint64 ) ) );
    connect( m_media, SIGNAL( totalTimeChanged( qint64 ) ), SLOT( slotTrackLengthChanged( qint64 ) ) );
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
    if ( m_media->state() == Phonon::PausedState )
    {
        m_media->play();
    }
    else emit orderCurrent();
}

void
EngineController::play( const Meta::TrackPtr& track, uint offset )
{
    DEBUG_BLOCK

    delete m_multi;
    m_multi = 0;
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
    m_stream = ( url.protocol() == "http" || url.protocol() == "rtsp" );
    m_media->setCurrentSource( url );
    m_media->pause();
    m_media->seek( offset );
    m_media->play();
    if( m_media->state() != Phonon::ErrorState )
        newTrackPlaying();
}

void
EngineController::pause() //SLOT
{
    m_media->pause();
}

void
EngineController::stop() //SLOT
{
    // will need to get a new instance of multi if played again
    delete m_multi;
    m_multi = 0;

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack )
        trackEnded( trackPosition(), m_currentTrack->length() * 1000, "stop" );

    //Remove requirement for track to be loaded for stop to be called (fixes gltiches
    //where stop never properly happens if call to m_engine->load fails in play)
    //if ( m_engine->loaded() )
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
    return static_cast<int>( m_media->totalTime() / 1000 );
}

Engine::State
EngineController::state() const
{
    Engine::State state;

    switch( m_media->state() )
    {
        case Phonon::PlayingState:
            state = Engine::Playing;
            break;

        case Phonon::PausedState:
            state = Engine::Paused;
            break;

        case Phonon::StoppedState:
        // fallthrough

        case Phonon::BufferingState:
        // fallthrough

        case Phonon::ErrorState:
        // fallthrough
        
        case Phonon::LoadingState:
            state = m_currentTrack && m_currentTrack->isPlayable() ? Engine::Empty : Engine::Idle;
    }

    return state;
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
    return m_stream;
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
EngineController::slotTrackEnded() //SLOT
{
    DEBUG_BLOCK
/*    if ( AmarokConfig::trackDelayLength() > 0 )
    {
        //FIXME not perfect
        if ( !m_isTiming )
        {
            QTimer::singleShot( AmarokConfig::trackDelayLength(), this, SLOT(trackDone()) );
            m_isTiming = true;
        }

    }
    else */
        trackDone();
}

void
EngineController::slotStateChanged() //SLOT
{
    DEBUG_BLOCK
    stateChangedNotify( state() );
}

void
EngineController::trackDone()
{
    emit trackFinished();
    if( m_multi )
        m_multi->fetchNext();
    else
        emit orderNext( false );
}

void
EngineController::slotPlayableUrlFetched( const KUrl &url )
{
    if( url.isEmpty() )
    {
        emit orderNext( false );
    }
    else
    {
        playUrl( url, 0 );
    }
}

void
EngineController::slotTrackLengthChanged( qint64 milliseconds )
{
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

EngineController*
The::engineController()
{
    return EngineController::instance(); //port amarok to the The:: style...
}

#include "enginecontroller.moc"
