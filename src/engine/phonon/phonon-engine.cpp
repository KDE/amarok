/***************************************************************************
 *   Copyright (C) 2007   Dan Meltzer <hydrogen@notyetimplemented.com>     *
 *                 2007   Seb Ruiz <me@sebruiz.net>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-engine"

#include "phonon-engine.h"
#include "amarok.h"
#include "amarokconfig.h"
//these files are from libamarok
#include "playlist.h"
#include "enginecontroller.h"
//Added by qt3to4:
#include <QTimerEvent>
#include <QCustomEvent>
#include <QEvent>

AMAROK_EXPORT_PLUGIN( PhononEngine )

#include "debug.h"
#include "statusbar/statusbar.h"

// #include <klocale.h>
#include <kmessagebox.h>
// #include <kstandarddirs.h>

#include <QApplication>
#include <QDir>

#include <phonon/mediaobject.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>
#include <phonon/backendcapabilities.h>

extern "C"
{
    #include <unistd.h>
}


PhononEngine::PhononEngine()
        : EngineBase()
        , m_mediaObject(0)
        , m_audioPath(0)
        , m_audioOutput(0)
{
    debug() << "Yay for Phonon being constructed" << endl;
}

PhononEngine::~PhononEngine()
{
    debug() << "Phonon Engine destroyed!!" << endl;
}

bool
PhononEngine::init()
{
   DEBUG_BLOCK

   debug() << "'Phonon Engine has been sucessfully created.'\n";

    m_mediaObject = new Phonon::MediaObject( this );
    m_audioPath   = new Phonon::AudioPath( this );
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );

   if( !m_mediaObject || !m_audioPath || !m_audioOutput )
   {
       Amarok::StatusBar::instance()->longMessage( "Amarok could not initialize Phonon.",
                                                  KDE::StatusBar::Error );
       return false;
   }
    m_mediaObject->addAudioPath( m_audioPath );
    m_audioPath->addOutput( m_audioOutput );

    connect( m_mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
                              SLOT( convertState( Phonon::State, Phonon::State ) ) );
    connect( m_mediaObject, SIGNAL( finished() ), SLOT( trackEnded() ) );

    connect( m_mediaObject, SIGNAL( length(qint64)), SLOT( length() ) );

   return true;
}

bool
PhononEngine::load( const KUrl &url, bool isStream )
{
    DEBUG_BLOCK

    Amarok::StatusBar::instance()->longMessage( "Loading file" );

    if( Engine::Base::load( url, isStream ) )
    {
        m_mediaObject->setUrl( url );
        return true;
    }
//     m_mediaObject->setUrl( KUrl() );
    return false;
}



bool
PhononEngine::play( uint offset )
{
    DEBUG_BLOCK

    if( m_mediaObject )
    {
        m_mediaObject->play();
        return true;
    }
    Amarok::StatusBar::instance()->longMessage( "Cannot Play File", KDE::StatusBar::Sorry );
    return false;
}

void
PhononEngine::stop()
{
    if( m_mediaObject )
        m_mediaObject->stop();
}

void
PhononEngine::pause()
{
    if( !m_mediaObject )
        m_mediaObject->pause();
}

void
PhononEngine::unpause()
{
    if( !m_mediaObject )
        m_mediaObject->pause();
}

//taken verbatim from noatun
Engine::State PhononEngine::convertState( Phonon::State s )
{
    switch(s)
    {
        case Phonon::PlayingState:
            return Engine::Playing;
        case Phonon::PausedState:
            return Engine::Paused;
        case Phonon::StoppedState:
        case Phonon::BufferingState:
        case Phonon::LoadingState:
            return Engine::Idle;
    }
    return Engine::Empty; //default to this.. possibly a bad idea.
}


Engine::State
PhononEngine::state() const
{
    if(m_mediaObject);
        return PhononEngine::convertState(m_mediaObject->state());
    return Engine::Empty;
}

uint
PhononEngine::position() const
{
    if( state() != Engine::Empty && m_mediaObject )
        return m_mediaObject->currentTime();

    return 0;
}

uint
PhononEngine::length() const
{
    if( m_mediaObject )
        return m_mediaObject->totalTime();

    return 0;
}

void
PhononEngine::seek( uint ms )
{
    if( m_mediaObject && ms >=0 )
        m_mediaObject->seek(ms);
}

void
PhononEngine::setVolumeSW( uint vol )
{
    if( m_audioOutput )
    {
        debug() << "New volume:" << vol << endl;
        m_audioOutput->setVolume( vol );
    }
}

bool
PhononEngine::canDecode( const KUrl &url ) const
{
    //TODO: phonon only offers mime type checks, need to investigate.
    return true;
}

//TODO: Configuration?

// Amarok::PluginConfig*
// XineEngine::configure() const
// {
//     XineConfigDialog* xcf = new XineConfigDialog( m_xine );
//     connect(xcf, SIGNAL( settingsSaved() ), this, SLOT( configChanged() ));
//     connect(this, SIGNAL( resetConfig(xine_t*) ), xcf, SLOT( reset(xine_t*) ));
//     return xcf;
// }
//
// //SLOT
// void XineEngine::configChanged()
// {
//     //reset xine to load new audio plugin
//     if( m_currentAudioPlugin != XineCfg::outputPlugin() )
//     {
//         stop();
//         xine_config_save( m_xine, configPath() );
//         if( m_stream )     xine_close( m_stream );
//         if( m_eventQueue ) xine_event_dispose_queue( m_eventQueue );
//         m_eventQueue = NULL;
//         if( m_stream )     xine_dispose( m_stream );
//         m_stream = NULL;
//         if( m_audioPort )  xine_close_audio_driver( m_xine, m_audioPort );
//         m_audioPort = NULL;
//         if( m_post )       xine_post_dispose( m_xine, m_post );
//         m_post = NULL;
//         if( m_xine )       xine_exit( m_xine );
//         m_xine = NULL;
//         init();
//         setEqualizerEnabled( m_equalizerEnabled );
//         if( m_equalizerEnabled )
//             setEqualizerParameters( m_intPreamp, m_equalizerGains );
//         emit resetConfig(m_xine);
//     }
// }