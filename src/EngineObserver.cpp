/***************************************************************************
                      engineobserver.cpp  -  Observer pattern for engine
                         -------------------
begin                : Mar 14 2003
copyright            : (C) 2003 by Frederik Holljen
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

#include "EngineObserver.h"

#include "Debug.h"

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineObserver
//////////////////////////////////////////////////////////////////////////////////////////

EngineObserver::EngineObserver( EngineSubject *s )
    : m_subject( s )
{
    Q_ASSERT( m_subject );
    m_subject->attach( this );
}

EngineObserver::~EngineObserver()
{
    m_subject->detach( this );
}

void
EngineObserver::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    Q_UNUSED( oldState );
    Q_UNUSED( currentState );
}

void
EngineObserver::enginePlaybackEnded( int finalPosition, int trackLength, const QString &reason )
{
    Q_UNUSED( finalPosition );
    Q_UNUSED( trackLength );
    Q_UNUSED( reason );
}

void
EngineObserver::engineNewTrackPlaying()
{
}

void
EngineObserver::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData );
    Q_UNUSED( trackChanged );
}

void
EngineObserver::engineVolumeChanged( int percent )
{
    Q_UNUSED( percent );
}

void
EngineObserver::engineTrackPositionChanged( long position , bool userSeek )
{
    Q_UNUSED( position );
    Q_UNUSED( userSeek );
}

void
EngineObserver::engineTrackLengthChanged( long seconds )
{
    Q_UNUSED( seconds );
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineSubject
//////////////////////////////////////////////////////////////////////////////////////////

EngineSubject::EngineSubject()
    : m_realState( Phonon::StoppedState )
{}

EngineSubject::~EngineSubject()
{
    //do not delete the observers, we don't ahve ownership of them!
}


void EngineSubject::stateChangedNotify( Phonon::State newState, Phonon::State oldState )
{
    DEBUG_BLOCK
    // We explicitly block notifications where newState == buffering in enginecontroller, so if the old state = buffering we can ignore the playing update.
    if( newState == m_realState && newState != Phonon::PlayingState )  // To prevent Playing->Buffering->Playing->buffering.
        return;
    foreach( EngineObserver *observer, Observers )
    {
        observer->engineStateChanged( newState, oldState );
    }
    m_realState = newState;
}

void EngineSubject::playbackEnded( int finalPosition, int trackLength, const QString &reason )
{
    foreach( EngineObserver *observer, Observers )
        observer->enginePlaybackEnded( finalPosition, trackLength, reason );
}

void
EngineSubject::newMetaDataNotify( const QHash<qint64, QString> &newMetaData, bool trackChanged ) const
{
    DEBUG_BLOCK
    foreach( EngineObserver *observer, Observers )
        observer->engineNewMetaData( newMetaData, trackChanged );
}

void EngineSubject::volumeChangedNotify( int percent )
{
    foreach( EngineObserver *observer, Observers )
    {
        observer->engineVolumeChanged( percent );
    }
}


void EngineSubject::trackPositionChangedNotify( long position, bool userSeek )
{
    foreach( EngineObserver *observer, Observers )
    {
        observer->engineTrackPositionChanged( position, userSeek );
    }
}


void EngineSubject::trackLengthChangedNotify( long seconds )
{
    foreach( EngineObserver *observer, Observers )
    {
        observer->engineTrackLengthChanged( seconds );
    }
}

void
EngineSubject::newTrackPlaying() const
{
    foreach( EngineObserver *observer, Observers )
        observer->engineNewTrackPlaying();
}


void EngineSubject::attach( EngineObserver *observer )
{
    if( !observer )
        return;
    Observers.insert( observer );
}


void EngineSubject::detach( EngineObserver *observer )
{
    Observers.remove( observer );
}
