/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2010, 2013 Ralf Engels <ralf-engels@gmx.de>                            *
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

#define DEBUG_PREFIX "BiasSolver"

#include "BiasSolver.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "core/collections/QueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <ThreadWeaver/Thread>

#include <QHash>
#include <QMutexLocker>

#include <cmath>

/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be careful */

namespace Dynamic
{

class SolverList
{
    public:

    SolverList( Meta::TrackList trackList,
                int contextCount,
                BiasPtr bias )
        : m_trackList(trackList)
        , m_contextCount( contextCount )
        , m_bias( bias )
    {}

    void appendTrack( Meta::TrackPtr track )
    {
        m_trackList.append( track );
    }

    void removeTrack()
    {
        m_trackList.removeLast();
    }

    SolverList &operator=( const SolverList& x )
    {
        m_trackList = x.m_trackList;
        m_contextCount = x.m_contextCount;
        m_bias = x.m_bias;

        return *this;
    }

    Meta::TrackList m_trackList;
    int m_contextCount; // the number of tracks belonging to the context
    BiasPtr m_bias;
};



BiasSolver::BiasSolver( int n, BiasPtr bias, Meta::TrackList context )
    : m_n( n )
    , m_bias( bias )
    , m_context( context )
    , m_abortRequested( false )
    , m_currentProgress( 0 )
{
    debug() << "CREATING BiasSolver in thread:" << QThread::currentThreadId() << "to get"<<n<<"tracks with"<<context.count()<<"context";

    m_allowDuplicates = AmarokConfig::dynamicDuplicates();

    getTrackCollection();

    connect( m_bias.data(), &Dynamic::AbstractBias::resultReady,
             this, &BiasSolver::biasResultReady );
}


BiasSolver::~BiasSolver()
{
    debug() << "DESTROYING BiasSolver in thread:" << QThread::currentThreadId();
    emit endProgressOperation( this );
}


void
BiasSolver::requestAbort()
{
    m_abortRequested = true;
    emit endProgressOperation( this );
}

bool
BiasSolver::success() const
{
    return !m_abortRequested;
}

void
BiasSolver::setAutoDelete( bool autoDelete )
{
    if( autoDelete )
    {
        if( isFinished() )
            deleteLater();
        connect( this, &BiasSolver::done, this, &BiasSolver::deleteLater );
    }
    else
    {
        disconnect( this, &BiasSolver::done, this, &BiasSolver::deleteLater );
    }
}


void
BiasSolver::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread )
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    DEBUG_BLOCK

    debug() << "BiasSolver::run in thread:" << QThread::currentThreadId();
    m_startTime = QDateTime::currentDateTime();

    // wait until we get the track collection
    {
        QMutexLocker locker( &m_collectionResultsMutex );
        if( !m_trackCollection )
        {
            debug() << "waiting for collection results";
            m_collectionResultsReady.wait( &m_collectionResultsMutex );
        }
        debug() << "collection has" << m_trackCollection->count()<<"uids";
    }

    debug() << "generating playlist";
    SolverList list( m_context, m_context.count(), m_bias );
    addTracks( &list );
    debug() << "found solution"<<list.m_trackList.count()<<"time"<< m_startTime.msecsTo( QDateTime::currentDateTime() );

    m_solution = list.m_trackList.mid( m_context.count() );
//    setFinished( true );
    setStatus(Status_Success);
}

void
BiasSolver::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
BiasSolver::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
BiasSolver::addTracks( SolverList *list )
{
    bool firstTrack = ( list->m_trackList.count() == list->m_contextCount );

    if( m_abortRequested )
        return;

    updateProgress( list );

    if( list->m_trackList.count() >= list->m_contextCount + m_n )
        return; // we have all tracks

    TrackSet set = matchingTracks( list->m_trackList );
    if( !m_allowDuplicates )
        set = withoutDuplicate( list->m_trackList.count(), list->m_trackList, set );

    if( set.trackCount() == 0 )
        return; // no candidates

    // debug() << "addTracks at"<<list->m_trackList.count()<<"candidates:"<<set.trackCount()<<"time"<< m_startTime.msecsTo( QDateTime::currentDateTime() );

    for( int tries = 0; tries < 5 || firstTrack ; tries++ )
    {
        if( m_abortRequested )
            return;

        list->appendTrack( getRandomTrack( set ) );
        addTracks( list ); // add another track recursively
        if( list->m_trackList.count() >= list->m_contextCount + m_n )
            return; // we have all tracks

        // if time is up just try to fill the list as much as possible not cleaning up
        if( m_startTime.msecsTo( QDateTime::currentDateTime() ) > MAX_TIME_MS )
            return;

        list->removeTrack();
    }
}


Meta::TrackList
BiasSolver::solution()
{
    return m_solution;
}


Meta::TrackPtr
BiasSolver::getRandomTrack( const TrackSet& subset ) const
{
    if( subset.trackCount() == 0 )
        return Meta::TrackPtr();

    Meta::TrackPtr track;

    // this is really dumb, but we sometimes end up with uids that don't point to anything
    int giveup = 50;
    while( giveup-- && !track )
        track = trackForUid( subset.getRandomTrack() );

    if( !track )
        error() << "track is 0 in BiasSolver::getRandomTrack()";

    return track;
}

Meta::TrackPtr
BiasSolver::trackForUid( const QString& uid ) const
{
    const QUrl url( uid );
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    if( !track )
        warning() << "trackForUid returned no track for "<<uid;
    return track;
}


// ---- getting the matchingTracks ----

void
BiasSolver::biasResultReady( const TrackSet &set )
{
    QMutexLocker locker( &m_biasResultsMutex );
    m_tracks = set;
    m_biasResultsReady.wakeAll();
}

TrackSet
BiasSolver::matchingTracks( const Meta::TrackList& playlist ) const
{
    QMutexLocker locker( &m_biasResultsMutex );
    m_tracks = m_bias->matchingTracks( playlist,
                                       m_context.count(), m_context.count() + m_n,
                                       m_trackCollection );
    if( m_tracks.isOutstanding() )
        m_biasResultsReady.wait( &m_biasResultsMutex );

    // debug() << "BiasSolver::matchingTracks returns"<<m_tracks.trackCount()<<"of"<<m_trackCollection->count()<<"tracks.";

    return m_tracks;
}

Dynamic::TrackSet
BiasSolver::withoutDuplicate( int position, const Meta::TrackList& playlist,
                              const Dynamic::TrackSet& oldSet )
{
    Dynamic::TrackSet result = Dynamic::TrackSet( oldSet );
    for( int i = 0; i < playlist.count(); i++ )
        if( i != position && playlist[i] )
            result.subtract( playlist[i] );

    return result;
}


// ---- getting the TrackCollection ----

void
BiasSolver::trackCollectionResultsReady( QStringList uids )
{
    m_collectionUids.append( uids );
}

void
BiasSolver::trackCollectionDone()
{
    QMutexLocker locker( &m_collectionResultsMutex );

    m_trackCollection = TrackCollectionPtr( new TrackCollection( m_collectionUids ) );
    m_collectionUids.clear();

    m_collectionResultsReady.wakeAll();
}

void
BiasSolver::getTrackCollection()
{
    // get all the unique ids from the collection manager
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnValue( Meta::valUniqueId );
    qm->setAutoDelete( true );

    connect( qm, &Collections::QueryMaker::newResultReady,
             this, &BiasSolver::trackCollectionResultsReady );
    connect( qm, &Collections::QueryMaker::queryDone,
             this, &BiasSolver::trackCollectionDone );

    qm->run();
}

void
BiasSolver::updateProgress( const SolverList* list )
{
    if( m_n <= 0 )
        return;

    int progress = (int)(100.0 * (double)( list->m_trackList.count() - list->m_contextCount ) / m_n );

    while( m_currentProgress < progress )
    {
        m_currentProgress++;
        emit incrementProgress();
    }
}


}

