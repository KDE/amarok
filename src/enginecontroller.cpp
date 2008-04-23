/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *             (C) 2004,5 Mark Kretschmann                                 *
 *             (C) 2006 Ian Monroe                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "controller"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "lastfm.h"
#include "mediabrowser.h"
#include "playlist.h"
#include "playlistloader.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <qfile.h>
#include <qobjectlist.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kfileitem.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <krun.h>

#include <cstdlib>


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
        : m_engine( 0 )
        , m_voidEngine( 0 )
        , m_delayTime( 0 )
        , m_muteVolume( 0 )
        , m_xFadeThisTrack( false )
        , m_timer( new QTimer( this ) )
        , m_playFailureCount( 0 )
        , m_lastFm( false )
        , m_positionOffset( 0 )
        , m_lastPositionOffset( 0 )
{
    m_voidEngine = m_engine = loadEngine( "void-engine" );

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotMainTimer() ) );
}

EngineController::~EngineController()
{
    DEBUG_FUNC_INFO //we like to know when singletons are destroyed
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

EngineBase*
EngineController::loadEngine() //static
{
    /// always returns a valid pointer to EngineBase

    DEBUG_BLOCK
    //TODO remember song position, and resume playback

    // new engine, new ext cache required
    extensionCache().clear();

    if( m_engine != m_voidEngine ) {
        EngineBase *oldEngine = m_engine;

        // we assign this first for thread-safety,
        // EngineController::engine() must always return an engine!
        m_engine = m_voidEngine;

        // we unload the old engine first because there are a number of
        // bugs associated with keeping one engine loaded while loading
        // another, eg xine-engine can't init(), and aRts-engine crashes
        PluginManager::unload( oldEngine );

        // the engine is not required to do this when we unload it but
        // we need to do it to ensure Amarok looks correct.
        // We don't do this for the void-engine because that
        // means Amarok sets all components to empty on startup, which is
        // their responsibility.
        slotStateChanged( Engine::Empty );
    }

    m_engine = loadEngine( AmarokConfig::soundSystem() );

    const QString engineName = PluginManager::getService( m_engine )->property( "X-KDE-Amarok-name" ).toString();

    if( !AmarokConfig::soundSystem().isEmpty() && engineName != AmarokConfig::soundSystem() ) {
        //AmarokConfig::soundSystem() is empty on the first-ever-run

        Amarok::StatusBar::instance()->longMessage( i18n(
                "Sorry, the '%1' could not be loaded, instead we have loaded the '%2'." )
                        .arg( AmarokConfig::soundSystem() )
                        .arg( engineName ),
                KDE::StatusBar::Sorry );

        AmarokConfig::setSoundSystem( engineName );
    }

    // Important: Make sure soundSystem is not empty
    if( AmarokConfig::soundSystem().isEmpty() )
        AmarokConfig::setSoundSystem( engineName );

    return m_engine;
}

#include <qvaluevector.h>
EngineBase*
EngineController::loadEngine( const QString &engineName )
{
    /// always returns a valid plugin (exits if it can't get one)

    DEBUG_BLOCK

    QString query = "[X-KDE-Amarok-plugintype] == 'engine' and [X-KDE-Amarok-name] != '%1'";
    KTrader::OfferList offers = PluginManager::query( query.arg( engineName ) );

    // sort by rank, QValueList::operator[] is O(n), so this is quite inefficient
    #define rank( x ) (x)->property( "X-KDE-Amarok-rank" ).toInt()
    for( int n = offers.count()-1, i = 0; i < n; i++ )
        for( int j = n; j > i; j-- )
            if( rank( offers[j] ) > rank( offers[j-1] ) )
                qSwap( offers[j], offers[j-1] );
    #undef rank

    // this is the actual engine we want
    query = "[X-KDE-Amarok-plugintype] == 'engine' and [X-KDE-Amarok-name] == '%1'";
    offers = PluginManager::query( query.arg( engineName ) ) + offers;

    foreachType( KTrader::OfferList, offers ) {
        Amarok::Plugin *plugin = PluginManager::createFromService( *it );

        if( plugin ) {
            QObject *bar = Amarok::StatusBar::instance();
            EngineBase *engine = static_cast<EngineBase*>( plugin );

            connect( engine, SIGNAL(stateChanged( Engine::State )),
                       this,   SLOT(slotStateChanged( Engine::State )) );
            connect( engine, SIGNAL(trackEnded()),
                       this,   SLOT(slotTrackEnded()) );
            if( bar )
            {
                connect( engine, SIGNAL(statusText( const QString& )),
                            bar,   SLOT(shortMessage( const QString& )) );
                connect( engine, SIGNAL(infoMessage( const QString& )),
                            bar,   SLOT(longMessage( const QString& )) );
            }
            connect( engine, SIGNAL(metaData( const Engine::SimpleMetaBundle& )),
                       this,   SLOT(slotEngineMetaData( const Engine::SimpleMetaBundle& )) );
            connect( engine, SIGNAL(showConfigDialog( const QCString& )),
                       kapp,   SLOT(slotConfigAmarok( const QCString& )) );

            if( engine->init() )
                return engine;
            else
                warning() << "Could not init() an engine\n";
        }
    }

    KRun::runCommand( "kbuildsycoca" );

    KMessageBox::error( 0, i18n(
            "<p>Amarok could not find any sound-engine plugins. "
            "Amarok is now updating the KDE configuration database. Please wait a couple of minutes, then restart Amarok.</p>"
            "<p>If this does not help, "
            "it is likely that Amarok is installed under the wrong prefix, please fix your installation using:<pre>"
            "$ cd /path/to/amarok/source-code/<br>"
            "$ su -c \"make uninstall\"<br>"
            "$ ./configure --prefix=`kde-config --prefix` && su -c \"make install\"<br>"
            "$ kbuildsycoca<br>"
            "$ amarok</pre>"
            "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net.</p>" ) );

    // don't use QApplication::exit, as the eventloop may not have started yet
    std::exit( EXIT_SUCCESS );

    // Not executed, just here to prevent compiler warning
    return 0;
}


bool EngineController::canDecode( const KURL &url ) //static
{
   //NOTE this function must be thread-safe
    //TODO a KFileItem version? <- presumably so we can mimetype check

    const QString fileName = url.fileName();
    const QString ext = Amarok::extension( fileName );

    if ( PlaylistFile::isPlaylistFile( fileName ) ) return false;

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
    KFileItem f( KFileItem::Unknown, KFileItem::Unknown, url, false );
    if ( !f.size() )
        return false;

    const bool valid = engine()->canDecode( url );

    if( engine() != EngineController::instance()->m_voidEngine )
    {
        //we special case this as otherwise users hate us
        if ( !valid && ext.lower() == "mp3"){
            QCustomEvent * e = new QCustomEvent( 2000 );
            QApplication::postEvent( Amarok::StatusBar::instance(), e );
	}

        // Cache this result for the next lookup
        if ( !ext.isEmpty() )
            extensionCache().insert( ext, valid );
    }

    return valid;
}

void EngineController::unplayableNotification() {

    if( !installDistroCodec(AmarokConfig::soundSystem()))
        Amarok::StatusBar::instance()->longMessageThreadSafe(
                  i18n( "<p>The %1 claims it <b>cannot</b> play MP3 files."
                        "<p>You may want to choose a different engine from the <i>Configure Dialog</i>, or examine "
                        "the installation of the multimedia-framework that the current engine uses. "
                        "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok HandBook</i>." )
                    .arg( AmarokConfig::soundSystem() ), KDE::StatusBar::Error );
}

bool EngineController::installDistroCodec( const QString& engine /*Filetype type*/)
{
    KService::Ptr service = KTrader::self()->query( "Amarok/CodecInstall"
        , QString("[X-KDE-Amarok-codec] == 'mp3' and [X-KDE-Amarok-engine] == '%1'").arg(engine) ).first();
    if( service )
    {
        QString installScript = service->exec();
        if( !installScript.isNull() ) //just a sanity check
        {
            KGuiItem installButton( i18n( "Install MP3 Support" ) );
            if(KMessageBox::questionYesNo(PlaylistWindow::self()
            , i18n("Amarok currently cannot play MP3 files.")
            , i18n( "No MP3 Support" )
            , installButton
            , KStdGuiItem::no()
            , "codecInstallWarning" ) == KMessageBox::Yes )
            {
                    KRun::runCommand(installScript);
                    return true;
            }
        }
    }
return false;
}

void EngineController::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if( !AmarokConfig::resumeTrack().isEmpty() )
    {
        const KURL url = AmarokConfig::resumeTrack();

        play( MetaBundle( url ), AmarokConfig::resumeTime() );
    }
}


void EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() )
    {
        trackEnded( trackPosition(), m_bundle.length() * 1000, "quit" );
    }

    PluginManager::unload( m_voidEngine );
    m_voidEngine = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void EngineController::previous() //SLOT
{
    emit orderPrevious();
}


void EngineController::next( bool forceNext ) //SLOT
{
    m_previousUrl = m_bundle.url();
    m_isTiming = false;
    emit orderNext(forceNext);
}


void EngineController::play() //SLOT
{
    if ( m_engine->state() == Engine::Paused )
    {
        m_engine->unpause();
    }
    else emit orderCurrent();
}


void EngineController::play( const MetaBundle &bundle, uint offset )
{
    DEBUG_BLOCK

    KURL url = bundle.url();
    // don't destroy connection if we need to change station
    if( url.protocol() != "lastfm" && LastFm::Controller::instance()->isPlaying() )
    {
        m_engine->stop();
        LastFm::Controller::instance()->playbackStopped();
    }
    m_lastFm = false;
    //Holds the time since we started trying to play non-existent files
    //so we know when to abort
    static QTime failure_time;
    if ( !m_playFailureCount )
        failure_time.start();

    debug() << "Loading URL: " << url.url() << endl;
    m_lastMetadata.clear();

    //TODO bummer why'd I do it this way? it should _not_ be in play!
    //let Amarok know that the previous track is no longer playing
    if ( m_timer->isActive() )
        trackEnded( trackPosition(), m_bundle.length() * 1000, "change" );

    if ( url.isLocalFile() ) {
        // does the file really exist? the playlist entry might be old
        if ( ! QFile::exists( url.path()) ) {
            //debug() << "  file >" << url.path() << "< does not exist!" << endl;
            Amarok::StatusBar::instance()->shortMessage( i18n("Local file does not exist.") );
            goto some_kind_of_failure;
        }
    }
    else
    {
        if( url.protocol() == "cdda" )
            Amarok::StatusBar::instance()->shortMessage( i18n("Starting CD Audio track...") );
        else
            Amarok::StatusBar::instance()->shortMessage( i18n("Connecting to stream source...") );
        debug() << "Connecting to protocol: " << url.protocol() << endl;
    }

    // WebDAV protocol is HTTP with extensions (and the "webdav" scheme
    // is a KDE-ism anyway). Most engines cope with HTTP streaming, but
    // not through KIO, so they don't support KDE-isms.
    if ( url.protocol() == "webdav" )
        url.setProtocol( "http" );
    else if ( url.protocol() == "webdavs" )
        url.setProtocol( "https" );

    // streams from last.fm should be handled by our proxy, in order to authenticate with the server
    else if ( url.protocol() == "lastfm" )
    {
        if( LastFm::Controller::instance()->isPlaying() )
        {
            LastFm::Controller::instance()->getService()->changeStation( url.url() );
            connect( m_engine, SIGNAL( lastFmTrackChange() ), LastFm::Controller::instance()->getService()
            , SLOT( requestMetaData() ) );
            connect( LastFm::Controller::instance()->getService(), SIGNAL( metaDataResult( const MetaBundle& ) ),
                     this, SLOT( slotStreamMetaData( const MetaBundle& ) ) );
            return;
        }
        else
        {
            url = LastFm::Controller::instance()->getNewProxy( url.url(), m_engine->lastFmProxyRequired() );
            if( url.isEmpty() ) goto some_kind_of_failure;
            m_lastFm = true;
            connect( m_engine, SIGNAL( lastFmTrackChange() ), LastFm::Controller::instance()->getService()
                    , SLOT( requestMetaData() ) );
            connect( LastFm::Controller::instance()->getService(), SIGNAL( metaDataResult( const MetaBundle& ) ),
                    this, SLOT( slotStreamMetaData( const MetaBundle& ) ) );
        }
        debug() << "New URL is " << url.url() << endl;
    }
    else if (url.protocol() == "daap" )
    {
        KURL newUrl = MediaBrowser::instance()->getProxyUrl( url );
        if( !newUrl.isEmpty() )
        {
            debug() << newUrl << endl;
            url = newUrl;
        }
        else
            return;
    }

    if( m_engine->load( url, url.protocol() == "http" || url.protocol() == "rtsp" ) )
    {
        //assign bundle now so that it is available when the engine
        //emits stateChanged( Playing )
        if( !m_bundle.url().path().isEmpty() ) //wasn't playing before
            m_previousUrl = m_bundle.url();
        else
            m_previousUrl = bundle.url();
        m_bundle = bundle;

        if( m_engine->play( offset ) )
        {
            //Reset failure count as we are now successfully playing a song
            m_playFailureCount = 0;

            // Ask engine for track length, if available. It's more reliable than TagLib.
            const uint trackLength = m_engine->length() / 1000;
            if ( trackLength ) m_bundle.setLength( trackLength );

            m_xFadeThisTrack = !m_engine->isStream() && !(url.protocol() == "cdda") &&
                    m_bundle.length()*1000 - offset - AmarokConfig::crossfadeLength()*2 > 0;

            newMetaDataNotify( m_bundle, true /* track change */ );
            return;
        }
    }

    some_kind_of_failure:
        debug() << "Failed to play this track." << endl;

        ++m_playFailureCount;

        //Code to skip to next track if playback fails:
        //
        //* The failure counter is reset if a track plays successfully or if playback is
        //  stopped, for whatever reason.
        //* For normal playback, the attempt to play is stopped at the end of the playlist
        //* For repeat playlist , a whole playlist worth of songs is tried
        //* For repeat album, the number of songs tried is the number of tracks from the
        //  album that are in the playlist.
        //* For repeat track, no attempts are made
        //* For the nmm engine, no attempts are made (necessary? / FIXME)
        //* To prevent GUI freezes we don't try to play again after 0.5s of failure
        int totalTracks = Playlist::instance()->totalTrackCount();
        int currentTrack = Playlist::instance()->currentTrackIndex();
        if ( ( ( Amarok::repeatPlaylist() && static_cast<int>(m_playFailureCount) < totalTracks )
            || ( Amarok::repeatNone() && currentTrack != totalTracks - 1 )
            || ( Amarok::repeatAlbum() && m_playFailureCount < Playlist::instance()->repeatAlbumTrackCount() ) )
            && AmarokConfig::soundSystem() != "nmm-engine"
            && failure_time.elapsed() < 500 )
        {

           debug() << "Skipping to next track." << endl;

           // The test for loaded must be done _before_ next is called
           if ( !m_engine->loaded() )
           {
               //False gives behaviour as if track played successfully
               next( false );
               QTimer::singleShot( 0, this, SLOT(play()) );
           }
           else
           {
               //False gives behaviour as if track played successfully
               next( false );
           }
        }
        else
        {
            //Stop playback, including resetting failure count (as all new failures are
            //treated as independent after playback is stopped)
            stop();
        }
}


void EngineController::pause() //SLOT
{
    if ( m_engine->loaded() && !LastFm::Controller::instance()->isPlaying() )
        m_engine->pause();
}


void EngineController::stop() //SLOT
{
    //Reset failure counter as after stop, everything else is unrelated
    m_playFailureCount = 0;

    //let Amarok know that the previous track is no longer playing
    trackEnded( trackPosition(), m_bundle.length() * 1000, "stop" );

    //Remove requirement for track to be loaded for stop to be called (fixes gltiches
    //where stop never properly happens if call to m_engine->load fails in play)
    //if ( m_engine->loaded() )
    m_engine->stop();
}


void EngineController::playPause() //SLOT
{
    //this is used by the TrayIcon, PlayPauseAction and DCOP

    if( m_engine->state() == Engine::Playing )
    {
        pause();
    }
    else if( m_engine->state() == Engine::Paused )
    {
        if ( m_engine->loaded() )
            m_engine->unpause();
    }
    else
        play();
}


void EngineController::seek( int ms ) //SLOT
{
    if( bundle().length() > 0 )
    {
        trackPositionChangedNotify( ms, true ); /* User seek */
        engine()->seek( ms );
    }
}


void EngineController::seekRelative( int ms ) //SLOT
{
    if( m_engine->state() != Engine::Empty )
    {
        int newPos = m_engine->position() + ms;
        seek( newPos <= 0 ? 1 : newPos );
    }
}


void EngineController::seekForward( int ms )
{
    seekRelative( ms );
}


void EngineController::seekBackward( int ms )
{
    seekRelative( -ms );
}


int EngineController::increaseVolume( int ticks ) //SLOT
{
    return setVolume( m_engine->volume() + ticks );
}


int EngineController::decreaseVolume( int ticks ) //SLOT
{
    return setVolume( m_engine->volume() - ticks );
}


int EngineController::setVolume( int percent ) //SLOT
{
    m_muteVolume = 0;

    if( percent < 0 ) percent = 0;
    if( percent > 100 ) percent = 100;

    if( (uint)percent != m_engine->volume() )
    {
        m_engine->setVolume( (uint)percent );

        percent = m_engine->volume();
        AmarokConfig::setMasterVolume( percent );
        volumeChangedNotify( percent );
        return percent;
    }
    else // Still notify
    {
        volumeChangedNotify( percent );
    }

    return m_engine->volume();
}


void EngineController::mute() //SLOT
{
    if( m_muteVolume == 0 )
    {
        int saveVolume = m_engine->volume();
        setVolume( 0 );
        m_muteVolume = saveVolume;
    }
    else
    {
        setVolume( m_muteVolume );
        m_muteVolume = 0;
    }
}


const MetaBundle&
EngineController::bundle() const
{
    static MetaBundle null;
    return m_engine->state() == Engine::Empty ? null : m_bundle;
}


void EngineController::slotStreamMetaData( const MetaBundle &bundle ) //SLOT
{
    // Prevent spamming by ignoring repeated identical data (some servers repeat it every 10 seconds)
    if ( m_lastMetadata.contains( bundle ) )
        return;

    // We compare the new item with the last two items, because mth.house currently cycles
    // two messages alternating, which gets very annoying
    if ( m_lastMetadata.count() == 2 )
        m_lastMetadata.pop_front();

    m_lastMetadata << bundle;

    m_previousUrl = m_bundle.url();
    m_bundle = bundle;
    m_lastPositionOffset = m_positionOffset;
    if( m_lastFm )
        m_positionOffset = m_engine->position();
    else
        m_positionOffset = 0;
    newMetaDataNotify( m_bundle, false /* not a new track */ );
}

void EngineController::currentTrackMetaDataChanged( const MetaBundle& bundle )
{
    m_previousUrl = m_bundle.url();
    m_bundle = bundle;
    newMetaDataNotify( bundle, false /* no track change */ );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void EngineController::slotEngineMetaData( const Engine::SimpleMetaBundle &simpleBundle ) //SLOT
{
    if ( !m_bundle.url().isLocalFile() )
    {
        MetaBundle bundle = m_bundle;
        bundle.setArtist( simpleBundle.artist );
        bundle.setTitle( simpleBundle.title );
        bundle.setComment( simpleBundle.comment );
        bundle.setAlbum( simpleBundle.album );

        if( !simpleBundle.genre.isEmpty() )
            bundle.setGenre( simpleBundle.genre );
        if( !simpleBundle.bitrate.isEmpty() )
            bundle.setBitrate( simpleBundle.bitrate.toInt() );
        if( !simpleBundle.samplerate.isEmpty() )
            bundle.setSampleRate( simpleBundle.samplerate.toInt() );
        if( !simpleBundle.year.isEmpty() )
            bundle.setYear( simpleBundle.year.toInt() );
        if( !simpleBundle.tracknr.isEmpty() )
            bundle.setTrack( simpleBundle.tracknr.toInt() );

        slotStreamMetaData( bundle );
    }
}


void EngineController::slotMainTimer() //SLOT
{
    const uint position = trackPosition();

    trackPositionChangedNotify( position );

    // Crossfading
    if ( m_engine->state() == Engine::Playing &&
         AmarokConfig::crossfade() && m_xFadeThisTrack &&
         m_engine->hasPluginProperty( "HasCrossfade" ) &&
         Playlist::instance()->stopAfterMode() != Playlist::StopAfterCurrent &&
         ( (uint) AmarokConfig::crossfadeType() == 0 ||    //Always or...
           (uint) AmarokConfig::crossfadeType() == 1 ) &&  //...automatic track change only
         Playlist::instance()->isTrackAfter() &&
         m_bundle.length()*1000 - position < (uint) AmarokConfig::crossfadeLength() )
    {
        debug() << "Crossfading to next track...\n";
        m_engine->setXFadeNextTrack( true );
        trackFinished();
    }
    else if ( m_engine->state() == Engine::Playing &&
              AmarokConfig::fadeout() &&
              Playlist::instance()->stopAfterMode() == Playlist::StopAfterCurrent &&
              m_bundle.length()*1000 - position < (uint) AmarokConfig::fadeoutLength() )
    {
       m_engine->stop();
    }
}


void EngineController::slotTrackEnded() //SLOT
{
    if ( AmarokConfig::trackDelayLength() > 0 )
    {
        //FIXME not perfect
        if ( !m_isTiming )
        {
            QTimer::singleShot( AmarokConfig::trackDelayLength(), this, SLOT(trackFinished()) );
            m_isTiming = true;
        }

    }
    else trackFinished();
}


void EngineController::slotStateChanged( Engine::State newState ) //SLOT
{

    switch( newState )
    {
    case Engine::Empty:

        //FALL THROUGH...

    case Engine::Paused:

        m_timer->stop();
        break;

    case Engine::Playing:

        m_timer->start( MAIN_TIMER );
        break;

    default:
        ;
    }

    stateChangedNotify( newState );
}

uint EngineController::trackPosition() const
{
    const uint buffertime = 5000; // worked for me with xine engine over 1 mbit dsl
    if( !m_engine )
        return 0;
    uint pos = m_engine->position();
    if( !m_lastFm )
        return pos;

    if( m_positionOffset + buffertime <= pos )
        return pos - m_positionOffset - buffertime;
    if( m_lastPositionOffset + buffertime <= pos )
        return pos - m_lastPositionOffset - buffertime;
    return pos;
}


#include "enginecontroller.moc"
