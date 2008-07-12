/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "BiasedPlaylist.h"
#include "BlockingQuery.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "MetaQueryMaker.h"
#include "playlist/PlaylistModel.h"
#include "StatusBar.h"

#include <threadweaver/ThreadWeaver.h>


// The bigger this is, the more accurate the result will be. Big is good.
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 100;


Dynamic::BiasedPlaylist::BiasedPlaylist( QString title, QList<Bias*> biases )
    : m_biases(biases), m_solver(0)
{
    setTitle( title );
}

Dynamic::BiasedPlaylist::BiasedPlaylist( 
        QString title,
        QList<Bias*> biases, 
        Collection* collection )
    : DynamicPlaylist(collection)
    , m_biases(biases)
    , m_solver(0)
    , m_randomSource(collection)
{
    setTitle( title );
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

    if( !m_solver )
    {
        updateBiases();
        m_solver = new BiasSolver( 
                BUFFER_SIZE, m_biases, &m_randomSource, m_context );
        connect( m_solver, SIGNAL(done(ThreadWeaver::Job*)),
                this, SLOT(solverFinished(ThreadWeaver::Job*)),
                Qt::DirectConnection );

        ThreadWeaver::Weaver::instance()->enqueue( m_solver );
    }

    if( withStatusBar )
    {
        The::statusBar()->newProgressOperation( m_solver );
        The::statusBar()->shortMessage( i18n("Generating playlist...") );

        connect( m_solver, SIGNAL(statusUpdate(int)), SLOT(updateStatus(int)) );
    }
}

void
Dynamic::BiasedPlaylist::updateStatus( int progress )
{
    The::statusBar()->setProgress( m_solver, progress );
}


Dynamic::BiasedPlaylist::~BiasedPlaylist()
{
    if( m_solver )
        delete m_solver;
}


Meta::TrackPtr
Dynamic::BiasedPlaylist::getTrack()
{
    Meta::TrackPtr track;

    if( m_buffer.isEmpty() )
    {
        if( m_backbuffer.isEmpty() )
        {
            // we need it now !
            debug() << "BiasedPlaylist: waiting for results.";
            if( m_context.isEmpty() )
                getContext();
            startSolver( true );
            m_solverLoop.exec();
        }

        m_context = m_buffer;
        m_buffer = m_backbuffer;
        m_backbuffer.clear();
        // start working on the backbuffer again
        startSolver();
    }

    // uh,oh. this means we have an empty collection.
    if( m_buffer.isEmpty() ) return Meta::TrackPtr();

    track = m_buffer.back();
    m_buffer.pop_back();

    return track;
}

void
Dynamic::BiasedPlaylist::recalculate()
{
    m_buffer.clear();
    m_backbuffer.clear();

    getContext();
    startSolver();
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
Dynamic::BiasedPlaylist::solverFinished( ThreadWeaver::Job* job )
{
    The::statusBar()->endProgressOperation( m_solver );
    m_backbuffer = m_solver->solution();
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    m_solver = 0;
    m_solverLoop.exit();
}

void
Dynamic::BiasedPlaylist::updateBiases()
{
    CollectionDependantBias* cb;
    foreach( Bias* b, m_biases )
    {
        if( (cb = dynamic_cast<CollectionDependantBias*>( b ) ) )
        {
            if( cb->needsUpdating() )
                cb->update();
        }
    }
}

void
Dynamic::BiasedPlaylist::getContext()
{
    m_context.clear();

    QList<Playlist::Item*> items = The::playlistModel()->itemList();
    int i = qMax( 0, The::playlistModel()->activeRow() );

    for( ; i < items.size(); ++i )
    {
        m_context.append( items[i]->track() );
    }
}

