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
#include "metabundle.h"
#include "amarokconfig.h"
#include "titleproxy.h"

#include <kdebug.h>
#include <qtimer.h>

EngineController *EngineController::Instance = 0;

EngineController *EngineController::instance()
{
    if( Instance == 0 )
    {
        Instance = new EngineController();
    }
    return Instance;
}

EngineController::EngineController()
    : m_pEngine( 0 )
    , m_pMainTimer( new QTimer( this ) )
    , m_length( 0 )
    , m_delayTime( 0 )
{
    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    m_pMainTimer->start( MAIN_TIMER );
}


EngineController::~EngineController()
{
    Instance = 0;
    m_pMainTimer->stop();
}

void EngineController::previous()
{
    if( !m_pEngine ) return;

    emit orderPrevious();
}

void EngineController::next()
{
    if( !m_pEngine ) return;

    emit orderNext();
}

void EngineController::playPause()
{
    //this is used by the TrayIcon and PlayPauseAction

    if( m_pEngine && m_pEngine->state() == EngineBase::Playing )
    {
        pause();
    }
    else play();
}


void EngineController::play()
{
    if( !m_pEngine ) return;

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
    const KURL &url = bundle.url();
    
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

    m_playingURL = url;
    kdDebug() << "[engine] Playing: " << url.filename() << endl;
    
    stateChangedNotify( EngineBase::Playing );
    newMetaDataNotify( bundle, true /* track change */ );

    //when TagLib can't get us the track length, we ask the engine as fallback
//    m_determineLength = ( engine->isStream() || bundle.length() ) ? false : true;
    m_length = bundle.length() * 1000;
}

void EngineController::pause()
{
    if( !m_pEngine ) return;

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
    if( !m_pEngine ) return;

    m_playingURL = KURL();
    m_pEngine->stop();
    stateChangedNotify( m_pEngine->state() );
}

int EngineController::setVolume( int percent )
{
    if( percent < 0 ) percent = 0;
    if( percent > 100 ) percent = 100;

    if( percent != m_pEngine->volume() )
    {
        m_pEngine->setVolume( percent );
        AmarokConfig::setMasterVolume( percent );
    }

    volumeChangedNotify( percent );
    return m_pEngine->volume();
}


void EngineController::newMetaData( const MetaBundle &bundle )
{
    newMetaDataNotify( bundle, false /* not a new track */ );
}

void EngineController::slotMainTimer()
{
    if ( m_playingURL.isEmpty() )
        return;

    // TODO!!! (if length 0 then check --> over and over )
    // send signal on success
    //try to get track length from engine when TagLib fails
    trackPositionChangedNotify( m_pEngine->position() );

    // check if track has ended or is broken
    if ( m_pEngine->state() == EngineBase::Empty ||
         m_pEngine->state() == EngineBase::Idle )
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
              ( m_length ) &&
              ( m_length > m_pEngine->position() ) &&
              ( m_length - m_pEngine->position() < AmarokConfig::crossfadeLength() )  )
    {
        kdDebug() << k_funcinfo << "Crossfading to next track.\n";
        next();
        return;
    }
}


#include "enginecontroller.moc"
