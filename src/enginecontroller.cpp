/***************************************************************************
                      enginecontroller.cpp  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
                       (C) 2004 by Max Howell
                       (C) 2004 by Mark Kretschmann
email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "pluginmanager.h"
#include "statusbar.h"
#include "streamprovider.h"

#include <qtimer.h>

#include <kapplication.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>


ExtensionCache EngineController::s_extensionCache;


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
    , m_delayTime( 0 )
    , m_muteVolume( 0 )
    , m_xFadeThisTrack( false )
    , m_timer( new QTimer( this ) )
    , m_stream( 0 )
{
    m_engine = (EngineBase*)loadEngine( "void-engine" );

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotMainTimer() ) );
}

EngineController::~EngineController()
{
    DEBUG_FUNC_INFO //we like to know when singletons are destroyed
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

EngineBase *EngineController::loadEngine() //static
{
    DEBUG_BEGIN

    Engine::Base   *engine = instance()->m_engine;
    amaroK::Plugin *plugin = loadEngine( AmarokConfig::soundSystem() );

    {
        QObject *bar = amaroK::StatusBar::instance();
        QObject *eng = (EngineBase*)plugin;
        QObject *ins = instance();

        connect( eng, SIGNAL(stateChanged( Engine::State )),
                        ins, SLOT(slotStateChanged( Engine::State )) );
        connect( eng, SIGNAL(trackEnded()),
                        ins, SLOT(slotTrackEnded()) );
        connect( eng, SIGNAL(statusText( const QString& )),
                        bar, SLOT(shortMessage( const QString& )) );
        connect( eng, SIGNAL(infoMessage( const QString& )),
                        bar, SLOT(longMessage( const QString& )) );
        connect( eng, SIGNAL(metaData( const Engine::SimpleMetaBundle& )),
                        ins, SLOT(slotEngineMetaData( const Engine::SimpleMetaBundle& )) );
        connect( eng, SIGNAL(showConfigDialog( const QCString& )),
                        kapp, SLOT(slotConfigAmarok( const QCString& )) );
    }

    if( static_cast<EngineBase*>(plugin)->init() )
    {
        //only change things if the init was successful,
        //otherwise leave amaroK with the old engine loaded

        //set amaroK to stopped state
        instance()->stop();

        //new engine, new ext cache required
        extensionCache().clear();

        //assign new engine, unload old one. Order is thread-safe!
        instance()->m_engine = static_cast<EngineBase*>(plugin);
        PluginManager::unload( engine );

        engine = static_cast<EngineBase*>(plugin);

        //the engine is not required to do this when we unload it
        //but we need to do it to ensure amaroK looks correctly
        //and to delete m_stream
        instance()->slotStateChanged( Engine::Empty );

        //NOTE engine settings are set in App::applySettings()

    } else {

        //init failed - fall back to currently loaded engine
        //TODO avoid two levels of message dialogs, eg
        //       gst-engine says "argh you haven't run gst-register"
        //       then the following message is shown

        QString oldEngine = PluginManager::getService( engine )->property( "X-KDE-amaroK-name" ).toString();

        StatusBar::instance()->longMessage(
            i18n( "amaroK could not initialize the '%1', instead we will revert to the '%2'" )
                .arg( AmarokConfig::soundSystem(), oldEngine ),
            KDE::StatusBar::Error );

        AmarokConfig::setSoundSystem( PluginManager::getService( engine )->property( "X-KDE-amaroK-name" ).toString() );

        delete plugin;
     }

    DEBUG_END

    return engine;
}


amaroK::Plugin *EngineController::loadEngine( const QString &engineName )
{
    QString query = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] == '%1'";
    amaroK::Plugin* plugin = PluginManager::createFromQuery( query.arg( engineName ) );

    if( !plugin ) {
       query = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] != '%1'";
       KTrader::OfferList offers = PluginManager::query( query.arg( engineName ) );

       /*
         for ( uint i = 0; i < offers.count(); i++ )
             kdDebug() << "  - offers[" << i << "].name: " << offers[i]->name()
             << "; rank:" << offers[i]->property( "X-KDE-amaroK-rank" ).toInt() << endl;
       */
       while( !plugin && !offers.isEmpty() ) {
          // prioritise highest rank for left engines
          int rank = 0;
          uint current = 0;
          for ( uint i = 0; i < offers.count(); i++ ) {
            if ( offers[i]->property( "X-KDE-amaroK-rank" ).toInt() > rank ) {
             current = i;
             rank = offers[i]->property( "X-KDE-amaroK-rank" ).toInt();
            }
          }
          plugin = PluginManager::createFromService( offers[current] );
          offers.remove(offers[current]);
       }

       if( !plugin ) {
          KMessageBox::error( 0, i18n(
            "<p>amaroK could not find any sound-engine plugins. "
            "It is likely that amaroK is installed under the wrong prefix, please fix your installation using:<pre>"
            "$ cd /path/to/amarok/source-code/<br>"
            "$ su -c \"make uninstall\"<br>"
            "$ ./configure --prefix=`kde-config --prefix` && su -c \"make install\"<br>"
            "$ kbuildsycoca<br>"
            "$ amarok</pre>"
            "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net." ) );

          ::exit( EXIT_SUCCESS );
       }

       AmarokConfig::setSoundSystem( PluginManager::getService( plugin )->property( "X-KDE-amaroK-name" ).toString() );

       StatusBar::instance()->longMessage( i18n( "Sorry, the requested engine could not be loaded" ), KDE::StatusBar::Sorry );
   }

   return plugin;
}


bool EngineController::canDecode( const KURL &url ) //static
{
   //NOTE this function must be thread-safe
    //TODO a KFileItem version? <- presumably so we can mimetype check

    const QString fileName = url.fileName();
    const QString ext = amaroK::extension( fileName );

    //FIXME why do we do this? Please add comments to odd looking code!
    if ( ext == "m3u" || ext == "pls" ) return false;

    // Ignore protocols "fetchcover" and "musicbrainz", they're not local but we dont really want them in the playlist :)
    if ( url.protocol() == "fetchcover" || url.protocol() == "musicbrainz" ) return false;

    // Accept non-local files, since we can't test them for validity at this point
    if ( !url.isLocalFile() ) return true;

    // If extension is already in the cache, return cache result
    if ( extensionCache().contains( ext ) )
        return s_extensionCache[ext];

    const bool valid = engine()->canDecode( url );

    //we special case this as otherwise users hate us
    if ( !valid && ext == "mp3" )
        amaroK::StatusBar::instance()->longMessageThreadSafe(
           i18n( "<p>The %1 claims it <b>cannot</b> play MP3 files."
                 "<p>You may want to choose a different engine from the <i>Configure Dialog</i>, or examine "
                 "the installation of the multimedia-framework that the current engine uses. "
                 "<p>You may find useful information in the <i>FAQ</i> section of the <i>amaroK HandBook</i>." )
            .arg( AmarokConfig::soundSystem() ) );

    // Cache this result for the next lookup
    if ( !ext.isEmpty() )
        extensionCache().insert( ext, valid );

    return valid;
}


void EngineController::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if( !AmarokConfig::resumeTrack().isEmpty() )
    {
        const KURL url = AmarokConfig::resumeTrack();

        if ( m_engine->load( url ) && m_engine->play( AmarokConfig::resumeTime() ) )
            newMetaDataNotify( m_bundle = MetaBundle( url ), true );
    }
}


void EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( m_bundle.length() > 0 && !AmarokConfig::resumePlayback() )
        trackEnded( m_engine->position(), m_bundle.length() * 1000 );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void EngineController::previous() //SLOT
{
    emit orderPrevious();
}


void EngineController::next() //SLOT
{
    emit orderNext();
}


void EngineController::play() //SLOT
{
    if ( m_engine->state() == Engine::Paused )
    {
        m_engine->pause();
    }
    else emit orderCurrent();
}


void EngineController::play( const MetaBundle &bundle )
{
    const KURL &url = bundle.url();
    kdDebug() << "[controller] Loading URL: " << url.url() << endl;
    // Destroy stale StreamProvider
    delete m_stream;

    //TODO bummer why'd I do it this way? it should _not_ be in play!
    //let amaroK know that the previous track is no longer playing
    if ( m_timer->isActive() && m_bundle.length() > 0 )
        trackEnded( m_engine->position(), m_bundle.length() * 1000 );

    if ( m_engine->pluginProperty( "StreamingMode") != "NoStreaming" && url.protocol() == "http" ) {
        m_bundle = bundle;
        m_xFadeThisTrack = false;
        // Detect mimetype of remote file
        KIO::MimetypeJob* job = KIO::mimetype( url, false );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(playRemote( KIO::Job* )) );
        StatusBar::instance()->shortMessage( i18n("Connecting to stream source...") );
        return; //don't do notify
    }


    if( m_engine->load( url ) )
    {
        //assign bundle now so that it is available when the engine
        //emits stateChanged( Playing )
        m_bundle = bundle;

        if( m_engine->play() )
        {
            m_xFadeThisTrack = AmarokConfig::crossfade() &&
                               m_engine->hasPluginProperty( "HasCrossfade" ) &&
                              !m_engine->isStream() &&
                               m_bundle.length()*1000 - AmarokConfig::crossfadeLength()*2 > 0;

            newMetaDataNotify( bundle, true /* track change */ );
        }
        else goto some_kind_of_failure;
    }
    else
    some_kind_of_failure:
        //don't do for repeatPlaylist() as it can produce a freeze
        //FIXME -> mxcl
        if ( !AmarokConfig::repeatPlaylist() )
            next();
}


void EngineController::pause() //SLOT
{
    if ( m_engine->loaded() )
        m_engine->pause();
}


void EngineController::stop() //SLOT
{
    //let amaroK know that the previous track is no longer playing
    if ( m_bundle.length() > 0 )
        trackEnded( m_engine->position(), m_bundle.length() * 1000 );

    if ( m_engine->loaded() )
        m_engine->stop();
}


void EngineController::playPause() //SLOT
{
    //this is used by the TrayIcon, PlayPauseAction and DCOP

    if( m_engine->state() == Engine::Playing )
    {
        pause();
    }
    else play();
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

    return m_engine->volume();
}


void EngineController::mute() //SLOT
{
    if( m_muteVolume == 0 )
    {
        m_muteVolume = m_engine->volume();
        setVolume( 0 );
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
    return m_engine->state() == Engine::Empty ? MetaBundle::null : m_bundle;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void EngineController::playRemote( KIO::Job* job ) //SLOT
{
    const QString mimetype = static_cast<KIO::MimetypeJob*>( job )->mimetype();
    kdDebug() << "[controller] Detected mimetype: " << mimetype << endl;

    const KURL &url = m_bundle.url();

    const bool isStream = mimetype.isEmpty() || mimetype == "text/html" ||
                          url.host().endsWith( "last.fm" ); // HACK last.fm uses the mimetype audio/x-mp3

    if ( isStream && m_engine->pluginProperty( "StreamingMode" ) != "NoStreaming" )
    {
        delete m_stream;
        m_stream = new amaroK::StreamProvider( url, m_engine->pluginProperty( "StreamingMode" ) );

        if ( !m_stream->initSuccess() || !m_engine->play( m_stream->proxyUrl(), isStream ) ) {
            delete m_stream;
            if ( !AmarokConfig::repeatPlaylist() )
                next();
            return; //don't notify
        }

        connect( m_stream, SIGNAL(metaData( const MetaBundle& )),
                 this,       SLOT(slotStreamMetaData( const MetaBundle& )) );
        connect( m_stream, SIGNAL(streamData( char*, int )),
                 m_engine,   SLOT(newStreamData( char*, int )) );
        connect( m_stream, SIGNAL(sigError()),
                 this,     SIGNAL(orderNext()) );
    }
    else if( !m_engine->play( url, isStream ) && !AmarokConfig::repeatPlaylist() )
    {
        next();
        return; //don't notify
    }

    newMetaDataNotify( m_bundle, true /* track change */ );
}

void EngineController::slotStreamMetaData( const MetaBundle &bundle ) //SLOT
{
    m_bundle = bundle;
    newMetaDataNotify( m_bundle, false /* not a new track */ );
}

void EngineController::slotEngineMetaData( const Engine::SimpleMetaBundle &bundle ) //SLOT
{
    if ( m_engine->isStream() )
    {
        m_bundle.setArtist( bundle.artist );
        m_bundle.setTitle( bundle.title );
        m_bundle.setComment( bundle.comment );
        m_bundle.setAlbum( bundle.album );

        newMetaDataNotify( m_bundle, false /* not a new track */ );
    }
}

void EngineController::slotMainTimer() //SLOT
{
    const uint position = m_engine->position();

    trackPositionChangedNotify( position );

    // Crossfading
    if ( m_engine->state() == Engine::Playing &&
         m_xFadeThisTrack &&
         ( m_bundle.length()*1000 - position < (uint) AmarokConfig::crossfadeLength() ) )
    {
        kdDebug() << "[controller] Crossfading to next track...\n";
        next();
    }
}

void EngineController::slotTrackEnded() //SLOT
{
    if ( AmarokConfig::trackDelayLength() > 0 )
    {
        //FIXME not perfect
        QTimer::singleShot( AmarokConfig::trackDelayLength(), this, SLOT(next()) );
    }
    else next();
}

void EngineController::slotStateChanged( Engine::State newState ) //SLOT
{
    switch( newState )
    {
    case Engine::Empty:

        delete m_stream;

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


void EngineController::streamError() //SLOT
{
    delete m_stream;
    next();
}



#include "enginecontroller.moc"
