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
//these files are from libamarok
#include "enginecontroller.h"

AMAROK_EXPORT_PLUGIN( PhononEngine )

#include "debug.h"
#include "statusbar/statusbar.h"

#include <phonon/mediaobject.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>
#include <phonon/backendcapabilities.h>


PhononEngine::PhononEngine()
        : EngineBase()
        , m_mediaObject( 0 )
        , m_audioPath  ( 0 )
        , m_audioOutput( 0 )
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

    debug() << "'Phonon Engine has been successfully created.'\n";

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

    //connect( m_mediaObject, SIGNAL( finished() ), SLOT( trackEnded() ) );
    //connect( m_mediaObject, SIGNAL( length(qint64)), SLOT( length() ) );

    return true;
}

bool
PhononEngine::load( const KUrl &url, bool isStream )
{
    DEBUG_BLOCK

    if( Engine::Base::load( url, isStream ) )
    {
        m_mediaObject->setUrl( url );
        return true;
    }

    debug() << "Could not load file " << url.prettyUrl() << endl;
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

        connect( m_mediaObject, SIGNAL( finished() ), SIGNAL( trackEnded() ) );

        emit stateChanged( Engine::Playing );
        return true;
    }

    debug() << "Could not play file " << m_mediaObject->url().prettyUrl() << endl;
    emit stateChanged( Engine::Empty );

    return false;
}

void
PhononEngine::stop()
{
    if( m_mediaObject )
    {
        m_mediaObject->stop();
        emit stateChanged( Engine::Empty );
    }
}

void
PhononEngine::pause()
{
    if( m_mediaObject )
    {
        m_mediaObject->pause();
        emit stateChanged( Engine::Paused );
    }
}

void
PhononEngine::unpause()
{
    if( m_mediaObject )
    {
        m_mediaObject->play();
        emit stateChanged( Engine::Playing );
    }
}

Engine::State
PhononEngine::convertState( Phonon::State s )
{
    Engine::State state;

    switch( s )
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
            state = Engine::Empty;
    }

    return state;
}

Engine::State
PhononEngine::state() const
{
    if( m_mediaObject );
        return convertState( m_mediaObject->state() );

    return Engine::Empty;
}

uint
PhononEngine::position() const
{
    if( state() != Engine::Empty && m_mediaObject ) { 
        uint i = m_mediaObject->currentTime();
        debug() << "Position: " << i << endl;
        return i;
    }

    return 0;
}

uint
PhononEngine::length() const
{
    if( m_mediaObject )
        return ( m_mediaObject->totalTime() / 1000 );

    return 0;
}

void
PhononEngine::seek( uint ms )
{
    if( m_mediaObject )
        m_mediaObject->seek( ms );
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

#include "phonon-engine.moc"
