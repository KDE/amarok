/***************************************************************************
                      engineobserver.h  -  Observer pattern for engine
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

#ifndef AMAROK_ENGINEOBSERVER_H
#define AMAROK_ENGINEOBSERVER_H

#include "enginebase.h"

class MetaBundle;

/**
 * if you want to observe the engine, inherit from this class and attach yourself to
 * the engine with attach
 */
class EngineObserver
{
public:
    EngineObserver();
    virtual ~EngineObserver();
    virtual void engineStateChanged( EngineBase::EngineState /*state*/ ) {}
    virtual void engineNewMetaData( const MetaBundle &/*bundle*/, bool /*trackChanged*/ ) {}
    virtual void engineVolumeChanged( int /*percent*/ ) {}
    virtual void engineTrackPositionChanged( long /*position*/ ) {}
};

#include <qptrlist.h>
/**
 * Inherited by EngineController.
 * Notify observer functionality is captured in this class.
 */
class EngineSubject
{
public:
    void attach( EngineObserver *observer );
    void detach( EngineObserver *observer );

protected:
    EngineSubject();
    virtual ~EngineSubject();
    void stateChangedNotify( EngineBase::EngineState /*state*/ );
    void newMetaDataNotify( const MetaBundle &/*bundle*/, bool /*trackChanged*/ );
    void volumeChangedNotify( int /*percent*/ );
    void trackPositionChangedNotify( long /*position*/ );

private:
    QPtrList<EngineObserver> Observers;
};

#endif // AMAROK_ENGINEOBSERVER_H
