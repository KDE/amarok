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

#include "enginecontroller.h"
#include "enginebase.h"
#include "amarokconfig.h"
#include "titleproxy.h"

#include <kdebug.h>
#include <qtimer.h>


class DummyEngine : public EngineBase
{
    virtual void init( bool&, int, bool ) {}
    virtual bool initMixer( bool ) { return false; }
    virtual bool canDecode( const KURL&, mode_t, mode_t ) { return false; }
    virtual long length() const { return 0; }
    virtual long position() const { return 0; }
    virtual EngineState state() const { return EngineBase::Empty; }
    virtual bool isStream() const { return false; }
    virtual const QObject* play( const KURL& ) { return 0; }
    virtual void play() {}
    virtual void stop() {}
    virtual void pause() {}

    virtual void seek( long ) {}
    virtual void setVolume( int ) {}

public: DummyEngine() : EngineBase() { setName( "Dummy" ); }

} dummyEngine;


EngineController *EngineController::instance()
{
    //will only be instantiated the first time this function is called
    static EngineController Instance;

    return &Instance;
}

EngineController::EngineController()

    //we use the dummy Engine to ensure we always have an engine and don't have to test for
    //m_pEngine == 0 and amaroK is in a safe state before an engine is loaded on startup

    : m_pEngine( &dummyEngine )
    , m_proxyError( false )
    , m_pMainTimer( new QTimer( this ) )
    , m_delayTime( 0 )
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

    if ( AmarokConfig::titleStreaming() &&
         engine()->streamingMode() != EngineBase::NoStreaming &&
         url.protocol() == "http" &&
         !url.path().endsWith( ".ogg" ) )
    {
        TitleProxy::Proxy* proxy = new TitleProxy::Proxy( url, engine()->streamingMode() );
        if ( !proxy->initSuccess() ) {
            delete proxy;
            emit orderNext();
            return;
        }
        const QObject* object = m_pEngine->play( proxy->proxyUrl() );

        connect( this,     SIGNAL( deleteProxy () ),
                 proxy,      SLOT( deleteLater () ) );
        connect( proxy,    SIGNAL( metaData( const MetaBundle& ) ),
                 this,       SLOT( newMetaData( const MetaBundle& ) ) );
        connect( proxy,    SIGNAL( streamData( char*, int ) ),
                 engine(),   SLOT( newStreamData( char*, int ) ) );

        if ( object )
            connect( object, SIGNAL( destroyed () ),
                     proxy,    SLOT( deleteLater() ) );
    }
    else
        m_pEngine->play( url );

    kdDebug() << "[engine] Playing: " << url.filename() << endl;

    stateChangedNotify( EngineBase::Playing );
    newMetaDataNotify( bundle, true /* track change */ );

    //when TagLib can't get us the track length, we ask the engine as fallback
//    m_determineLength = ( engine->isStream() || bundle.length() ) ? false : true;
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

int EngineController::increaseVolume()
{
    return setVolume( m_pEngine->volume() + 100/25 );
}

int EngineController::decreaseVolume()
{
    return setVolume( m_pEngine->volume() - 100/25 );
}

void EngineController::setEngine( EngineBase *engine )
{
    instance()->m_pEngine = engine;

    engine->setVolume( AmarokConfig::masterVolume() );
}

int EngineController::setVolume( int percent )
{
    if( percent < 0 ) percent = 0;
    if( percent > 100 ) percent = 100;

    if( percent != m_pEngine->volume() )
    {
        m_pEngine->setVolume( percent );
        AmarokConfig::setMasterVolume( percent );

        volumeChangedNotify( percent );
    }

    return m_pEngine->volume();
}

inline void EngineController::newMetaData( const MetaBundle &bundle )
{
    newMetaDataNotify( bundle, false /* not a new track */ );
}

inline void EngineController::slotMainTimer()
{
    if( m_pEngine->state() == EngineBase::Empty ) return;

    const uint position = m_pEngine->position();
    const uint length   = m_bundle.length() * 1000;

    // TODO!!! (if length 0 then check --> over and over )
    // send signal on success
    // try to get track length from engine when TagLib fails
    trackPositionChangedNotify( position );

    // check if track has ended or is broken
    if ( m_pEngine->state() == EngineBase::Idle )
    {
        kdDebug() << k_funcinfo << "Idle detected. Skipping track.\n";

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
    // Crossfading
    else if ( ( AmarokConfig::crossfade() ) &&
              ( m_pEngine->supportsXFade() ) &&
              ( !m_pEngine->isStream() ) &&
              ( length ) &&
              ( length - position < (uint)AmarokConfig::crossfadeLength() )  )
    {
        kdDebug() << k_funcinfo << "Crossfading to next track.\n";
        next();
    }
}


#include "enginecontroller.moc"
