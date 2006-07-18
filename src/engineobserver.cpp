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

#include "debug.h"
#include "collectiondb.h"
#include "engineobserver.h"
#include "metabundle.h"
#include "podcastbundle.h"
#include <qptrlist.h>


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineObserver
//////////////////////////////////////////////////////////////////////////////////////////

EngineObserver::EngineObserver()
    : m_subject( 0 )
{}

EngineObserver::EngineObserver( EngineSubject *s )
    : m_subject( s )
{
   m_subject->attach( this );
}

EngineObserver::~EngineObserver()
{
    if ( m_subject )
        m_subject->detach( this );
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS EngineSubject
//////////////////////////////////////////////////////////////////////////////////////////

EngineSubject::EngineSubject()
    : m_oldEngineState( Engine::Empty )
{}

EngineSubject::~EngineSubject()
{}


void EngineSubject::stateChangedNotify( Engine::State state )
{
    DEBUG_BLOCK

    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineStateChanged( state, m_oldEngineState );
    }

    m_oldEngineState = state;
}


void EngineSubject::newMetaDataNotify( const MetaBundle &bundle, bool trackChanged )
{
    DEBUG_BLOCK

    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;

    PodcastEpisodeBundle peb;
    MetaBundle b( bundle );
    if( CollectionDB::instance()->getPodcastEpisodeBundle( bundle.url(), &peb ) )
    {
        b.setPodcastBundle( peb );
    }

    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineNewMetaData( b, trackChanged );
    }
}


void EngineSubject::trackEnded( int finalPosition, int trackLength, const QString &reason )
{
    for( QPtrListIterator<EngineObserver> it( Observers ); *it; ++it )
        (*it)->engineTrackEnded( finalPosition, trackLength, reason );
}


void EngineSubject::volumeChangedNotify( int percent )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineVolumeChanged( percent );
    }
}


void EngineSubject::trackPositionChangedNotify( long position, bool userSeek )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineTrackPositionChanged( position, userSeek );
    }
}


void EngineSubject::trackLengthChangedNotify( long length )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineTrackLengthChanged( length );
    }
}


void EngineSubject::attach( EngineObserver *observer )
{
    if( !observer || Observers.find( observer ) != -1 )
        return;
    Observers.append( observer );
}


void EngineSubject::detach( EngineObserver *observer )
{
    if( Observers.find( observer ) != -1 ) Observers.remove();
}
