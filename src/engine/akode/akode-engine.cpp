/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <akode-engine.h>
#include <akode/decoder.h>
#include <akode/player.h>
#include <klocale.h>
#include <qapplication.h>

AMAROK_EXPORT_PLUGIN( AkodeEngine )


namespace Amarok
{
    class Manager : public aKode::Player::Manager
    {
        AkodeEngine *m_engine;

        /// Called for all stateChanges
        virtual void stateChangeEvent( aKode::Player::State )
        {
            QApplication::postEvent( m_engine, new QCustomEvent( 3000 ) );
        }

        /// Called when a decoder reaches end of file
        virtual void eofEvent()
        {
            QApplication::postEvent( m_engine, new QCustomEvent( 3001 ) );
        }

        /// Called when a decoder encounters a fatal error
        virtual void errorEvent()
        {
            QApplication::postEvent( m_engine, new QCustomEvent( 3002 ) );
        }

    public:
        Manager( AkodeEngine *engine ) : m_engine( engine ) {}
    };
}


AkodeEngine::AkodeEngine()
        : m_player( 0 )
{}

AkodeEngine::~AkodeEngine()
{
    if( m_player )
        m_player->close();
}

bool
AkodeEngine::init()
{
//    startTimer( 20 );

    m_player = new aKode::Player();
    m_player->setManager( new Amarok::Manager( this ) );
    m_player->setMonitor( &m_scope );

    return m_player->open( "auto" );
}

bool
AkodeEngine::load( const KURL &url, bool isStream )
{
    Engine::Base::load( url, isStream );

    return m_player->load( url.path().local8Bit().data() );
}

bool
AkodeEngine::play( uint /*offset*/ )
{
    //FIXME this seemed to crash Amarok
    //m_player->decoder()->seek( offset );
    m_player->play();

    return true;
}

void
AkodeEngine::unpause()
{
    m_player->play();
}

bool
AkodeEngine::canDecode( const KURL &url ) const
{
    const QString ext = url.path().right( 4 ).lower();

    return ext == ".mp3" || ext == ".ogg" || ext == ".wav" || ext ==".mpc" || ext == "flac";
}

uint
AkodeEngine::position() const
{
    if( !m_player->decoder() )
        return 0;

    const int pos = m_player->decoder()->position();

    return pos >= 0 ? pos : 0;
}

void
AkodeEngine::stop()
{
    m_player->stop();
    m_player->unload();
}

void
AkodeEngine::pause()
{
    switch( m_player->state() ) {
        case aKode::Player::Playing: m_player->pause(); break;
        case aKode::Player::Paused: m_player->play(); break;
        default: ;
    }
}

void
AkodeEngine::setVolumeSW( uint v )
{
    m_player->setVolume( (float)v / 100.0 );
}

void
AkodeEngine::seek( uint ms )
{
    m_player->decoder()->seek( ms );
}

Engine::State
AkodeEngine::state() const
{
    switch( m_player->state() )
    {
        case aKode::Player::Open:
        case aKode::Player::Closed:  return Engine::Empty;
        default:
        case aKode::Player::Loaded:  return Engine::Idle;
        case aKode::Player::Playing: return Engine::Playing;
        case aKode::Player::Paused:  return Engine::Paused;
    }
}

bool
AkodeEngine::event( QEvent *e )
{
    switch( e->type() )
    {
    /*
    case QEvent::Timer:
        if( m_player->decoder() && m_player->decoder()->eof() ) {
            m_player->stop();
            emit trackEnded();
        }
        break;
    */
    case 3000:
        emit stateChanged( state() );
        break;

    case 3001:
        m_player->stop();
        emit trackEnded();
        break;

    case 3002:
        m_player->stop();
        emit trackEnded();
        emit infoMessage( i18n("Unable to decode <i>%1</i>").arg( m_url.prettyURL()) );
        break;

    default:
        return false;
    }

    return true;
}

const Engine::Scope& AkodeEngine::scope()
{
    return m_scope.scope();
}
