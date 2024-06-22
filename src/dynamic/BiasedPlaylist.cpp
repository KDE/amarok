/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "App.h"
#include "amarokconfig.h"
#include "core/collections/Collection.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "dynamic/BiasSolver.h"
#include "dynamic/BiasFactory.h"
#include "dynamic/DynamicModel.h"
#include "playlist/PlaylistModelStack.h" // for The::playlist

#include <QThread>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Dynamic::BiasedPlaylist::BiasedPlaylist( QObject *parent )
    : DynamicPlaylist( parent )
    , m_bias( nullptr )
    , m_solver( nullptr )
{
    m_title = i18nc( "Title for a default dynamic playlist. The default playlist only returns random tracks.", "Random" );

    BiasPtr biasPtr( BiasPtr( new Dynamic::RandomBias() ) );
    biasReplaced( BiasPtr(), biasPtr );
}

Dynamic::BiasedPlaylist::BiasedPlaylist( QXmlStreamReader *reader, QObject *parent )
    : DynamicPlaylist( parent )
    , m_bias( nullptr )
    , m_solver( nullptr )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringView name = reader->name();
            if( name == QStringLiteral("title") )
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
    writer->writeTextElement( QStringLiteral("title"), m_title );
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
        m_solver = nullptr;
    }
}

void
Dynamic::BiasedPlaylist::startSolver( int numRequested )
{
    DEBUG_BLOCK
    debug() << "BiasedPlaylist in:" << QThread::currentThreadId();

    if( !m_solver )
    {
        debug() << "assigning new m_solver";
        m_solver = new BiasSolver( numRequested, m_bias, getContext() );
        connect( m_solver, &BiasSolver::done, this, &BiasedPlaylist::solverFinished );

        Amarok::Logger::newProgressOperation( m_solver,
                                                            i18n( "Generating playlist..." ), 100,
                                                            this, &BiasedPlaylist::requestAbort );

        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(m_solver) );

        debug() << "called prepareToRun";
    }
    else
        debug() << "solver already running!";
}

void
Dynamic::BiasedPlaylist::biasChanged()
{
    Q_EMIT changed( this );
    bool inModel = DynamicModel::instance()->index( this ).isValid();
    if( inModel )
        DynamicModel::instance()->biasChanged( m_bias );
}

void
Dynamic::BiasedPlaylist::biasReplaced( const Dynamic::BiasPtr &oldBias, const Dynamic::BiasPtr &newBias )
{
    if( oldBias && !newBias ) // don't move the last bias away from this playlist without replacement
        return;

    bool inModel = DynamicModel::instance()->index( this ).isValid();
    if( m_bias )
    {
        disconnect( m_bias.data(), nullptr, this, nullptr );

        if( inModel )
            Dynamic::DynamicModel::instance()->beginRemoveBias( this );
        m_bias = nullptr;
        if( inModel )
            Dynamic::DynamicModel::instance()->endRemoveBias();
    }

    if( inModel )
        Dynamic::DynamicModel::instance()->beginInsertBias( this );
    m_bias = newBias;
    if( inModel )
        Dynamic::DynamicModel::instance()->endInsertBias();

    connect( m_bias.data(), &AbstractBias::changed,
             this, &BiasedPlaylist::biasChanged );
    connect( m_bias.data(), &AbstractBias::replaced,
             this, &BiasedPlaylist::biasReplaced );

    if( oldBias ) // don't Q_EMIT a changed during construction
        biasChanged();
}

void
Dynamic::BiasedPlaylist::requestTracks( int n )
{
    if( n > 0 )
        startSolver( n + 1 ); // we request an additional track so that we don't end up in a position that e.g. does have no "similar" track.
}

Dynamic::BiasPtr
Dynamic::BiasedPlaylist::bias() const
{
    return m_bias;
}

void
Dynamic::BiasedPlaylist::solverFinished()
{
    DEBUG_BLOCK

    if( m_solver != sender() )
        return; // maybe an old solver... aborted solvers should autodelete

    Meta::TrackList list = m_solver->solution();
    if( list.count() > 0 )
    {
        // remove the additional requested track
        if( list.count() > 1 )
            list.removeLast();
        Q_EMIT tracksReady( list );
    }

    m_solver->deleteLater();
    m_solver = nullptr;
}


Meta::TrackList
Dynamic::BiasedPlaylist::getContext()
{
    Meta::TrackList context = The::playlist()->tracks();

    return context;
}

