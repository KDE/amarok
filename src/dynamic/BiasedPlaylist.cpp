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
#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "MetaQueryMaker.h"
#include "playlist/PlaylistModelStack.h"
#include "statusbar/StatusBar.h"

#include <threadweaver/ThreadWeaver.h>
#include <QThread>


// The bigger this is, the more accurate the result will be. Big is good.
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 100;


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


Dynamic::BiasedPlaylist::BiasedPlaylist( QString title, QList<Bias*> biases, Amarok::Collection* collection )
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

    if( m_solver )
    {
        m_solver->requestAbort();

        while( !m_solver->isFinished() )
            usleep( 20000 ); // Sleep 20 msec

        delete m_solver;
    }
}

QDomElement
Dynamic::BiasedPlaylist::xml() const
{
    QDomDocument doc =
        PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc();
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
    if( m_solver )
        m_solver->requestAbort();
}

void
Dynamic::BiasedPlaylist::setContext( Meta::TrackList context )
{
    m_context = context;
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

        m_solver = new BiasSolver( BUFFER_SIZE, m_biases, m_context );
        connect( m_solver, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( solverFinished(ThreadWeaver::Job* ) ) );

        if( withStatusBar )
        {
            The::statusBar()->newProgressOperation( m_solver,  i18n( "Generating playlist..." ) );

            connect( m_solver, SIGNAL(statusUpdate(int)), SLOT(updateStatus(int)) );
        }

        connect( m_solver, SIGNAL(readyToRun()), SLOT(solverReady()) );
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


    if( n <= 0 )
        emit tracksReady( Meta::TrackList() );

    m_requestCache.clear();
    m_numRequested = n;

    handleRequest();
}

void
Dynamic::BiasedPlaylist::recalculate()
{
    DEBUG_BLOCK
    if ( AmarokConfig::dynamicMode() && !m_solver ) {
        m_buffer.clear();
        if ( m_backbufferMutex.tryLock() ) {
            m_backbuffer.clear();
            m_backbufferMutex.unlock();
        }

        getContext();
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

    if( m_buffer.isEmpty() )
    {
        m_backbufferMutex.lock();
        m_buffer = m_backbuffer;
        m_backbuffer.clear();
        startSolver( true );
        m_backbufferMutex.unlock();
    }

    while( m_buffer.size() && m_numRequested-- )
        m_requestCache.append( m_buffer.takeLast() );

    if( m_numRequested <= 0 )
    {
        m_numRequested = 0;
        debug() << "Returning " << m_requestCache.size() << " tracks.";
        emit tracksReady( m_requestCache );
    }
    // otherwise, we ran out of buffer
    else
    {
        m_backbufferMutex.lock();
        m_buffer = m_backbuffer;
        m_backbuffer.clear();
        startSolver( true );
        m_backbufferMutex.unlock();
    }
}


void
Dynamic::BiasedPlaylist::solverFinished( ThreadWeaver::Job* job )
{
    DEBUG_BLOCK

    if( !m_solver )
        return;

    bool success;
    The::statusBar()->endProgressOperation( m_solver );
    m_backbufferMutex.lock();
    m_backbuffer = m_solver->solution();
    m_backbufferMutex.unlock();
    success = m_solver->success();
    job->deleteLater();
    m_solver = 0;

    // empty collection, or it was aborted
    if( !success || m_backbuffer.isEmpty() )
    {
        m_requestCache.clear();
        m_numRequested = 0;

        emit tracksReady( Meta::TrackList() );
    }
    else if( m_numRequested > 0 )
        handleRequest();
}


void
Dynamic::BiasedPlaylist::getContext()
{
    m_context.clear();

    int i = qMax( 0, The::playlist()->activeRow() );

    for( ; i < The::playlist()->rowCount(); ++i )
    {
        m_context.append( The::playlist()->trackAt(i) );
    }
}

