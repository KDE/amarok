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

#include "engineobserver.h"

#include <qptrlist.h>
#include <kdebug.h>

EngineObserver::EngineObserver()
{
    // autoadd to subject in constructor?
}


EngineObserver::~EngineObserver()
{
}


EngineSubject::EngineSubject()
{
}


EngineSubject::~EngineSubject()
{
}


void EngineSubject::stateChangedNotify( EngineBase::EngineState state )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineStateChanged( state );
    }
}


void EngineSubject::newMetaDataNotify( const MetaBundle &bundle, bool trackChanged )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineNewMetaData( bundle, trackChanged );
    }
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


void EngineSubject::trackPositionChangedNotify( long position )
{
    QPtrListIterator<EngineObserver> it( Observers );
    EngineObserver *observer;
    while( ( observer = it.current() ) != 0 )
    {
        ++it;
        observer->engineTrackPositionChanged( position );
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
