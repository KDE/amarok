/***************************************************************************
                      enginecontroller.cpp  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
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
#include "app.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "pluginmanager.h"
#include "titleproxy.h"

#include <kdebug.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>

#include <qtimer.h>


static
class DummyEngine : public EngineBase
{
    //Does nothing, just here to prevent crashes on startup
    //and in case no engines are found

    virtual void init( bool&, int, bool ) {}
    virtual bool initMixer( bool ) { return false; }
    virtual bool canDecode( const KURL&, mode_t, mode_t ) { return false; }
    virtual long length() const { return 0; }
    virtual long position() const { return 0; }
    virtual EngineState state() const { return EngineBase::Empty; }
    virtual void play( const KURL&, bool ) {}
    virtual void play() {}
    virtual void stop() {}
    virtual void pause() {}

    virtual void seek( long ) {}
    virtual void setVolume( int v ) { m_volume = v; }

public: DummyEngine() : EngineBase() { setName( "Dummy" ); }

} dummyEngine;


EngineController *EngineController::instance()
{
    //will only be instantiated the first time this function is called
    static EngineController Instance;

    return &Instance;
}


EngineController::EngineController()
    : m_pEngine( &dummyEngine )
    , m_proxyError( false )
    , m_pMainTimer( new QTimer( this ) )
    , m_delayTime( 0 )
    , m_muteVolume( 0 )
{
    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    m_pMainTimer->start( MAIN_TIMER );
}


EngineController::~EngineController()
{}


void EngineController::previous()
{
    emit orderPrevious();
}


void EngineController::next()
{
    emit orderNext();
}


void EngineController::playPause()
{
    //this is used by the TrayIcon and PlayPauseAction

    if( m_pEngine->state() == EngineBase::Playing )
    {
        pause();
    }
    else play();
}


void EngineController::play()
{
    if ( m_pEngine->state() == EngineBase::Paused )
    {
        m_pEngine->play();
        stateChangedNotify( EngineBase::Playing );
    }
    else
        emit orderCurrent(); // keep currenttrack to avoid signal?
}


void EngineController::play( const MetaBundle &bundle )
{
    m_bundle = bundle;
    const KURL &url = bundle.url();

    kdDebug() << "[engine] Playing: " << url.filename() << endl;

    if ( url.protocol() == "http" ) {
        // Detect mimetype of remote file
        KIO::MimetypeJob* job = KIO::mimetype( url, false );
        connect( job, SIGNAL( result( KIO::Job* ) ), this, SLOT( playRemote( KIO::Job* ) ) );
        return;
    }
                        
    m_pEngine->play( url );

    stateChangedNotify( EngineBase::Playing );
    newMetaDataNotify( bundle, true /* track change */ );
}


void EngineController::playRemote( KIO::Job* job ) //SLOT
{
    const QString mimetype = static_cast<KIO::MimetypeJob*>( job )->mimetype();
    kdDebug() << "DETECTED MIMETYPE: " << mimetype << endl;
    
    const KURL &url = m_bundle.url();
    const bool stream = mimetype.isEmpty() || mimetype == "text/html";               
         
    if ( stream &&
         AmarokConfig::titleStreaming() &&
         m_pEngine->streamingMode() != EngineBase::NoStreaming )
    {
        TitleProxy::Proxy* proxy = new TitleProxy::Proxy( url, m_pEngine->streamingMode() );
        if ( !proxy->initSuccess() ) {
            delete proxy;
            emit orderNext();
            return;
        }
        m_pEngine->stop(); //hack, prevents artsengine killing the proxy when stopped() is emitted
        m_pEngine->play( proxy->proxyUrl(), stream );

        connect( proxy,     SIGNAL( metaData( const MetaBundle& ) ),
                 this,        SLOT( newMetaData( const MetaBundle& ) ) );
        connect( proxy,     SIGNAL( streamData( char*, int ) ),
                 m_pEngine,   SLOT( newStreamData( char*, int ) ) );
        connect( proxy,     SIGNAL( proxyError() ),
                 this,      SIGNAL( orderNext() ) );
        connect( m_pEngine, SIGNAL( stopped() ),
                 proxy,       SLOT( deleteLater() ) );
    }
    else
        m_pEngine->play( url, stream );

    kdDebug() << "[engine] Playing: " << url.filename() << endl;

    stateChangedNotify( EngineBase::Playing );
    newMetaDataNotify( m_bundle, true /* track change */ );
}


void EngineController::pause()
{
    if ( m_pEngine->loaded() )
    {
        if ( m_pEngine->state() == EngineBase::Paused )
            m_pEngine->play();
        else
            m_pEngine->pause();
        stateChangedNotify( m_pEngine->state() );
    }
}


void EngineController::stop()
{
    m_bundle = MetaBundle();
    m_pEngine->stop();
    stateChangedNotify( m_pEngine->state() );
}


int EngineController::increaseVolume( int ticks )
{
    return setVolume( m_pEngine->volume() + ticks );
}


int EngineController::decreaseVolume( int ticks )
{
    return setVolume( m_pEngine->volume() - ticks );
}


EngineBase *EngineController::loadEngine() //static
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    EngineBase *m_pEngine = engine();

    //first unload previous engine
    if( m_pEngine != &dummyEngine ) PluginManager::unload( m_pEngine );

    //now load new engine
    const QString query    = "[X-KDE-amaroK-plugintype] == 'engine' and Name == '%1'";
    amaroK::Plugin* plugin = PluginManager::createFromQuery( query.arg( AmarokConfig::soundSystem() ) );

    if ( !plugin )
    {
        kdWarning() << "Cannot load the: " << AmarokConfig::soundSystem() << " plugin. Trying another engine..\n";

        //try to invoke _any_ engine plugin
        plugin = PluginManager::createFromQuery( "[X-KDE-amaroK-plugintype] == 'engine'" );

        if ( !plugin )
        {
            KMessageBox::error( 0, i18n(
                "<p>amaroK could not find any sound-engine plugins. "
                "It is likely that amaroK is installed under the wrong prefix, please fix your installation using:"
                "<pre>$ cd /path/to/amarok/source-code/<br>"
                "$ su -c \"make uninstall\"<br>"
                "$ ./configure --prefix=`kde-config --prefix` && su -c \"make install\"<br>"
                "$ kbuildsycoca<br>"
                "$ amarok</pre>"
                "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net." ) );

            ::exit( EXIT_SUCCESS );
        }

        AmarokConfig::setSoundSystem( PluginManager::getService( plugin )->name() );
        kdDebug() << "Setting soundSystem to: " << AmarokConfig::soundSystem() << endl;
    }

    m_pEngine = static_cast<EngineBase*>( plugin );
    bool restartArts = AmarokConfig::version() != APP_VERSION;

    m_pEngine->init( restartArts, amaroK::SCOPE_SIZE, AmarokConfig::rememberEffects() );
    connect( m_pEngine, SIGNAL( endOfTrack() ), instance(), SLOT( slotEndOfTrack() ) );

    instance()->m_pEngine = m_pEngine;

    kdDebug() << "END " << k_funcinfo << endl;
    //NOTE App::applySettings() must be called now to ensure mixer settings are set

    return m_pEngine;
}


int EngineController::setVolume( int percent )
{
    if( percent < 0 ) percent = 0; //can't make uint as all signals use int and slot has to match
    if( percent > 100 ) percent = 100;

    if( percent != m_pEngine->volume() )
    {
        AmarokConfig::setMasterVolume( percent );

        m_pEngine->setVolume( percent );

        volumeChangedNotify( percent );
    }

    return m_pEngine->volume();
}

void EngineController::mute()
{
    if( m_muteVolume == 0 )
    {
        m_muteVolume = m_pEngine->volume();
        setVolume( 0 );
    }
    else
    {
        setVolume( m_muteVolume );
        m_muteVolume = 0;
    }
}

void EngineController::newMetaData( const MetaBundle &bundle )
{
    m_bundle = bundle;

    newMetaDataNotify( m_bundle, false /* not a new track */ );
}


void EngineController::slotMainTimer()
{
    if( m_pEngine->state() == EngineBase::Empty ) return;

    const uint position = m_pEngine->position();
    const uint length   = m_bundle.length() * 1000;

    trackPositionChangedNotify( position );

    // Crossfading
    if ( ( AmarokConfig::crossfade() ) &&
         ( m_pEngine->supportsXFade() ) &&
         ( !m_pEngine->isStream() ) &&
         ( length ) &&
         ( length - position < (uint)AmarokConfig::crossfadeLength() )  )
    {
        kdDebug() << k_funcinfo << "Crossfading to next track.\n";
        next();
    }
}


void EngineController::slotEndOfTrack()
{
    if ( AmarokConfig::trackDelayLength() > 0 ) //this can occur syncronously to XFade and not be fatal
    {
        //delay before start of next track, without freezing the app
        m_delayTime += MAIN_TIMER;
        if ( m_delayTime >= AmarokConfig::trackDelayLength() )
        {
            m_delayTime = 0;
            next();
        }
    }
    else
        next();
}


#include "enginecontroller.moc"
