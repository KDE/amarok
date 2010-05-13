/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "BiasedPlaylist"

#include "BiasedPlaylist.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "DynamicModel.h"
#include "core/collections/MetaQueryMaker.h"
#include "playlist/PlaylistModelStack.h"
#include "statusbar/StatusBar.h"

#include <threadweaver/ThreadWeaver.h>
#include <QThread>


// The bigger this is, the more accurate the result will be. Big is good.
// On the other hand optimizing a ridiculous large playlist for nothing
// is just a waste of processing power.
// expecially since it seems that the buffers are deleted
// every time the playlist changes (e.g. after the rating changed)
// So Small is good.
// Pick your poison...
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 50;


Dynamic::BiasedPlaylist*
Dynamic::BiasedPlaylist::fromXml( QDomElement e )
{
    if( e.tagName() != "playlist" )
        return 0;

    QString title = e.attribute( "title" );
    QList<Dynamic::Bias*> biases;

    for( int j = 0; j < e.childNodes().size(); ++j )
    {
        if( !e.childNodes().at(j).isElement() )
            continue;

        QDomElement e2 = e.childNodes().at(j).toElement();
        if( e2.tagName() == "bias" )
            biases.append( Dynamic::Bias::fromXml( e2 ) );
    }

    return new Dynamic::BiasedPlaylist( title, biases );
}

QString
Dynamic::BiasedPlaylist::nameFromXml( QDomElement e )
{
    if( e.tagName() != "playlist" )
        return 0;

    return e.attribute( "title" );
}


Dynamic::BiasedPlaylist::BiasedPlaylist( QString title, QList<Bias*> biases, Collections::Collection* collection )
    : DynamicPlaylist(collection)
    , m_numRequested( 0 )
    , m_biases( biases )
{
    setTitle( title );

    // Make sure that the BiasedPlaylist instance gets destroyed when App destroys
    setParent( App::instance() );
}


Dynamic::BiasedPlaylist::~BiasedPlaylist()
{
    DEBUG_BLOCK

    requestAbort();
}

QDomElement
Dynamic::BiasedPlaylist::xml() const
{
    QDomDocument doc;
    QDomElement e = doc.createElement( "playlist" );
    e.setAttribute( "title", m_title );

    foreach( Bias* b, m_biases )
    {
        e.appendChild( b->xml() );
    }

    return e;
}

void
Dynamic::BiasedPlaylist::requestAbort()
{
    if( m_solver ) {
        m_solver->requestAbort();
        disconnect(m_solver, 0, this, 0);
        m_solver = 0;
    }
}

void
Dynamic::BiasedPlaylist::startSolver( bool withStatusBar )
{
    DEBUG_BLOCK
    debug() << "BiasedPlaylist in:" << QThread::currentThreadId();

    if( !m_solver )
    {
        BiasSolver::setUniverseCollection( m_collection );
        debug() << "assigning new m_solver";

        m_solver = new BiasSolver( BUFFER_SIZE, m_biases, getContext() );
        m_solver->setAutoDelete(true);
        connect( m_solver, SIGNAL(readyToRun()), SLOT(solverReady()) );
        connect( m_solver, SIGNAL(done(ThreadWeaver::Job*)), SLOT(solverFinished()) );
        connect( m_solver, SIGNAL(failed(ThreadWeaver::Job*)), SLOT(solverFinished()) );

        if( withStatusBar )
        {
            The::statusBar()->newProgressOperation( m_solver,  i18n( "Generating playlist..." ) );

            connect( m_solver, SIGNAL(statusUpdate(int)), SLOT(updateStatus(int)) );
        }

        m_solver->prepareToRun();
        debug() << "called prepareToRun";
    }
    else
        debug() << "solver already running!";
}

void
Dynamic::BiasedPlaylist::solverReady()
{
    debug() << "ENQUEUEING new m_solver!" << m_solver;
    if( m_solver )
        ThreadWeaver::Weaver::instance()->enqueue( m_solver );
}


void
Dynamic::BiasedPlaylist::updateStatus( int progress )
{
    The::statusBar()->setProgress( m_solver, progress );
}

void
Dynamic::BiasedPlaylist::requestTracks( int n )
{
    debug() << "Requesting " << n << " tracks.";

    {
        QMutexLocker locker(&m_bufferMutex);
        m_numRequested = n;
    }
    handleRequest();
}

void
Dynamic::BiasedPlaylist::recalculate()
{
    DEBUG_BLOCK
    if ( AmarokConfig::dynamicMode() && !m_solver ) {
        {
            QMutexLocker locker(&m_bufferMutex);
            m_buffer.clear();
        }

        if( m_numRequested > 0 )
            startSolver( true );
    }
}

void
Dynamic::BiasedPlaylist::invalidate()
{
    DEBUG_BLOCK
     if ( AmarokConfig::dynamicMode() )
     {
         BiasSolver::outdateUniverse();
         if( m_active )
            recalculate();
     }
}

QList<Dynamic::Bias*>&
Dynamic::BiasedPlaylist::biases()
{
    return m_biases;
}

const QList<Dynamic::Bias*>&
Dynamic::BiasedPlaylist::biases() const
{
    return m_biases;
}

void
Dynamic::BiasedPlaylist::handleRequest()
{
    DEBUG_BLOCK

    QMutexLocker locker(&m_bufferMutex);

    // if we have enough tracks, submit them.
    if( m_buffer.count() >= m_numRequested )
    {
        Meta::TrackList tracks;
        while( !m_buffer.isEmpty() && m_numRequested-- )
            tracks.append( m_buffer.takeFirst() );
        locker.unlock();

        debug() << "Returning " << tracks.size() << " tracks.";
        emit tracksReady( tracks );
    }
    else
    {
        locker.unlock();
        // otherwise, we ran out of buffer
        startSolver( true );
    }
}


void
Dynamic::BiasedPlaylist::solverFinished()
{
    DEBUG_BLOCK

    if( !m_solver )
        return;

    The::statusBar()->endProgressOperation( m_solver );

    bool success = m_solver->success();
    if( success )
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.append( m_solver->solution() );
    }

    m_solver = 0;

    // empty collection just give up.
    if(m_buffer.isEmpty())
    {
        m_numRequested = 0;
    }

    handleRequest();
}


Meta::TrackList
Dynamic::BiasedPlaylist::getContext()
{
    Meta::TrackList context;

    int i = qMax( 0, The::playlist()->activeRow() );

    for( ; i < The::playlist()->qaim()->rowCount(); ++i )
    {
        context.append( The::playlist()->trackAt(i) );
    }

    {
        QMutexLocker locker(&m_bufferMutex);
        context.append( m_buffer );
    }

    return context;
}

