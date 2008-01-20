/***************************************************************************
 *   Copyright (C) 2007  Dan Meltzer <hydrogen@notyetimplemented.com>      *
 *   Copyright (C) 2007  Seb Ruiz <ruiz@kde.org>                           *
 *   Copyright (C) 2007  Mark Kretschmann <markey@web.de>                  *
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
#include "amarok.h"
#include "amarokconfig.h"
#include "enginecontroller.h"
#include "meta/MetaConstants.h"

AMAROK_EXPORT_PLUGIN( PhononEngine )

#include "debug.h"
#include "statusbar/ContextStatusBar.h"

#include <kmimetype.h>

#include <phonon/mediaobject.h>
#include <phonon/path.h>
#include <phonon/audiooutput.h>
#include <phonon/backendcapabilities.h>
#include <Phonon/VolumeFaderEffect>

#include <QHash>


PhononEngine::PhononEngine()
        : EngineBase()
        , m_mediaObject( 0 )
        , m_audioOutput( 0 )
        , m_fader( 0 )
#ifdef VBR_SEEK_HACK
        , m_usedSeekHack( 0 )
#endif
{
    debug() << "Yay for Phonon being constructed";
}

PhononEngine::~PhononEngine()
{
    debug() << "Phonon Engine destroyed!!";
}

bool
PhononEngine::init()
{
    DEBUG_BLOCK

    debug() << "'Phonon Engine has been successfully created.'\n";

    m_mediaObject = new Phonon::MediaObject( this );
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );

    m_mediaObject->setTickInterval( 100 ); // Needed for position() to work

    m_path = Phonon::createPath(m_mediaObject, m_audioOutput);

    connect( m_mediaObject, SIGNAL( finished() ), SIGNAL( trackEnded() ) );
    //connect( m_mediaObject, SIGNAL( length(qint64)), SLOT( length() ) );
    connect( m_mediaObject, SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );

    return true;
}

bool
PhononEngine::load( const KUrl &url, bool isStream )
{
    DEBUG_BLOCK

    Engine::Base::load( url, isStream );
    m_mediaObject->setCurrentSource( url );

    return true;
}

bool
PhononEngine::play( uint offset )
{
    DEBUG_BLOCK

    delete m_fader;
    m_mediaObject->pause();
    seek( offset );
    m_mediaObject->play();
    emit stateChanged( Engine::Playing );

    return true;
}

void
PhononEngine::stop()
{
    DEBUG_BLOCK

    m_mediaObject->stop();
    emit stateChanged( Engine::Empty );
}

void
PhononEngine::pause()
{
    DEBUG_BLOCK

    m_mediaObject->pause();
    emit stateChanged( Engine::Paused );
}

void
PhononEngine::unpause()
{
    DEBUG_BLOCK

    m_mediaObject->play();
    emit stateChanged( Engine::Playing );
}

void
PhononEngine::beginFadeOut()
{
    if( m_fader )
    {
        return;
    }
    //this code causes a crash in phonon code in insertEffect
    //i haven't had time to ask the phonon guys about it yet
    //but the code *seems* to be right - max
    /*m_fader = new Phonon::VolumeFaderEffect( this );
    m_path.insertEffect( m_fader );
    m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );
    m_fader->fadeOut( AmarokConfig::fadeoutLength() );*/
}

Engine::State
PhononEngine::convertState( Phonon::State s ) const
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
            state = m_url.isEmpty() ? Engine::Empty : Engine::Idle;
    }

    return state;
}

Engine::State
PhononEngine::state() const
{
    if( m_mediaObject )
        return convertState( m_mediaObject->state() );

    return Engine::Empty;
}

uint
PhononEngine::position() const
{
    if( state() != Engine::Empty )
    {
#ifdef VBR_SEEK_HACK
        if( m_usedSeekHack )
        {
            Meta::TrackPtr track = EngineController::instance()->currentTrack();
            if( track )
            {
                // work out the amount of real time since the seek, plus the seek position in real time
                return m_mediaObject->currentTime() - m_usedSeekHack + ((double)m_usedSeekHack * track->length()) / (length() / 1000.0);
            }
        }
#endif
        return m_mediaObject->currentTime();
    }

    return 0;
}

uint
PhononEngine::length() const
{
    const uint t = ( m_mediaObject->totalTime() == -1 ) ? 0 : m_mediaObject->totalTime();
    return t;
}

void
PhononEngine::seek( uint ms )
{
#ifdef VBR_SEEK_HACK
    m_usedSeekHack = 0;
    if( ms != 0 )
    {
        Meta::TrackPtr track = EngineController::instance()->currentTrack();
        if( track )
        {
            ms = ((double)ms * length()) / (track->length() * 1000.0);
            m_usedSeekHack = ms;
        }
    }
#endif
    m_mediaObject->seek( ms );
}

void
PhononEngine::setVolumeSW( uint volume )
{
    const float v = volume * 0.01;
    m_audioOutput->setVolume( v );
}

bool
PhononEngine::canDecode( const KUrl &url ) const
{
    const QString mimeType = KMimeType::findByUrl( url, 0, false, true )->name();

    return Phonon::BackendCapabilities::isMimeTypeAvailable( mimeType );
}

void
PhononEngine::slotMetaDataChanged()
{
    QHash<qint64, QString> meta;
    {
        QStringList data = m_mediaObject->metaData( "ARTIST" );
        if( !data.isEmpty() )
            meta.insert( Meta::valArtist, data.first() );
    }
    {
        QStringList data = m_mediaObject->metaData( "ALBUM" );
        if( !data.isEmpty() )
            meta.insert( Meta::valAlbum, data.first() );
    }
    {
        QStringList data = m_mediaObject->metaData( "TITLE" );
        if( !data.isEmpty() )
            meta.insert( Meta::valTitle, data.first() );
    }
    {
        QStringList data = m_mediaObject->metaData( "GENRE" );
        if( !data.isEmpty() )
            meta.insert( Meta::valGenre, data.first() );
    }
    {
        QStringList data = m_mediaObject->metaData( "TRACKNUMBER" );
        if( !data.isEmpty() )
            meta.insert( Meta::valTrackNr, data.first() );
    }
    {
        QStringList data = m_mediaObject->metaData( "LENGTH" );
        if( !data.isEmpty() )
            meta.insert( Meta::valLength, data.first() );
    }
    emit metaData( meta );
}

#include "phonon-engine.moc"
