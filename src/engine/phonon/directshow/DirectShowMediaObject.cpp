/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-directshow"

#include "DirectShowMediaObject.h"
#include "DirectShowGraph.h"

#include <kpluginfactory.h>


DirectShowMediaObject::DirectShowMediaObject(QObject *parent)
    : QObject( parent ),
      m_graph( 0 )
{
}


DirectShowMediaObject::~DirectShowMediaObject()
{
}


void 
DirectShowMediaObject::play()
{
    if( m_graph )
        m_graph->play();
}


void 
DirectShowMediaObject::pause()
{
    if( m_graph )
        m_graph->pause();
}


void
DirectShowMediaObject::stop()
{
    if( m_graph )
        m_graph->stop();
}


void 
DirectShowMediaObject::seek(qint64 milliseconds)
{
    if( m_graph )
        m_graph->seek( milliseconds );
}


// amarok works fine without an implementation of these tick functions
qint32 
DirectShowMediaObject::tickInterval() const
{
    return 0;
}


void 
DirectShowMediaObject::setTickInterval(qint32 interval)
{
}


// don't currently support video
bool 
DirectShowMediaObject::hasVideo() const
{
    return false;
}


bool
DirectShowMediaObject::isSeekable() const
{
    return m_graph && m_graph->isSeekable();
}


qint64 
DirectShowMediaObject::currentTime() const
{
    return m_graph ? m_graph->currentTime() : 0;
}


Phonon::State 
DirectShowMediaObject::state() const
{
    return m_graph ? m_graph->state() : Phonon::StoppedState;
}


qint64 
DirectShowMediaObject::totalTime() const
{
    return m_graph ? m_graph->totalTime() : 0;
}


QString 
DirectShowMediaObject::errorString() const
{
    return m_graph ? m_graph->errorString() : "";
}


Phonon::ErrorType 
DirectShowMediaObject::errorType() const
{
    return m_graph ? m_graph->errorType() : Phonon::NoError;
}


Phonon::MediaSource 
DirectShowMediaObject::source() const
{
    return m_graph->source();
}


void 
DirectShowMediaObject::setSource(const Phonon::MediaSource &source)
{
    if( m_graph )
        m_graph->setSource( source );
}


// don't currently support pre-queueing sources
void 
DirectShowMediaObject::setNextSource(const Phonon::MediaSource &source)
{
}


// amarok doesn't use the prefinish notification, so it's not currently implemented
qint32 
DirectShowMediaObject::prefinishMark() const
{
    return 0;
}


void 
DirectShowMediaObject::setPrefinishMark(qint32)
{
}


// amarok doesn't use the transition, so it's not currently implemented
qint32 
DirectShowMediaObject::transitionTime() const
{
    return 0;
}


void 
DirectShowMediaObject::setTransitionTime(qint32)
{
}


void
DirectShowMediaObject::setGraph( DirectShowGraph *graph )
{
    m_graph = graph;

    if( m_graph )
    {
        // pass through all graph signals
        connect( m_graph, SIGNAL( aboutToFinish() ), SIGNAL( aboutToFinish() ) );
        connect( m_graph, SIGNAL( finished() ), SIGNAL( finished() ) );
        connect( m_graph, SIGNAL( prefinishMarkReached(qint32) ), SIGNAL( prefinishMarkReached(qint32) ) );
        connect( m_graph, SIGNAL( totalTimeChanged(qint64) ), SIGNAL( prefinishMarkReached(qint32) ) );
        connect( m_graph, SIGNAL( currentSourceChanged(const Phonon::MediaSource &) ), SIGNAL( currentSourceChanged(const Phonon::MediaSource &) ) );

        connect( m_graph, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SIGNAL( stateChanged( Phonon::State, Phonon::State ) ) );
        connect( m_graph, SIGNAL( tick(qint64) ), SIGNAL( tick(qint64) ) );
        connect( m_graph, SIGNAL( metaDataChanged(const QMultiMap<QString, QString> &) ), SIGNAL( metaDataChanged(const QMultiMap<QString, QString> &) ) );
        connect( m_graph, SIGNAL( seekableChanged(bool) ), SIGNAL( seekableChanged(bool) ) );
        connect( m_graph, SIGNAL( hasVideoChanged(bool) ), SIGNAL( hasVideoChanged(bool) ) );
        connect( m_graph, SIGNAL( bufferStatus(int) ), SIGNAL( bufferStatus(int) ) );
    }
}
