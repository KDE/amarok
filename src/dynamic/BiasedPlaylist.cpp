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


#include "Debug.h"
#include "BiasedPlaylist.h"
#include "Collection.h"


// The bigger this is, the more accurate the result will be. Big is good.
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 100;



Dynamic::BiasedPlaylist::BiasedPlaylist( 
        QString title,
        QList<Bias*> biases, 
        Collection* collection )
    : m_biases(biases), m_collection(collection)
{
    setTitle( title );

    connect( collection, SIGNAL(updated()), this, SLOT(collectionUpdated()) );
    
    m_solver = new BiasSolver( BUFFER_SIZE, m_biases, m_collection );
    connect( m_solver, SIGNAL(finished()), this, SLOT(solverFinished()) );
    m_solver->start();
}


Dynamic::BiasedPlaylist::~BiasedPlaylist()
{
    if( m_solver )
        delete m_solver;
}


Meta::TrackPtr
Dynamic::BiasedPlaylist::getTrack()
{
    DEBUG_BLOCK

    Meta::TrackPtr track;

    if( m_buffer.isEmpty() )
    {
        if( m_backbuffer.isEmpty() )
        {
            // we need it now !
            m_solver->wait();
            m_buffer = m_solver->solution();

            // now for the backbuffer
            m_solver->start();
        }
        else
        {
            m_buffer = m_backbuffer;
            m_backbuffer.clear();
            m_solver->start();
        }
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
    // dump everything in buffer
    m_buffer.clear();
    m_backbuffer.clear();

    if( !m_solver->isRunning() ) m_solver->start();
}


void
Dynamic::BiasedPlaylist::solverFinished()
{
    m_backbuffer = m_solver->solution();
}

void 
Dynamic::BiasedPlaylist::collectionUpdated()
{
    m_buffer.clear();
    m_backbuffer.clear();

    m_solver->terminate();
    m_solver->start();
}


