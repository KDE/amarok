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
#include "BiasFactory.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h" // for The::playlist
#include "statusbar/StatusBar.h"

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
const int Dynamic::BiasedPlaylist::BUFFER_SIZE = 50;

Dynamic::BiasedPlaylist::BiasedPlaylist( QObject *parent )
    : DynamicPlaylist( parent )
    , m_numRequested( 0 )
{
    // , m_bias( new Dynamic::RandomBias() )
    AndBias *andB = new AndBias();
    BiasPtr biasPtr = BiasPtr( andB );
    andB->appendBias( BiasPtr( new TagMatchBias() ) );
    andB->appendBias( BiasPtr( new NotBias() ) );

    m_title = i18nc( "Title for a default dynamic playlist. The default playlist only returns random tracks.", "Random" );

    biasReplaced( BiasPtr(), biasPtr );
}

Dynamic::BiasedPlaylist::BiasedPlaylist( QXmlStreamReader *reader, QObject *parent )
    : DynamicPlaylist( parent )
    , m_numRequested( 0 )
    , m_bias( 0 )
{
    // Make sure that the BiasedPlaylist instance gets destroyed when App destroys
    setParent( App::instance() );

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "title" )
                m_title = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == "bias" )
            {
                m_bias = Dynamic::BiasFactory::fromXml( reader );
                connect( m_bias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
                         this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );
            }
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
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
    writer->writeStartElement( "bias" );
    writer->writeStartElement( m_bias->name() );
    m_bias->toXml( writer );
    writer->writeEndElement();
    writer->writeEndElement();
}

void
Dynamic::BiasedPlaylist::requestAbort()
{
    DEBUG_BLOCK
    if( m_solver ) {
        m_solver.data()->requestAbort();
        disconnect(m_solver.data(), 0, this, 0);
        m_solver.clear();
    }
}

void
Dynamic::BiasedPlaylist::startSolver( bool withStatusBar )
{
    DEBUG_BLOCK
    debug() << "BiasedPlaylist in:" << QThread::currentThreadId();

    if( !m_solver )
    {
        debug() << "assigning new m_solver";
        m_solver = new BiasSolver( BUFFER_SIZE, m_bias, getContext() );
        m_solver.data()->setAutoDelete(true);
        connect( m_solver.data(), SIGNAL(done(ThreadWeaver::Job*)), SLOT(solverFinished()) );
        connect( m_solver.data(), SIGNAL(failed(ThreadWeaver::Job*)), SLOT(solverFinished()) );

        if( withStatusBar )
        {
            The::statusBar()->newProgressOperation( m_solver.data(), i18n( "Generating playlist..." ) );

            connect( m_solver.data(), SIGNAL(statusUpdate(int)), SLOT(updateStatus(int)) );
        }

        ThreadWeaver::Weaver::instance()->enqueue( m_solver.data() );
        debug() << "called prepareToRun";
    }
    else
        debug() << "solver already running!";
}

void
Dynamic::BiasedPlaylist::biasChanged()
{
    DEBUG_BLOCK;
    emit changed( this );
}

void
Dynamic::BiasedPlaylist::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    Q_UNUSED( oldBias );
    m_bias = newBias;
    connect( m_bias.data(), SIGNAL( changed( Dynamic::BiasPtr ) ),
             this, SLOT( biasChanged() ) );
    connect( m_bias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );
}


void
Dynamic::BiasedPlaylist::updateStatus( int progress )
{
    The::statusBar()->setProgress( m_solver.data(), progress );
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
    debug() << "recalculate" << AmarokConfig::dynamicMode() << "solver?" << m_solver << "requested:" << m_numRequested;
    if( AmarokConfig::dynamicMode() && !m_solver )
    {
        {
            QMutexLocker locker(&m_bufferMutex);
            m_buffer.clear();
        }

        if( m_numRequested > 0 )
            startSolver( true );
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
        startSolver( true );
    }
}


void
Dynamic::BiasedPlaylist::solverFinished()
{
    DEBUG_BLOCK

    if( !m_solver )
        return;

    The::statusBar()->endProgressOperation( m_solver.data() );

    bool success = m_solver.data()->success();
    if( success )
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.append( m_solver.data()->solution() );
    }

    m_solver.data()->deleteLater();

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

