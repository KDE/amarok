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
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "TrackSet.h"

#include <cmath>
#include <typeinfo>

#include <QHash>
#include <QMutexLocker>

#include <KRandom>


/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be careful */
#include <threadweaver/Thread.h>

const int    Dynamic::BiasSolver::GA_ITERATION_LIMIT         = 70;
const int    Dynamic::BiasSolver::GA_POPULATION_SIZE         = 15;
const int    Dynamic::BiasSolver::GA_MATING_POPULATION_SIZE  = 5;
const double Dynamic::BiasSolver::GA_MUTATION_PROBABILITY    = 0.05;
const int    Dynamic::BiasSolver::GA_GIVE_UP_LIMIT           = 10;

const int    Dynamic::BiasSolver::SA_ITERATION_LIMIT     = 1000;
const double Dynamic::BiasSolver::SA_INITIAL_TEMPERATURE = 0.28;
const double Dynamic::BiasSolver::SA_COOLING_RATE        = 0.82;
const int    Dynamic::BiasSolver::SA_GIVE_UP_LIMIT       = 250;


namespace Dynamic
{
    struct TrackListEnergyPair
    {
        TrackListEnergyPair( Meta::TrackList trackList, double energy )
            : trackList(trackList), energy(energy) {}

        bool operator<( const TrackListEnergyPair& x ) const { return energy < x.energy; }

        Meta::TrackList trackList;
        double energy;
    };
}



Dynamic::BiasSolver::BiasSolver( int n, Dynamic::AbstractBias *bias, Meta::TrackList context )
    : m_n(n)
    , m_bias(bias)
    , m_context(context)
    , m_abortRequested(false)
{
    // debug() << "CREATING BiasSolver in thread:" << QThread::currentThreadId();
}


Dynamic::BiasSolver::~BiasSolver()
{
    // debug() << "DESTROYING BiasSolver in thread:" << QThread::currentThreadId();
}


void
Dynamic::BiasSolver::requestAbort()
{
    m_abortRequested = true;
}

bool
Dynamic::BiasSolver::success() const
{
    return !m_abortRequested;
}

void
Dynamic::BiasSolver::setAutoDelete( bool autoDelete )
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


void Dynamic::BiasSolver::run()
{
    DEBUG_BLOCK

    debug() << "BiasSolver::run in thread:" << QThread::currentThreadId();

    getTrackCollection();

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

    TrackListEnergyPair playlist = generateInitialPlaylist();
    while( playlist.energy > 0.05 * m_n ) // the playlist is only slightly wrong
    {
        // sa_optimize( playlist, SA_ITERATION_LIMIT, true );
    }

    m_solution = playlist.playlist.mid( m_context.count() );
    m_solution = playlist;
    debug() << "Found solution with energy"<<playlist.energy;
}


void
Dynamic::BiasSolver::sa_optimize( Meta::TrackList& playlist, int i, bool updateStatus )
{
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

    double E = energy( playlist );
    double T = SA_INITIAL_TEMPERATURE;

    Meta::TrackPtr mutation;
    double prevE = 0.0;
    int giveUpCount = 0;
    while( i-- && E >= m_epsilon && !m_abortRequested )
    {
        // if the energy hasn't changed in SA_GIVE_UP_LIMIT iterations, we give
        // up and bail out.
        if( prevE == E )
            giveUpCount++;
        else
        {
            prevE = E;
            giveUpCount = 0;
        }

        if( giveUpCount >= SA_GIVE_UP_LIMIT )
            break;


        // get a random mutation track.
        mutation = getMutation();

        if( !mutation )
            break;

        // choose the mutation position
        int mutationPos = KRandom::random() % playlist.size();

        double mutationE = recalculateEnergy( playlist, mutation, mutationPos );


        double p = 1.0 / ( 1.0 + exp( (mutationE - E)  / T ) );
        double r = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);

        // accept the mutation ?
        if( r <= p )
        {
            playlist[ mutationPos ] = mutation;
            E = mutationE;
            // m_biasEnergy = m_biasMutationEnergy;
        }

        // cool the temperature
        T *= SA_COOLING_RATE;

        
        if( updateStatus && i % 100 == 0 )
        {
            debug() << "SA: E = " << E;
            int progress = (int)(100.0 * (1.0 - E));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }
    }
}

Meta::TrackList
Dynamic::BiasSolver::ga_optimize( int iterationLimit, bool updateStatus )
{
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

    // 1.  Generate initial population
    QList<TrackListEnergyPair> population;
    Meta::TrackList playlist;
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

        double plEnergy = energy( playlist );

        if( plEnergy < m_epsilon ) // no need to continue if we already found an optimal playlist
            return playlist;

        population.append( TrackListEnergyPair( playlist, plEnergy ) );
    }

    qSort( population ); // sort the population by energy.


    double prevMin = 0.0;
    int giveUpCount = 0;
    int i = iterationLimit;
    QList<int> matingPopulation;
    while( i-- && population.first().energy >= m_epsilon && !m_abortRequested )
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

            population[j--] = TrackListEnergyPair( p, energy(p) );
        }

        qSort( population ); // sort playlists by energy
    }


    // select the best solution
    playlist = population.first().trackList;
    energy( playlist ); // (we have to recalculate, so m_biasEnergy gets set correctly.) 

    return playlist;
}


Meta::TrackList Dynamic::BiasSolver::solution()
{
    return m_solution;
}


TrackListEnergyPair
Dynamic::BiasSolver::generateInitialPlaylist() const
{
    TrackListEnergyPair result;
    result.playlist.append( m_context );
    result.energy = 0;

    // Empty collection
    if( m_trackCollection.count() == 0 )
    {
        debug() << "Empty collection when trying to generate initial playlist...";
        result.energy = m_n;
        return result;
    }

    // just create a simple playlist by adding tracks to the end.

    TrackSet universeSet( result );
    while( result.playlist.count() < m_context.count() + m_n )
    {
        TrackSet set = matchingTracks( result.playlist.count(),
                                       result.playlist );
        Meta::TrackPtr newTrack = getRandomTrack( set );
        if( newTrack )
            result.playlist.append( newTrack );
        else
        {
            result.playlist.append( getRandomTrack(universeSet) );
            result.energy++;
        }
    }

    return result;
}

Meta::TrackPtr
Dynamic::BiasSolver::getRandomTrack( const TrackSet& subset ) const
{
    if( subset.trackCount() == 0 )
        return Meta::TrackPtr();

    Meta::TrackPtr track;

    // this is really dumb, but we sometimes end up with uids that don't point to anything
    int giveup = 50;
    while( giveup-- && !track )
        track = trackForUid( subset.getRandomTrack(s_universe) );

    if( track )
    {
        // if( track->artist() )
            // debug() << "track selected:" << track->name() << track->artist()->name();
    }
    else
        error() << "track is 0 in BiasSolver::getRandomTrack()";

    return track;
}

Meta::TrackPtr
Dynamic::BiasSolver::getMutation()
{
    if( m_mutationPool.isEmpty() )
    {
        m_mutationPool = generateInitialPlaylist();
    }

    if( m_mutationPool.isEmpty() )
        return Meta::TrackPtr();
    else
        return m_mutationPool.takeLast();
}

Meta::TrackPtr
Dynamic::BiasSolver::trackForUid( const QByteArray& uid ) const
{
    const KUrl url( uid );
    Meta::TracckPtr track = CollectionManager::instance()->trackForUrl( url );

    if( !track )
        warning() << "trackForUid returned no track for "<<uid;
    return track;
}


// ---- getting the matchingTracks ----

void
Dynamic::BiasSolver::biasResultReady( const Dynamic::TrackSet &set )
{
    m_tracks = set;
    m_biasResultsReady.wakeAll();
}

TrackSet
Dynamic::BiasSOlver::matchingTracks( int position, const Meta::TrackList& playlist ) const
{
    QMutexLocker locker( &m_biasResultsMutex );
    m_tracks = m_bias->matchingTracks( position, playlist, m_trackCollection );
    if( m_tracks.isOutstanding() )
        m_collectionResultsReady.wait( m_collectionResultsMutex );

    return m_tracks;
}


// ---- getting the TrackCollection ----

void
Dynamic::BiasSolver::trackCollectionResultsReady( QString collectionId, QStringList uids )
{
    Q_UNUSED( collectionId );
    m_collectionUids.append( uids );
}

void
Dynamic::BiasSolver::trackCollectionDone()
{
    m_collectionResultsReady.wakeAll();
}

void
Dynamic::BiasSolver::getTrackCollection()
{
    // get all the unique ids from the collection manager
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnValue( Meta::valUniqueId );

    connect( qm, SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(trackCollectionResultsReady( QString, QStringList )),
             Qt::DirectConnection );
    connect( qm, SIGNAL(queryDone()),
             this, SLOT(trackCollectionDone()),
             Qt::DirectConnection );

    QMutexLocker locker( &m_collectionResultsMutex );
    qm->run();
    // wait until all results are there
    m_collectionResultsReady.wait( m_collectionResultsMutex );
    delete qm;

    m_trackCollection = TrackCollectionPtr( new TrackCollection( m_collectionUids ) );
    m_collectionUids.clear();
}


