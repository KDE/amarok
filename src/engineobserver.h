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

#include "engine_fwd.h"

class EngineSubject;
class MetaBundle;
class QString;

/**
 * if you want to observe the engine, inherit from this class and attach yourself to
 * the engine with attach
 * Note that all positional information and times are in milliseconds
 */
class EngineObserver
{
public:
    EngineObserver();
    EngineObserver( EngineSubject* );
    virtual ~EngineObserver();
    virtual void engineStateChanged( Engine::State /*state*/, Engine::State /*oldState*/ = Engine::Empty ) {}
    virtual void engineNewMetaData( const MetaBundle &/*bundle*/, bool /*trackChanged*/ ) {}
    virtual void engineTrackEnded( int /*finalPosition*/, int /*trackLength*/, const QString &/*reason*/ ) {}
    virtual void engineVolumeChanged( int /*percent*/ ) {}
    virtual void engineTrackPositionChanged( long /*position*/ , bool /*userSeek*/ ) {}
    virtual void engineTrackLengthChanged( long /*length*/ ) {}

private:
    EngineSubject *m_subject;
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
    void stateChangedNotify( Engine::State /*state*/ );
    void newMetaDataNotify( const MetaBundle &/*bundle*/, bool /*trackChanged*/ );
    void trackEnded( int /*finalPosition*/, int /*trackLength*/, const QString &reason );
    void volumeChangedNotify( int /*percent*/ );
    /* userSeek means the position didn't change due to normal playback */
    void trackPositionChangedNotify( long /*position*/ , bool userSeek=false );
    void trackLengthChangedNotify( long /*length*/ );

private:
    QPtrList<EngineObserver> Observers;
    Engine::State m_oldEngineState;
};

#endif // AMAROK_ENGINEOBSERVER_H
