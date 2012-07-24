/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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
#include "TrackSet.h"

#include "amarokconfig.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"

#include <cmath>

#include <QHash>
#include <QMutexLocker>

#include <KRandom>


/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be careful */
#include <threadweaver/Thread.h>

namespace Dynamic
{

const int    BiasSolver::GA_ITERATION_LIMIT         = 70;
const int    BiasSolver::GA_POPULATION_SIZE         = 15;
const int    BiasSolver::GA_MATING_POPULATION_SIZE  = 5;
const double BiasSolver::GA_MUTATION_PROBABILITY    = 0.05;
const int    BiasSolver::GA_GIVE_UP_LIMIT           = 10;

const int    BiasSolver::SA_ITERATION_LIMIT     = 1000;
const double BiasSolver::SA_INITIAL_TEMPERATURE = 0.28;
const double BiasSolver::SA_COOLING_RATE        = 0.82;
const int    BiasSolver::SA_GIVE_UP_LIMIT       = 250;

class SolverList
{
    public:

    SolverList( Meta::TrackList trackList,
                int contextCount,
                BiasPtr bias )
        : m_trackList(trackList)
        , m_contextCount( contextCount )
        , m_bias( bias )
        , m_energyValid( false )
    {}

    void appendTrack( Meta::TrackPtr track )
    {
        m_trackList.append( track );
        m_energyValid = false;
    }

    void setTrack( int pos, Meta::TrackPtr track )
    {
        m_trackList.replace( pos, track );
        m_energyValid = false;
    }

    bool operator<( const SolverList& x ) const
    { return energy() < x.energy(); }

    SolverList &operator=( const SolverList& x )
    {
        m_trackList = x.m_trackList;
        m_energyValid = x.m_energyValid;
        m_energy = x.m_energy;
        m_contextCount = x.m_contextCount;

        return *this;
    }

    double energy() const
    {
        if( !m_energyValid )
        {
            m_energy = m_bias->energy( m_trackList, m_contextCount );
            m_energyValid = true;
        }
        return m_energy;
    }

    Meta::TrackList m_trackList;
    int m_contextCount; // the number of tracks belonging to the context
    BiasPtr m_bias;

private:
    mutable bool m_energyValid;
    mutable double m_energy;
};



BiasSolver::BiasSolver( int n, BiasPtr bias, Meta::TrackList context )
    : m_n( n )
    , m_bias( bias )
    , m_context( context )
    , m_abortRequested( false )
{
    debug() << "CREATING BiasSolver in thread:" << QThread::currentThreadId() << "to get"<<n<<"tracks with"<<context.count()<<"context";

    m_allowDuplicates = AmarokConfig::dynamicDuplicates();

    getTrackCollection();

    connect( m_bias.data(), SIGNAL( resultReady( const Dynamic::TrackSet & ) ),
             this,  SLOT( biasResultReady( const Dynamic::TrackSet & ) ) );
}


BiasSolver::~BiasSolver()
{
    debug() << "DESTROYING BiasSolver in thread:" << QThread::currentThreadId();
}


void
BiasSolver::requestAbort()
{
    m_abortRequested = true;
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
        connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
        connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
    }
    else
    {
        disconnect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
        disconnect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
    }
}


void
BiasSolver::run()
{
    DEBUG_BLOCK

    debug() << "BiasSolver::run in thread:" << QThread::currentThreadId();

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

    /*
     * Two stage solver: Run ga_optimize and feed it's result into sa_optimize.
     *
     * Rationale: Genetic algorithms take better advantage of the heuristic used
     * by generateInitialPlaylist and also have tendency to converge faster
     * initially. How ever, they also tend to get stuck in local minima unless
     * the population size is quite large, which is why we switch over to
     * simulated annealing when that happens.
     */

    /*
     * NOTE: For now I am disabling the ga phase, until I can do more
     * experimentation.
     */
    //Meta::TrackList playlist = ga_optimize( GA_ITERATION_LIMIT, true );

    debug() << "generating playlist";
    SolverList playlist = generateInitialPlaylist();
    debug() << "got playlist with"<<playlist.energy();

    // actually for now the simple optimize finds a perfect solution in many cases
    simpleOptimize( &playlist );
    debug() << "after simple optimize playlist with"<<playlist.energy();

    if( playlist.energy() > epsilon() && !m_abortRequested ) // the playlist is only slightly wrong
    {
        annealingOptimize( &playlist, SA_ITERATION_LIMIT, true );
    }
    debug() << "Found solution with energy"<<playlist.energy();

    m_solution = playlist.m_trackList.mid( m_context.count() );
}

void
BiasSolver::simpleOptimize( SolverList *list )
{
    DEBUG_BLOCK;

    if( list->m_trackList.count() <= list->m_contextCount )
        return; // nothing to optimize. All tracks are in the context

    // first set some random tracks
    // this prevents the part bias from first fullfilling the easy conditions
    for( int i = 0; i < m_n / 2; i++ )
    {
        // choose the mutation position
        int newPos = (KRandom::random() % (list->m_trackList.count() - list->m_contextCount))
            + list->m_contextCount;

        TrackSet set = matchingTracks( newPos, list->m_trackList );
        if( !m_allowDuplicates )
            set = withoutDuplicate( newPos, list->m_trackList, set, false );

        Meta::TrackPtr newTrack;
        newTrack = getRandomTrack( set );
        if( newTrack )
            list->setTrack( newPos, newTrack );
    }

    // now go through the complete list again and try to fullfill all
    for( int newPos = list->m_contextCount; newPos < list->m_trackList.count(); newPos++ )
    {
        TrackSet set = matchingTracks( newPos, list->m_trackList );
        if( !m_allowDuplicates )
            set = withoutDuplicate( newPos, list->m_trackList, set, true );

        Meta::TrackPtr newTrack;
        newTrack = getRandomTrack( set );
        if( newTrack )
            list->setTrack( newPos, newTrack );
    }
}

void
BiasSolver::annealingOptimize( SolverList *list,
                               int iterationLimit,
                               bool updateStatus )
{
    DEBUG_BLOCK;

    if( list->m_trackList.count() <= list->m_contextCount )
        return; // nothing to optimize. All tracks are in the context

    SolverList originalList = *list;

    /*
     * The process used here is called "simulated annealing". The basic idea is
     * that the playlist is randomly mutated one track at a time. Mutations that
     * improve the playlist (decrease the energy) are always accepted, mutations
     * that make the playlist worse (increase the energy) are sometimes
     * accepted. The decision to accept is made randomly based on a special
     * probability curve that changes as the algorithm progresses.
     *
     * Accepting some bad mutations makes the algorithm resilient to getting
     * stuck in local minima (playlists that are not optimal but can't be
     * improved by making just one change). There is much more reading available
     * on the internet or your local library.
     */

    double T = SA_INITIAL_TEMPERATURE;
    TrackSet universeSet( m_trackCollection, true );

    double oldEnergy = 0.0;
    int giveUpCount = 0;
    while( iterationLimit-- && list->energy() >= epsilon() && !m_abortRequested )
    {
        // if the energy hasn't changed in SA_GIVE_UP_LIMIT iterations, we give
        // up and bail out.
        if( oldEnergy == list->energy() )
            giveUpCount++;
        else
        {
            oldEnergy = list->energy();
            giveUpCount = 0;
        }

        if( giveUpCount >= SA_GIVE_UP_LIMIT )
            break;

        // choose the mutation position
        int newPos = (KRandom::random() % (list->m_trackList.count() - list->m_contextCount))
            + list->m_contextCount;

        // choose a matching track or a random one. Prefer matching
        Meta::TrackPtr newTrack;
        if( iterationLimit % 4 )
        {
            TrackSet set = matchingTracks( newPos, list->m_trackList );
            if( !m_allowDuplicates )
                set = withoutDuplicate( newPos, list->m_trackList, set, false );
            newTrack = getRandomTrack( set );
        }
        else
        {
            if( !m_allowDuplicates )
                newTrack = getRandomTrack( withoutDuplicate( newPos, list->m_trackList, universeSet, false ) );
            else
                newTrack = getRandomTrack( universeSet );
        }

        if( !newTrack )
            continue;

        // debug() << "replacing"<<newPos<<list->m_trackList[newPos]->name()<<"with"<<newTrack->name();

        SolverList newList = *list;
        newList.setTrack( newPos, newTrack );

        double p = 1.0 / ( 1.0 + exp( (newList.energy() - list->energy()) / list->m_trackList.count()  / T ) );
        double r = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);

        // accept the mutation ?
        if( r <= p )
            *list = newList;

        // cool the temperature
        T *= SA_COOLING_RATE;

        if( updateStatus && iterationLimit % 100 == 0 )
        {
            debug() << "SA: E = " << list->energy();
            int progress = (int)(100.0 * (1.0 - list->energy()));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }
    }

    // -- use the original list if we made it worse
    if( list->energy() > originalList.energy() )
        *list = originalList;
}

void
BiasSolver::geneticOptimize( SolverList *list,
                             int iterationLimit,
                             bool updateStatus )
{
    Q_UNUSED( list );
    Q_UNUSED( iterationLimit );
    Q_UNUSED( updateStatus );

    /**
     * Here we attempt to produce an optimal playlist using a genetic algorithm.
     * The basic steps:
     *
     *   1. Generate a population of playlists using generateInitialPlaylist.
     *
     * REPEAT:
     *   2. Choose a portion of that population to reproduce. The better the
     *      playlist (the lower the energy) the more likely it is to reproduce.
     *   3. The mating population playlists are mixed with each other producing
     *      offspring playlists.
     *   4. The worst playlists in the population are thrown out and replaced
     *      with the new offspring.
     */
#if 0

    // 1.  Generate initial population
    QList<SolverList> population;
    SolverList playlist;
    while( population.size() < GA_POPULATION_SIZE )
    {
        // TODO: OPTIMIZATION: most of the time spend solving now is spent
        // getting Meta::Tracks, since we request so many with this. Experiment
        // with lowering the population size, or finding a faster way to get a
        // bunch of random tracks.
        playlist = generateInitialPlaylist();

        playlist.removeAll( Meta::TrackPtr() );

        // test for the empty collection case
        if( playlist.empty() )
        {
            warning() << "Empty collection, aborting.";
            return Meta::TrackList();
        }

        double plEnergy = playlist->energy();

        if( plEnergy < epsilon() ) // no need to continue if we already found an optimal playlist
            return playlist;

        population.append( playlist );
    }

    qSort( population ); // sort the population by energy.


    double prevMin = 0.0;
    int giveUpCount = 0;
    int i = iterationLimit;
    QList<int> matingPopulation;
    while( i-- && population.first().energy >= epsilon() && !m_abortRequested )
    {
        // Sometime the optimal playlist can't have an energy of 0.0, or the
        // algorithm just gets stuck. So if the energy hasn't changed after
        // GIVE_UP_LIMIT iterations, we assume we bail out.
        if( population.first().energy == prevMin )
            giveUpCount++;
        else
        {
            prevMin = population.first().energy;
            giveUpCount = 0;
        }

        if( giveUpCount >= GA_GIVE_UP_LIMIT )
            break;


        // status updates
        if( updateStatus && i % 5 == 0 )
        {
            int progress = (int)(100.0 * (1.0 - population.first().energy));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }

        debug() << "GA: min E = " << population.first().energy;
        debug() << "GA: max E = " << population.last().energy;



        // 2. Choose the portion of the population to reproduce.
        matingPopulation = generateMatingPopulation( population );

        // randomize the order of the mating population so we don't get the same
        // playlists mating over and over
        int m = matingPopulation.size();
        while( m > 1 )
        {
            int k = KRandom::random() % m;
            --m;
            matingPopulation.swap( m, k );
        }

        QList<Meta::TrackList> offspring;



        // (I'm hanging on to code for now, until I do more testing.)
        // reproduce using single point crossover
#if 0
        for( int j = 0; j < matingPopulation.size(); ++j )
        {
            int parent1 = matingPopulation[j];
            int parent2 = j == 0 ? matingPopulation.last() : matingPopulation[j-1];

            Meta::TrackList child1, child2;
            int locus = KRandom::random() % m_n;

            child1 += population[parent1].trackList.mid( 0, locus );
            child1 += population[parent2].trackList.mid( locus );

            child2 += population[parent2].trackList.mid( 0, locus );
            child2 += population[parent1].trackList.mid( locus );

            offspring += child1;
            offspring += child2;
        }
#endif
        
        // 3. Reproduce (using uniform crossover).
        for( int j = 0; j < matingPopulation.size(); ++j )
        {
            int parent1 = matingPopulation[j];
            int parent2 = j == 0 ? matingPopulation.last() : matingPopulation[j-1];

            Meta::TrackList child1, child2;

            for( int k = 0; k < m_n; ++k )
            {
                if( KRandom::random() < RAND_MAX/2 )
                {
                    child1.append( population[parent1].trackList[k] );
                    child2.append( population[parent2].trackList[k] );
                }
                else
                {
                    child1.append( population[parent2].trackList[k] );
                    child2.append( population[parent1].trackList[k] );
                }
            }

            offspring += child1;
            offspring += child2;
        }


        // 4. Replace the worst in the population with the offspring.
        int j = population.size() - 1;
        foreach( const Meta::TrackList &p, offspring )
        {
            // TODO: try introducing mutations to the offspring here.

            population[j--] = SolverList( p, energy(p) );
        }

        qSort( population ); // sort playlists by energy
    }


    // select the best solution
    *list = population.first();
#endif
}


Meta::TrackList
BiasSolver::solution()
{
    return m_solution;
}


SolverList
BiasSolver::generateInitialPlaylist() const
{
    SolverList result( m_context, m_context.count(), m_bias );

    // Empty collection
    if( m_trackCollection->count() == 0 )
    {
        debug() << "Empty collection when trying to generate initial playlist...";
        return result;
    }

    // just create a simple playlist by adding random tracks to the end.

    TrackSet universeSet( m_trackCollection, true );
    while( result.m_trackList.count() < m_context.count() + m_n )
    {
        Meta::TrackPtr newTrack;
        if( !m_allowDuplicates )
            newTrack = getRandomTrack( withoutDuplicate( -1, result.m_trackList, universeSet, false ) );
        else
            newTrack = getRandomTrack( universeSet );
        result.appendTrack( newTrack );
    }

    debug() << "generated random playlist with"<<result.m_trackList.count()<<"tracks";
    return result;
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
    const KUrl url( uid );
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
BiasSolver::matchingTracks( int position, const Meta::TrackList& playlist ) const
{
    QMutexLocker locker( &m_biasResultsMutex );
    m_tracks = m_bias->matchingTracks( position, playlist, m_context.count(), m_trackCollection );
    if( m_tracks.isOutstanding() )
        m_biasResultsReady.wait( &m_biasResultsMutex );

    debug() << "BiasSolver::matchingTracks returns"<<m_tracks.trackCount()<<"of"<<m_trackCollection->count()<<"tracks.";

    return m_tracks;
}

Dynamic::TrackSet
BiasSolver::withoutDuplicate( int position, const Meta::TrackList& playlist,
                              const Dynamic::TrackSet& oldSet,
                              bool onlyBackwards )
{
    Dynamic::TrackSet result = Dynamic::TrackSet( oldSet );
    for( int i = 0; i < (onlyBackwards ? position : playlist.count()); i++ )
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

    connect( qm, SIGNAL(newResultReady( QStringList )),
             this, SLOT(trackCollectionResultsReady( QStringList )),
             Qt::DirectConnection );
    connect( qm, SIGNAL(queryDone()),
             this, SLOT(trackCollectionDone()),
             Qt::DirectConnection );

    qm->run();
}

}

