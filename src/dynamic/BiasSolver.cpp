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

#define DEBUG_PREFIX "BiasSolver"

#include "BiasSolver.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "MetaConstants.h"
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



QList<QByteArray> Dynamic::BiasSolver::s_universe;
QMutex            Dynamic::BiasSolver::s_universeMutex;
QueryMaker*       Dynamic::BiasSolver::s_universeQuery = 0;
Amarok::Collection*       Dynamic::BiasSolver::s_universeCollection = 0;
bool              Dynamic::BiasSolver::s_universeOutdated = true;
unsigned int      Dynamic::BiasSolver::s_uidUrlProtocolPrefixLength = 0;



/*
 * A playlist/energy pair, used by ga_optimize to sort lists of playlists by
 * their energy.
 */
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



Dynamic::BiasSolver::BiasSolver( int n, QList<Bias*> biases, Meta::TrackList context )
    : m_biases(biases)
    , m_n(n)
    , m_context(context)
    , m_epsilon( 1.0 / (double)n )
    , m_pendingBiasUpdates(0)
    , m_abortRequested(false)
{
    debug() << "CREATING BiasSolver in thread:" << QThread::currentThreadId();
    int i = m_biases.size();
    while( i-- )
    {
        m_biasEnergy.append( 0.0 );
        m_biasMutationEnergy.append( 0.0 );
    }
    
}


Dynamic::BiasSolver::~BiasSolver()
{
    debug() << "DESTROYING BiasSolver in thread:" << QThread::currentThreadId();

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

void Dynamic::BiasSolver::prepareToRun()
{
    DEBUG_BLOCK

    // update biases
    CollectionDependantBias* cb;
    foreach( Bias* b, m_biases )
    {
        if( (cb = dynamic_cast<CollectionDependantBias*>( b ) ) )
        {
            if( cb->needsUpdating() )
            {
                connect( cb, SIGNAL(biasUpdated(CollectionDependantBias*)),
                        SLOT(biasUpdated()), Qt::DirectConnection );
                cb->update();
                m_pendingBiasUpdates++;
            }
        }
    }

    // nothing to update
    if( !m_pendingBiasUpdates && !s_universeOutdated )
    {
        emit readyToRun();
        return;
    }


    // update universe

    if( s_universeOutdated )
        updateUniverse();
}

void Dynamic::BiasSolver::run()
{
    DEBUG_BLOCK

    debug() << "BiasSolver::run in thread:" << QThread::currentThreadId();
    computeDomain();

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

    bool optimal;
    Meta::TrackList playlist = generateInitialPlaylist( optimal );

    if( playlist.isEmpty() )
        return;

    if( !optimal )
        sa_optimize( playlist, SA_ITERATION_LIMIT, true );

    m_solution = playlist;
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
            m_biasEnergy = m_biasMutationEnergy;
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
    bool optimal;
    QList<TrackListEnergyPair> population;
    Meta::TrackList playlist;
    while( population.size() < GA_POPULATION_SIZE )
    {
        // TODO: OPTIMIZATION: most of the time spend solving now is spent
        // getting Meta::Tracks, since we request so many with this. Experiment
        // with lowering the population size, or finding a faster way to get a
        // bunch of random tracks.
        playlist = generateInitialPlaylist( optimal );

        playlist.removeAll( Meta::TrackPtr() );

        // test for the empty collection case
        if( playlist.empty() )
        {
            warning() << "Empty collection, aborting.";
            return Meta::TrackList();
        }

        if( optimal )
            return playlist;

        population.append( TrackListEnergyPair( playlist, energy( playlist ) ) );
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



QList<int>
Dynamic::BiasSolver::generateMatingPopulation( const QList<TrackListEnergyPair>& population )
{
    /**
     * Used by the reproduction phase of each iteration of the ge_optimize, this
     * algorithm chooses the subset of the population that will be used to
     * produce offspring. The technique used here is called "stochastic
     * universal sampling".
     */

    double sum = 0.0;
    foreach( const TrackListEnergyPair &p, population )
        sum += 1.0 - p.energy;

    double p = 
        (1.0/(double)GA_MATING_POPULATION_SIZE) *
        ((double)KRandom::random() / (((double)RAND_MAX) + 1.0));

    QList<int> matingPopulation;
    for( int i = 0; i < population.size() && matingPopulation.size() < GA_MATING_POPULATION_SIZE; ++i )
    {
        if( p <= (1.0 - population[i].energy)/sum )
        {
            matingPopulation.append( i );
            p += 1.0/(double)GA_MATING_POPULATION_SIZE;
        }

        p -= (1.0 - population[i].energy)/sum;
    }

    return matingPopulation;
}


Meta::TrackList Dynamic::BiasSolver::solution()
{
    return m_solution;
}


void
Dynamic::BiasSolver::biasUpdated()
{
    DEBUG_BLOCK

    if( m_pendingBiasUpdates <= 0 )
        return;

    if( --m_pendingBiasUpdates == 0 && !s_universeOutdated )
        emit readyToRun();
}



double Dynamic::BiasSolver::energy( const Meta::TrackList& playlist )
{
    int activeBiases = 0;
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
        if( m_biases[i]->active() )
        {
            m_biasEnergy[i] = m_biases[i]->energy( playlist, m_context );
            sum += qAbs( m_biasEnergy[i]  );
            activeBiases++;
        }
    }

    return sum / (double)activeBiases;
}


double Dynamic::BiasSolver::recalculateEnergy( const Meta::TrackList& playlist, Meta::TrackPtr mutation, int mutationPos )
{
    int activeBiases = 0;
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
        if( m_biases[i]->active() )
        {
            m_biasMutationEnergy[i] = 
                m_biases[i]->reevaluate( 
                        m_biasEnergy[i], playlist, mutation, 
                        mutationPos, m_context );
            sum += qAbs( m_biasMutationEnergy[i] );
            activeBiases++;
        }
    }

    return sum / (double)activeBiases;
}


Meta::TrackList
Dynamic::BiasSolver::generateInitialPlaylist( bool& optimal )
{
    DEBUG_BLOCK

    /*
     * Playlist generation is NP-Hard, but the subset of playlist generation
     * that consists of just global, proportional biases can be solved in linear
     * time (to be precise, O(m*n), where m is the number of biases, and n is
     * the size of the playlist to generate.  Here we do that. Since we rely on
     * random mutations, and the subset of the proportional bias may be very
     * small, solving it otherwise could be potentially very slow. 
     *
     * Note that this is just a heuristic for the system as whole. We cross our
     * fingers a bit and assume it is mostly composed of global biases.
     *
     * The Algorithm:
     * (I invented this, but it seems like something that would already exist. If
     * anyone knows what this is called, please let me know. --DCJ)
     *
     * We start with a collection of proportional biases, those with
     * with a [0,1] proportion p_i and a subset of the universe, S_i.
     *
     * We build up a playlist one track at a time.  For each bias we must make a
     * decision: is the track in the set S_i, or not in the set S_i. So the
     * algorithm can be thought of as working its way down a tree:
     *
     *                              In S_0?
     *                               /   \
     *                            (yes) (no)
     *                             /      \
     *                         In S_1?   In S_1? 
     *                          /  \     /  \
     *                        ... ...  ...  ...
     *
     * In this way we find a subset from which to choose a random track so it
     * will satisfy the biases. We start with S = U (the universe set), then at
     * each step, if we decide the track is in S_i, we set "S := S intersect
     * S_i", on the other hand, if we decide the track is not in S_i, we take "S
     * := S subtract S_i". Once we get to the bottom of the tree, S is a subset
     * from which we choose a random track.
     *
     * Ok, but how do we decide if the track is in or not in a set S_i? We use
     * the proportions (p_i) and decide at random. If p_0 = 0.4, there is a 0.4
     * chance the we decide the track is in S_0, and a 0.6 chance it is not.
     *
     * But it's not quite that simple. As the playlist gets built up, we change
     * the p_i value to reflect the proportion of _remaining_ tracks should be
     * in S_i. So if, p_0 = 0.4, and we add a track that is in S_0, then that
     * number will go down, since we need fewer tracks to satisfy that bias.
     * This way we always get the best possible playlist.
     *
     * There are a couple of other caveats (such as producing reasonable
     * playlists when given infeasible systems of biases, e.g. 100% Radiohead,
     * AND 100% Bob Dylan), that you can read on to learn about.
     *
     */                

    // this algorithm will produce an optimal solution unless there are
    // non global biases in the system.
    optimal = (m_biases.size() == m_feasibleCollectionFilters.size());
    Meta::TrackList playlist;

    // Empty collection
    if( s_universe.isEmpty() )
    {
        optimal = false;
        debug() << "Empty collection when trying to generate initial playlist...";
        return Meta::TrackList();
    }

    // No feasible global biases
    if( m_feasibleCollectionFilters.size() == 0 )
    {
        int n = m_n;
        while( n-- )
            playlist += getRandomTrack( m_domain );

        optimal = m_biases.isEmpty();
        debug() << "No collection filters, returning random initial playlist";
        return playlist;
    }

    // We are going to be computing a lot of track set intersections so we will
    // memoize to try and save time (if not memory).
    QHash< QBitArray, QList<QByteArray> > memoizedIntersections;

    // As we build up the playlist the weights for each bias will change to
    // reflect what proportion of the tracks that remain to be chosen should
    // have the property in question.
    double* movingWeights = new double[m_feasibleCollectionFilters.size()];
    for( int i = 0; i < m_feasibleCollectionFilters.size(); ++i )
        movingWeights[i] = m_feasibleCollectionFilters[i]->weight();

    debug() << "Just set movingWeights to:" << movingWeights;
    
    // We use this array of indexes to randomize the order the biases are looked
    // at. That way we get reasonable results when the system is infeasible.
    // That is, specifying 100% of two mutually exclusive artists, will get you
    // about 50% of each.
    QList<int> indexes;
    for( int i = 0; i < m_feasibleCollectionFilters.size(); ++i )
        indexes.append( i );


    Dynamic::TrackSet S, R;

    double decider;
    int n = m_n;
    while( n-- && !m_abortRequested )
    {
        // For each bias, we must make a decision whether the track being chosen
        // should belong to it or not. This is simply a probabilistic choice
        // based on the current weight. 


        // Randomize the order.
        int m = m_feasibleCollectionFilters.size();
        while( m > 1 )
        {
            int k = KRandom::random() % m;
            --m;
            indexes.swap( m, k );
        }


        // The bit array represents the choice made at each branch.
        QBitArray branches( m_feasibleCollectionFilters.size(), 0x0 );

        S.setUniverseSet();

        for( int _i = 0; _i < m_feasibleCollectionFilters.size(); ++_i )
        {
            int i = indexes[_i];

            R = S;

            // Decide whether we should 'accept' or 'reject' a bias.
            decider = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);
            debug() << "decider is set to:" << decider << "movingWeights is:" << movingWeights[ i ];
            if( decider < movingWeights[i] )
            {
                debug() << "chose track from bias";
                branches.setBit( i, true );
                R.intersect( m_feasibleCollectionFilterSets[i] );
            }
            else
            {
                debug() << "bias NOT chosen.";
                branches.setBit( i, false );
                R.subtract( m_feasibleCollectionFilterSets[i] );
            }

            // Now we have to make sure our decision doesn't land us with an
            // empty set. If that's the case, we have to choose the other
            // branch, even if it does defy the probability. (This is how we
            // deal with infeasible systems.)
            //debug() << "after set intersection/subtraction, R has size:" << R.size();

            if( R.size() == 0 )
                branches.toggleBit( i );
            else
                S = R;

            if( branches[i] )
                movingWeights[i] = (movingWeights[i]*(double)(n+1)-1.0)/(double)n;
            else
                movingWeights[i] = (movingWeights[i]*(double)(n+1))/(double)n;
        }

        // Memoize to avoid touching U as much as possible, and to avoid
        // duplicate toList conversions.
        if( !memoizedIntersections.contains( branches ) )
        {
            memoizedIntersections[branches] = S.uidList();
        }

        const QList<QByteArray>& finalSubset = memoizedIntersections[branches];

        // this should never happen
        if( finalSubset.size() == 0 )
        {
            error() << "BiasSolver assumption failed.";
            continue;
        }

        // choose a track at random from our final subset
        playlist.append( getRandomTrack( finalSubset ) );

        if( optimal )
            emit statusUpdate( (int)(100.0 * (double)(m_n - n) / (double)n) );
    }

    delete[] movingWeights;

    return playlist;
}

Meta::TrackPtr
Dynamic::BiasSolver::getRandomTrack( const QList<QByteArray>& subset )
{
    if( subset.size() == 0 ) 
        return Meta::TrackPtr();

    Meta::TrackPtr track;

    // this is really dumb, but we sometimes end up with uids that don't point to anything
    int giveup = 50;
    while( giveup-- && !track )
        track = trackForUid( subset[ KRandom::random() % subset.size() ] );

    if( track )
    {
        if( track->artist() )
            debug() << "track selected:" << track->name() << track->artist()->name();
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
        bool optimal; // (we don't actually care if its optimal here)
        m_mutationPool = generateInitialPlaylist( optimal );
    }

    if( m_mutationPool.isEmpty() )
        return Meta::TrackPtr();
    else
        return m_mutationPool.takeLast();
}

Meta::TrackPtr
Dynamic::BiasSolver::trackForUid( const QByteArray& uid )
{
    const KUrl url = s_universeCollection->uidUrlProtocol() + "://" + QString( uid );
    return s_universeCollection->trackForUrl( url );
}


void
Dynamic::BiasSolver::computeDomain()
{
    DEBUG_BLOCK
    foreach( Dynamic::Bias* b, m_biases )
    {
        if( b->hasCollectionFilterCapability() )
        {
            debug() << "Got a bias which says it wants to filter from collection.";
            Dynamic::CollectionFilterCapability* fc = b->collectionFilterCapability();
            if( fc )
            {
                debug() << "and got a proper collectionfiltercapability from it";
                debug() << "property size: " << fc->propertySet().size();

                // if the bias is infeasible (i.e. size = 0), just ignore it
                if( fc->propertySet().size() == 0 )
                {
                    debug() << "infeasible bias detected"; // ugly but we're using the higher cast as they don't share a root parent
                    b->setActive(false);
                    delete fc;
                }
                else
                {
                    m_feasibleCollectionFilters.append( fc );
                    m_feasibleCollectionFilterSets.append( TrackSet( fc->propertySet() ) );
                }
            }
        }
    }
    
    TrackSet subset;
    subset.setUniverseSet();

    for( int i = 0; i < m_feasibleCollectionFilters.size(); ++i )
    {
        if( m_feasibleCollectionFilters.at(i)->weight() == 1.0 )
            subset.intersect( m_feasibleCollectionFilterSets.at(i) );

        if( m_feasibleCollectionFilters.at(i)->weight() == 0.0 )
            subset.subtract( m_feasibleCollectionFilterSets.at(i) );
    }

    m_domain = subset.uidList();

    // if we are left with an empty set, better we just use the universe than
    // give the user what they are really asking for.
    if( m_domain.size() == 0 )
        m_domain = s_universe;

    debug() << "domain size: " << m_domain.size();
}

void
Dynamic::BiasSolver::updateUniverse()
{
    DEBUG_BLOCK

    disconnect( CollectionManager::instance(), SIGNAL(collectionAdded(Amarok::Collection*,CollectionManager::CollectionStatus)), this, SLOT(updateUniverse()) );

    /* TODO: Using multiple collections.
     * One problem with just using MetaQueryMaker is that we can't store uids as
     * QByteArrays unless we keep separate lists for each collection. If we do
     * keep separate lists, we have to do some extra kung-fu when generating
     * random tracks to decide which list to choose from.
     *
     * We could just deal with the extra memory usage and store them as uid-url
     * strings, but when I first wrote this, I don't this there was a general
     * function to get a track from a uid-url.
     */

    QMutexLocker locker( &s_universeMutex );

    if( !s_universeOutdated )
        return;

    if( !s_universeQuery )
    {
        if( !s_universeCollection )
            s_universeCollection = CollectionManager::instance()->primaryCollection();
        if( !s_universeCollection ) // WTF we really can't get a primarycollection?
        {                           //  whenever a collection is added lets check again, so we catch the loading of the primary colletion
            connect( CollectionManager::instance(), SIGNAL(collectionAdded(Amarok::Collection*,CollectionManager::CollectionStatus)), this, SLOT(updateUniverse()) );
            return;
        }
        
        s_universeQuery = s_universeCollection->queryMaker();
        s_universeQuery->setQueryType( QueryMaker::Custom );
        s_universeQuery->addReturnValue( Meta::valUniqueId );
    }

    s_uidUrlProtocolPrefixLength = (QString(s_universeCollection->uidUrlProtocol()) + "://").length();

    connect( s_universeQuery, SIGNAL(newResultReady( QString, QStringList )),
            SLOT(universeResults( QString, QStringList )), Qt::DirectConnection );
    connect( s_universeQuery, SIGNAL(queryDone()),
            SLOT(universeUpdated()), Qt::DirectConnection );

    s_universe.clear();
    s_universeQuery->run();
}


void
Dynamic::BiasSolver::universeResults( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK
    Q_UNUSED(collectionId)

    QMutexLocker locker( &s_universeMutex );

    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        if ( uidString.isEmpty() )
            continue;

        // for some reason we sometimes get uids without the protocol part
        if( uidString.at( s_uidUrlProtocolPrefixLength - 1 ) != '/' )
            uid = uidString.toAscii();
        else
            uid = uidString.mid( s_uidUrlProtocolPrefixLength ).toAscii();

        if( !uid.isEmpty() )
            s_universe += uid;
    }
}

void
Dynamic::BiasSolver::universeUpdated()
{
    DEBUG_BLOCK
    QMutexLocker locker( &s_universeMutex );

    s_universeOutdated = false;

    if( m_pendingBiasUpdates == 0 )
        emit(readyToRun());
}

void
Dynamic::BiasSolver::outdateUniverse()
{
    QMutexLocker locker( &s_universeMutex );
    s_universeOutdated = true;
}

void
Dynamic::BiasSolver::setUniverseCollection( Amarok::Collection* coll )
{
    QMutexLocker locker( &s_universeMutex );

    if( coll != s_universeCollection )
    {
        s_universeCollection = coll;
        s_universeOutdated = true;
        s_universeQuery = 0; // this will get set on update
    }
}

const QList<QByteArray>&
Dynamic::BiasSolver::universe()
{
    QMutexLocker locker( &s_universeMutex );
    return s_universe;
}

