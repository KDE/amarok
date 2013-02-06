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

#include "BiasSolver.h"
#include "BiasedPlaylist.h"
#include "BiasFactory.h"
#include "DynamicModel.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h" // for The::playlist


#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <threadweaver/ThreadWeaver.h>
#include <QThread>


// The bigger this is, the more accurate the result will be. Big is good.
// On the other hand optimizing a ridiculous large playlist for nothing
// is just a waste of processing power.
// expecially since it seems that the buffers are deleted
// every time the playlist changes (e.g. after the rating changed)
// So Small is good.
// Pick your poison...
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 30;

Dynamic::BiasedPlaylist::BiasedPlaylist( QObject *parent )
    : DynamicPlaylist( parent )
    , m_numRequested( 0 )
    , m_bias( 0 )
    , m_solver( 0 )
{
    m_title = i18nc( "Title for a default dynamic playlist. The default playlist only returns random tracks.", "Random" );

    BiasPtr biasPtr( BiasPtr( new Dynamic::RandomBias() ) );
    biasReplaced( BiasPtr(), biasPtr );
}

Dynamic::BiasedPlaylist::BiasedPlaylist( QXmlStreamReader *reader, QObject *parent )
    : DynamicPlaylist( parent )
    , m_numRequested( 0 )
    , m_bias( 0 )
    , m_solver( 0 )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "title" )
                m_title = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else
            {
                BiasPtr biasPtr( Dynamic::BiasFactory::fromXml( reader ) );
                if( biasPtr )
                {
                    biasReplaced( BiasPtr(), biasPtr );
                }
                else
                {
                    debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                    reader->skipCurrentElement();
                }
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

Dynamic::BiasedPlaylist::~BiasedPlaylist()
{
    requestAbort();
}

void
Dynamic::BiasedPlaylist::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "title", m_title );
    writer->writeStartElement( m_bias->name() );
    m_bias->toXml( writer );
    writer->writeEndElement();
}

void
Dynamic::BiasedPlaylist::requestAbort()
{
    DEBUG_BLOCK
    if( m_solver ) {
        m_solver->setAutoDelete( true );
        m_solver->requestAbort();
        m_solver = 0;
    }
}

void
Dynamic::BiasedPlaylist::startSolver()
{
    DEBUG_BLOCK
    debug() << "BiasedPlaylist in:" << QThread::currentThreadId();

    if( !m_solver )
    {
        debug() << "assigning new m_solver";
        m_solver = new BiasSolver( BUFFER_SIZE, m_bias, getContext() );
        connect( m_solver, SIGNAL(done(ThreadWeaver::Job*)), SLOT(solverFinished()) );

        Amarok::Components::logger()->newProgressOperation( m_solver,
                                                            i18n( "Generating playlist..." ), 100,
                                                            this, SLOT(requestAbort()) );

        ThreadWeaver::Weaver::instance()->enqueue( m_solver );
        debug() << "called prepareToRun";
    }
    else
        debug() << "solver already running!";
}

void
Dynamic::BiasedPlaylist::biasChanged()
{
    QMutexLocker locker( &m_bufferMutex );
    m_buffer.clear();

    emit changed( this );
    bool inModel = DynamicModel::instance()->index( this ).isValid();
    if( inModel )
        DynamicModel::instance()->biasChanged( m_bias );
}

void
Dynamic::BiasedPlaylist::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    if( oldBias && !newBias ) // don't move the last bias away from this playlist without replacement
        return;

    bool inModel = DynamicModel::instance()->index( this ).isValid();
    if( m_bias )
    {
        disconnect( m_bias.data(), 0, this, 0 );

        if( inModel )
            Dynamic::DynamicModel::instance()->beginRemoveBias( this );
        m_bias = 0;
        if( inModel )
            Dynamic::DynamicModel::instance()->endRemoveBias();
    }

    if( inModel )
        Dynamic::DynamicModel::instance()->beginInsertBias( this );
    m_bias = newBias;
    if( inModel )
        Dynamic::DynamicModel::instance()->endInsertBias();

    connect( m_bias.data(), SIGNAL( changed( Dynamic::BiasPtr ) ),
             this, SLOT( biasChanged() ) );
    connect( m_bias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );

    if( oldBias ) // don't emit a changed during construction
        biasChanged();
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
Dynamic::BiasedPlaylist::repopulate()
{
    DEBUG_BLOCK
    debug() << "repopulate" << AmarokConfig::dynamicMode() << "solver?" << m_solver << "requested:" << m_numRequested;
    if( AmarokConfig::dynamicMode() && !m_solver )
    {
        {
            QMutexLocker locker(&m_bufferMutex);
            m_buffer.clear();
        }

        if( m_numRequested > 0 )
            startSolver();
    }
}

Dynamic::BiasPtr
Dynamic::BiasedPlaylist::bias() const
{
    return m_bias;
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
        startSolver();
    }
}


void
Dynamic::BiasedPlaylist::solverFinished()
{
    DEBUG_BLOCK

    if( m_solver != sender() )
        return; // maybe an old solver... aborted solvers should autodelete

    bool success = m_solver->success();
    if( success )
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.append( m_solver->solution() );
    }
    m_solver->deleteLater();
    m_solver = 0;

    // empty collection just give up.
    if(m_buffer.isEmpty())
        m_numRequested = 0;

    handleRequest();
}


Meta::TrackList
Dynamic::BiasedPlaylist::getContext()
{
    Meta::TrackList context = The::playlist()->tracks();

    {
        QMutexLocker locker(&m_bufferMutex);
        context.append( m_buffer );
    }

    return context;
}

