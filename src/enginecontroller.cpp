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

#include "amarokconfig.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "pluginmanager.h"
#include "statusbar.h"
#include "streamprovider.h"

#include <qtimer.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>


ExtensionCache EngineController::s_extensionCache;


static EngineBase*
dummyEngine()
{
    //use a function so it isn't created when --help is called
    //for instance (and thus help output isn't clouded by
    //Plugin::Plugin debug ouput)

    static
    class DummyEngine : public EngineBase
    {
        //Does nothing, just here to prevent crashes on startup
        //and in case no engines are found

        virtual bool init() { return true; }
        virtual bool canDecode( const KURL& ) const { return false; }
        virtual uint position() const { return 0; }
        virtual bool load( const KURL&, bool ) { return false; }
        virtual bool play( uint ) { return false; }
        virtual void stop() {}
        virtual void pause() {}
        virtual void setVolumeSW( uint ) {}
        virtual void seek( uint ) {}

        virtual Engine::State state() const { return Engine::Empty; }

    public: DummyEngine() : EngineBase() {}

    } dummyEngine;

    return &dummyEngine;
}


EngineController::EngineController()
    : m_engine( dummyEngine() )
    , m_delayTime( 0 )
    , m_muteVolume( 0 )
    , m_xFadeThisTrack( false )
    , m_timer( new QTimer( this ) )
    , m_stream( 0 )
{
    connect( m_timer, SIGNAL( timeout() ), SLOT( slotMainTimer() ) );
}



//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

EngineBase *EngineController::loadEngine() //static
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    EngineBase *engine = instance()->m_engine;

    //now load new engine
    const QString query    = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] == '%1'";
    amaroK::Plugin* plugin = PluginManager::createFromQuery( query.arg( AmarokConfig::soundSystem() ) );

    if( !plugin )
    {
        QString query = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] != '%1'";
        KTrader::OfferList offers = PluginManager::query( query.arg( AmarokConfig::soundSystem() ) );

        while( !plugin && !offers.isEmpty() ) {
            plugin = PluginManager::createFromService( offers.front() );
            offers.pop_front();
        }

        if( !plugin )
        {
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

        //TODO KMessageBox::error( 0, i18n( "The requested engine could not be loaded, instead %1 was loaded." ) );
        kdDebug() << "Setting soundSystem to: " << AmarokConfig::soundSystem() << endl;
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
        if( engine != dummyEngine() ) PluginManager::unload( engine );

        engine = static_cast<EngineBase*>(plugin);

        connect( engine, SIGNAL(stateChanged( Engine::State )), instance(), SLOT(slotStateChanged( Engine::State )) );
        connect( engine, SIGNAL(trackEnded()), instance(), SLOT(slotTrackEnded()) );
        connect( engine, SIGNAL(statusText( const QString& )), instance(), SIGNAL(statusText( const QString& )) );
        connect( engine, SIGNAL(showConfigDialog( int )), kapp, SLOT(slotConfigAmarok( int )) );

        //NOTE engine settings are set in App::applySettings()

    } else {

        //init failed - fall back to currently loaded engine
        KMessageBox::error( 0, i18n( "The new engine could not be loaded." ) );
        AmarokConfig::setSoundSystem( PluginManager::getService( engine )->property( "X-KDE-amaroK-name" ).toString() );
    }

    kdDebug() << "END " << k_funcinfo << endl;

    return engine;
}


bool EngineController::canDecode( const KURL &url ) //static
{
    //TODO engine refactor branch has to be KURL aware for this function
    //TODO a KFileItem version?

    // Accept non-local files, since we can't test them for validity at this point
    if ( !url.isLocalFile() ) return true;

    const QString fileName = url.fileName();
    const QString ext = fileName.mid( fileName.findRev( '.' ) + 1 ).lower();

    if ( extensionCache().contains( ext ) )
        return s_extensionCache[ext];

    const bool valid = engine()->canDecode( url );

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

        if ( m_engine->load( url ) && m_engine->play( AmarokConfig::resumeTime()*1000 ) )
            newMetaDataNotify( m_bundle = MetaBundle( url ), true );
    }
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
    // Destroy stale StreamProvider
    delete m_stream;

    if ( m_engine->streamingMode() != Engine::NoStreaming && url.protocol() == "http" ) {
        m_bundle = bundle;
        m_xFadeThisTrack = false;
        // Detect mimetype of remote file
        KIO::MimetypeJob* job = KIO::mimetype( url, false );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(playRemote( KIO::Job* )) );
        StatusBar::instance()->message( i18n("Connecting to stream source...") );
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
                m_engine->hasXFade() &&
                !m_engine->isStream() &&
                m_bundle.length()*1000 - AmarokConfig::crossfadeLength()*2 > 0;

            newMetaDataNotify( bundle, true /* track change */ );
        }
    }
    else m_bundle = MetaBundle::null;
}


void EngineController::pause() //SLOT
{
    if ( m_engine->loaded() )
        m_engine->pause();
}


void EngineController::stop() //SLOT
{
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
    const bool isStream = mimetype.isEmpty() || mimetype == "text/html";

    StatusBar::instance()->clear();

    if ( isStream &&
         AmarokConfig::titleStreaming() &&
         m_engine->streamingMode() != Engine::NoStreaming )
    {
        m_stream = new amaroK::StreamProvider( url, m_engine->streamingMode() );

        if ( !m_stream->initSuccess() || !m_engine->play( m_stream->proxyUrl(), isStream ) ) {
            delete m_stream;
            next();
            return; //don't notify
        }

        connect( m_stream, SIGNAL(metaData( const MetaBundle& )),
                 this,       SLOT(slotNewMetaData( const MetaBundle& )) );
        connect( m_stream, SIGNAL(streamData( char*, int )),
                 m_engine,   SLOT(newStreamData( char*, int )) );
        connect( m_stream, SIGNAL(sigError()),
                 this,     SIGNAL(orderNext()) );
    }
    else if( !m_engine->play( url, isStream ) )
    {
        next();
        return; //don't notify
    }

    newMetaDataNotify( m_bundle, true /* track change */ );
}

void EngineController::slotNewMetaData( const MetaBundle &bundle ) //SLOT
{
    m_bundle = bundle;

    newMetaDataNotify( bundle, false /* not a new track */ );
}

void EngineController::slotMainTimer() //SLOT
{
    const uint position = m_engine->position();

    trackPositionChangedNotify( position );

    // Crossfading
    if ( m_xFadeThisTrack && (m_bundle.length()*1000 - position < (uint)AmarokConfig::crossfadeLength()) )
    {
        kdDebug() << "[controller] Crossfading...\n";
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
