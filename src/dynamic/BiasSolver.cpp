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

#define DEBUG_PREFIX "BiasSolver"

#include "BiasSolver.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "TrackSet.h"

#include <cmath>
#include <typeinfo>

#include <QHash>
#include <QMutexLocker>

#include <KRandom>
#include <threadweaver/ThreadWeaver.h>


/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be carefull */

const int    Dynamic::BiasSolver::GA_ITERATION_LIMIT         = 100;
const int    Dynamic::BiasSolver::GA_POPULATION_SIZE         = 30;
const int    Dynamic::BiasSolver::GA_MATING_POPULATION_SIZE  = 10;
const double Dynamic::BiasSolver::GA_MUTATION_PROBABILITY    = 0.05;
const int    Dynamic::BiasSolver::GA_GIVE_UP_LIMIT           = 10;

const int    Dynamic::BiasSolver::SA_ITERATION_LIMIT     = 2000;
const double Dynamic::BiasSolver::SA_INITIAL_TEMPERATURE = 0.01;
const double Dynamic::BiasSolver::SA_COOLING_RATE        = 0.8;
const int    Dynamic::BiasSolver::SA_GIVE_UP_LIMIT       = 250;



QList<QByteArray> Dynamic::BiasSolver::s_universe;
QMutex            Dynamic::BiasSolver::s_universeMutex;
QueryMaker*       Dynamic::BiasSolver::s_universeQuery = 0;
Collection*       Dynamic::BiasSolver::s_universeCollection = 0;
bool              Dynamic::BiasSolver::s_universeOutdated = true;
unsigned int      Dynamic::BiasSolver::s_uidUrlProtocolPrefixLength = 0;



/*
 * A playlist/energy pair, just so we can sort playlists by energy.
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
    int i = m_biases.size();
    while( i-- )
    {
        m_biasEnergy.append( 0.0 );
        m_biasMutationEnergy.append( 0.0 );
    }
    
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

void Dynamic::BiasSolver::doWork()
{
    DEBUG_BLOCK
    // update biases

    m_biasMutex.lock();

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

    // update universe

    if( s_universeOutdated )
        updateUniverse();

    if( !m_pendingBiasUpdates && !s_universeOutdated )
    {
        ThreadWeaver::Weaver::instance()->enqueue( this );
    }
    m_biasMutex.unlock();
}

void Dynamic::BiasSolver::run()
{
    DEBUG_BLOCK
    
    computeDomain();

    bool optimal = generateInitialPlaylist();
    if( optimal )
        return;

    // test for the empty collection case
    m_playlist.removeAll( Meta::TrackPtr() );
    if( m_playlist.empty() )
    {
        warning() << "Empty collection, aborting.";
        return;
    }

    if( optimal )
        return;

    // generate initial population
    QList<TrackListEnergyPair> population;
    population.append( TrackListEnergyPair( m_playlist, energy() ) );
    while( population.size() < GA_POPULATION_SIZE )
    {
        generateInitialPlaylist();
        population.append( TrackListEnergyPair( m_playlist, energy() ) );
    }

    qSort( population ); // sort playlists by energy

    /*
     * GENETIC OPTIMIZATION PHASE
     * Genetic algorithms tend converge faster initially. So we begin with a
     * genetic phase before switching over to annealing.
     */

    double prevMin = 0.0;
    int giveUpCount = 0;
    int i = GA_ITERATION_LIMIT;
    QList<int> matingPopulation;
    while( i-- && population.first().energy >= m_epsilon && !m_abortRequested )
    {
        // Sometime the optimal playlist can't have an energy of 0.0. So if the
        // energy hasn't changed after GIVE_UP_LIMIT iterations, we assume we've
        // found the optimal playlist and bail out.
        if( population.first().energy == prevMin )
            giveUpCount++;
        else
        {
            prevMin = population.first().energy;
            giveUpCount = 0;
        }


        if( giveUpCount >= GA_GIVE_UP_LIMIT )
            break;

        if( i % 5 == 0 )
        {
            int progress = (int)(100.0 * (1.0 - population.first().energy));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }

        debug() << "GA: min E = " << population.first().energy;
        debug() << "GA: max E = " << population.last().energy;

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
        
        // rerpoduce using uniform crossover
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


        // Replace the worst in the population with the offspring.
        int j = population.size() - 1;
        foreach( Meta::TrackList p, offspring )
        {
            m_playlist = p;
            m_E = energy();
            
            // TODO: try introducing mutations

            population[j--] = TrackListEnergyPair( p, m_E );
        }

        qSort( population ); // sort playlists by energy
    }


    // select the best solution
    m_playlist = population.first().trackList;
    m_E = energy(); // (we have to recalculate, so m_biasEnergy gets set correctly.) 


    // SIMULATE ANNEALING PHASE
    // Now let annealing have a go at it.
    m_T = SA_INITIAL_TEMPERATURE;
    anneal( SA_ITERATION_LIMIT, true );

    debug() << "System solved in " << (GA_ITERATION_LIMIT - i)
            << "GA iterations, with E = " << m_E;
}


void
Dynamic::BiasSolver::anneal( int i, bool updateStatus )
{
    Meta::TrackPtr mutation;
    double prevE = 0.0;
    int giveUpCount = 0;
    while( i-- && m_E >= m_epsilon && !m_abortRequested )
    {
        if( prevE == m_E )
            giveUpCount++;
        else
        {
            prevE = m_E;
            giveUpCount = 0;
        }

        if( giveUpCount >= SA_GIVE_UP_LIMIT )
            break;

        mutation = getMutation();

        // empty collection, abort
        if( !mutation )
            break;
        
        iterateAnnealing( mutation );

        if( updateStatus && i % 100 == 0 )
        {
            debug() << "SA: E = " << m_E;
            int progress = (int)(100.0 * (1.0 - m_E));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }
    }
}


QList<int>
Dynamic::BiasSolver::generateMatingPopulation( const QList<TrackListEnergyPair>& population )
{
    // Selection using stochastic universal sampling.

    double sum = 0.0;
    foreach( TrackListEnergyPair p, population )
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
    return m_playlist;
}


void
Dynamic::BiasSolver::biasUpdated()
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_biasMutex );

    if( m_pendingBiasUpdates == 0 )
        return;

    if( --m_pendingBiasUpdates == 0 && !s_universeOutdated )
        ThreadWeaver::Weaver::instance()->enqueue( this );
}


void Dynamic::BiasSolver::iterateAnnealing( Meta::TrackPtr mutation )
{
    int mutationPos = KRandom::random() % m_playlist.size();

    double mutationE = recalculateEnergy( mutation, mutationPos );

    double p = 1.0 / ( 1.0 + exp( (mutationE - m_E)  / m_T ) );

    // generate a random number in [0,1]
    double r = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);

    // accept the mutation ?
    if( r <= p )
    {
        m_playlist[ mutationPos ] = mutation;
        m_E = mutationE;
        m_biasEnergy = m_biasMutationEnergy;
    }


    // cool the temperature
    m_T *= SA_COOLING_RATE;
}


double Dynamic::BiasSolver::energy()
{
    int activeBiases = 0;
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
        if( m_biases[i]->active() )
        {
            m_biasEnergy[i] = m_biases[i]->energy( m_playlist, m_context );
            sum += qAbs( m_biasEnergy[i]  );
            activeBiases++;
        }
    }

    return sum / (double)activeBiases;
}


double Dynamic::BiasSolver::recalculateEnergy( Meta::TrackPtr mutation, int mutationPos )
{
    int activeBiases = 0;
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
        if( m_biases[i]->active() )
        {
            m_biasMutationEnergy[i] = 
                m_biases[i]->reevaluate( 
                        m_biasEnergy[i], m_playlist, mutation, 
                        mutationPos, m_context );
            sum += qAbs( m_biasMutationEnergy[i] );
            activeBiases++;
        }
    }

    return sum / (double)activeBiases;
}


bool
Dynamic::BiasSolver::generateInitialPlaylist()
{
    DEBUG_BLOCK
    // Playlist generation is NP-Hard, but the subset of playlist generation
    // that consists of just global, proportional biases can be solved in linear
    // time (to be precise, O(m*n), where m is the number of biases, and n is
    // the size of the playlist to generate.  Here we do that. Solving it
    // otherwise could be potentially very slow.

    // Note that this is just a heuristic for the system as whole. We cross our
    // fingers a bit and assume it is mostly composed of global biases.


    // this algorithm will produce an optimal solution unless there are
    // non global biases in the system.
    bool optimal = (m_biases.size() == m_feasibleGlobalBiases.size());
    m_playlist.clear();

    // Empty collection
    if( s_universe.isEmpty() )
        return false;

    // No feasible global biases
    if( m_feasibleGlobalBiases.size() == 0 )
    {
        int n = m_n;
        while( n-- )
            m_playlist += getMutation();
        return m_biases.isEmpty();
    }

    // We are going to be computing a lot of track set intersections so we will
    // memoize to try and save time (if not memory).
    QHash< QBitArray, QList<QByteArray> > memoizedIntersections;

    // As we build up the playlist the weights for each bias will change to
    // reflect what proportion of the tracks that remain to be chosen should
    // have the property in question.
    double* movingWeights = new double[m_feasibleGlobalBiases.size()];
    for( int i = 0; i < m_feasibleGlobalBiases.size(); ++i )
        movingWeights[i] = m_feasibleGlobalBiases[i]->weight();


    // We use this array of indexes to randomize the order the biases are looked
    // at. That was we get reasonable results when the system is infeasible.
    QList<int> indexes;
    for( int i = 0; i < m_feasibleGlobalBiases.size(); ++i )
        indexes.append( i );


    Dynamic::TrackSet S, R;

    double decider;
    int n = m_n;
    while( n-- && !m_abortRequested )
    {
        // For each bias, we must make a decision whether the track being chosen
        // should belong to it or not. This is simply a probabalistic choice
        // based on the current weight. 


        // Randomize the order.
        int m = m_feasibleGlobalBiases.size();
        while( m > 1 )
        {
            int k = KRandom::random() % m;
            --m;
            indexes.swap( m, k );
        }


        // The bit array represents the choice made at each branch.
        QBitArray branches( m_feasibleGlobalBiases.size(), 0x0 );

        S.setUniverseSet();

        for( int _i = 0; _i < m_feasibleGlobalBiases.size(); ++_i )
        {
            int i = indexes[_i];

            R = S;

            // Decide whether we should 'accept' or 'reject' a bias.
            decider = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);
            if( decider < movingWeights[i] )
            {
                branches.setBit( i, true );
                R.intersect( m_feasibleGlobalBiasSets[i] );
            }
            else
            {
                branches.setBit( i, false );
                R.subtract( m_feasibleGlobalBiasSets[i] );
            }

            // Now we have to make sure our decision doesn't land us with an
            // empty set. If that's the case, we have to choose the other
            // branch, even if it does defy the probability. (This is how we
            // deal with infeasible systems.)
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
        m_playlist.append( getRandomTrack( finalSubset ) );

        if( optimal )
            emit statusUpdate( (int)(100.0 * (double)(m_n - n) / (double)n) );
    }

    return optimal;
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

    return track;
}

Meta::TrackPtr
Dynamic::BiasSolver::getMutation()
{
    return getRandomTrack( m_domain );
}

Meta::TrackPtr
Dynamic::BiasSolver::trackForUid( const QByteArray& uid )
{
    return s_universeCollection->trackForUrl( 
            s_universeCollection->uidUrlProtocol() + "://" + QString(uid.toHex()) );
}


void
Dynamic::BiasSolver::computeDomain()
{
    foreach( Dynamic::Bias* b, m_biases )
    {
        Dynamic::GlobalBias* gb = dynamic_cast<Dynamic::GlobalBias*>( b );

        if( gb )
        {
            debug() << "property size: " << gb->propertySet().size();

            // if the bias is infeasable (i.e. size = 0), just ignore it
            if( gb->propertySet().size() == 0 )
            {
                debug() << "infeasible bias detected";
                gb->setActive(false);
            }
            else
            {
                m_feasibleGlobalBiases.append( gb );
                m_feasibleGlobalBiasSets.append( TrackSet( gb->propertySet() ) );
            }
        }
    }

    TrackSet subset;
    subset.setUniverseSet();

    for( int i = 0; i < m_feasibleGlobalBiases.size(); ++i )
    {
        if( m_feasibleGlobalBiases.at(i)->weight() == 1.0 )
            subset.intersect( m_feasibleGlobalBiasSets.at(i) );

        if( m_feasibleGlobalBiases.at(i)->weight() == 0.0 )
            subset.subtract( m_feasibleGlobalBiasSets.at(i) );
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
    QMutexLocker locker( &s_universeMutex );

    if( !s_universeOutdated )
        return;

    if( !s_universeQuery )
    {
        if( !s_universeCollection )
            s_universeCollection = CollectionManager::instance()->primaryCollection();

        s_universeQuery = s_universeCollection->queryMaker();
        s_universeQuery->setQueryType( QueryMaker::Custom );
        s_universeQuery->addReturnValue( QueryMaker::valUniqueId );
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
    foreach( QString uidString, uids )
    {
        if ( uidString.isEmpty() )
            continue;

        // for some reason we sometimes get uidt without the protocol part
        if( uidString.at( s_uidUrlProtocolPrefixLength - 1 ) != '/' )
            uid = QByteArray::fromHex( uidString.toAscii() );
        else
            uid = QByteArray::fromHex( uidString.mid(s_uidUrlProtocolPrefixLength).toAscii() );
            

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
        ThreadWeaver::Weaver::instance()->enqueue( this );
}

void
Dynamic::BiasSolver::outdateUniverse()
{
    QMutexLocker locker( &s_universeMutex );
    s_universeOutdated = true;
}

void
Dynamic::BiasSolver::setUniverseCollection( Collection* coll )
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

