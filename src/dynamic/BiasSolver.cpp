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


#include "BiasSolver.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "TrackSet.h"

#include <cmath>
#include <typeinfo>

#include <QHash>

#include <KRandom>

const int    Dynamic::BiasSolver::ITERATION_LIMIT = 5000;

/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be carefull */
const double Dynamic::BiasSolver::INITIAL_TEMPERATURE = 0.1;
const double Dynamic::BiasSolver::COOLING_RATE        = 0.8;



Dynamic::BiasSolver::BiasSolver( int n, QList<Bias*> biases, RandomPlaylist* randomSource, Meta::TrackList context )
    : m_biases(biases)
    , m_n(n)
    , m_context(context)
    , m_mutationSource(randomSource)
    , m_abortRequested(false)
{
    int i = m_biases.size();
    while( i-- )
        m_biasEnergy.append( 0.0 );
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

void Dynamic::BiasSolver::run()
{
    bool optimal = generateInitialPlaylist();

    // test for the empty collection case
    m_playlist.removeAll( Meta::TrackPtr() );
    if( m_playlist.empty() )
    {
        debug() << "Empty collection, aborting BiasSolver.";
        return;
    }

    if( optimal )
        return;

    m_E = energy();
    m_T = INITIAL_TEMPERATURE;

    //const double E_0 = m_E;

    int i = ITERATION_LIMIT;
    double epsilon = 1.0 / (double)m_n;
    while( i-- && m_E >= epsilon && !m_abortRequested )  
    {
        iterate();

        if( i % 250 == 0 )
        {
            debug() << "E = " << m_E;
            //int progress = (int)(100.0 * (1.0 - m_E/E_0));
            int progress = (int)(100.0 * (1.0 - m_E));
            emit statusUpdate( progress >= 0 ? progress : 0 );
        }
    }

    debug() << "BiasSolver: System solved in " << (ITERATION_LIMIT - i) << " iterations.";
    debug() << "with E = " << m_E;
}

Meta::TrackList Dynamic::BiasSolver::solution()
{
    return m_playlist;
}


void Dynamic::BiasSolver::iterate()
{
    Meta::TrackPtr mutation = getMutation();
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
    }

    // cool the temperature
    m_T *= COOLING_RATE;

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
            m_biasEnergy[i] = 
                m_biases[i]->reevaluate( 
                        m_biasEnergy[i], m_playlist, mutation, 
                        mutationPos, m_context );
            sum += qAbs( m_biasEnergy[i] );
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
    bool optimal = true;

    // First we make a list of all the global biases which are feasible.
    QList<Dynamic::GlobalBias*> globalBiases;
    foreach( Dynamic::Bias* b, m_biases )
    {
        Dynamic::GlobalBias* gb = dynamic_cast<Dynamic::GlobalBias*>( b );

        if( gb )
        {
            debug() << "property size: " << gb->propertySet().size();

            // if the bias in unsatisfiable (i.e. size = 0), just ignore it
            if( gb->propertySet().size() == 0 )
            {
                debug() << "unsitisfiable bias detected";
                gb->setActive(false);
            }
            else
                globalBiases.append( gb );
        }
        else
            optimal = false;

    }

    // Empty collection
    if( PlaylistBrowserNS::DynamicModel::instance()->universe().size() == 0 )
        return false;

    // No feasible global biases
    if( globalBiases.size() == 0 )
    {
        m_playlist = m_mutationSource->getTracks( m_n );
        return false;
    }

    // convert all the property sets to TrackSets.
    QList<TrackSet> propertySets;
    foreach( Dynamic::GlobalBias* b, globalBiases )
        propertySets.append( Dynamic::TrackSet( b->propertySet() ) );


    // We are going to be computing a lot of track set intersections so we will
    // memoize to try and save time (if not memory).
    QHash< QBitArray, QList<Meta::TrackPtr> > memoizedIntersections;



    // As we build up the playlist the weights for each bias will change to
    // reflect what proportion of the tracks that remain to be chosen should
    // have the property in question.
    double* movingWeights = new double[globalBiases.size()];
    for( int i = 0; i < globalBiases.size(); ++i )
        movingWeights[i] = globalBiases[i]->weight();

    //const QSet<Meta::TrackPtr>& U = PlaylistBrowserNS::DynamicModel::instance()->universe();
    //

    m_playlist.clear();

    // We use this array of indexes to randomize the order the biases are looked
    // at. That was we get reasonable results when the system is infeasible.
    QList<int> indexes;
    for( int i = 0; i < globalBiases.size(); ++i )
        indexes.append( i );


    Dynamic::TrackSet S, R;

    double decider;
    int n = m_n;
    while( --n )
    {
        // For each bias, we must make a decision whether the track being chosen
        // should belong to it or not. This is simply a probabalistic choice
        // based on the current weight. 


        // Randomize the order.
        int m = globalBiases.size();
        while( m > 1 )
        {
            int k = KRandom::random() % m;
            --m;
            indexes.swap( m, k );
        }


        // The bit array represents the choice made at each branch.
        QBitArray branches( globalBiases.size(), 0x0 );

        S.setUniverseSet();

        for( int _i = 0; _i < globalBiases.size(); ++_i )
        {
            int i = indexes[_i];

            R = S;

            // Decide whether we should 'accept' or 'reject' a bias.
            decider = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);
            if( decider < movingWeights[i] )
            {
                branches.setBit( i, true );
                R.intersect( propertySets[i] );
            }
            else
            {
                branches.setBit( i, false );
                R.subtract( propertySets[i] );
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
            memoizedIntersections[branches] = S.trackList();
        }

        const Meta::TrackList& finalSubset = memoizedIntersections[branches];

        // this should never happen
        if( finalSubset.size() == 0 )
        {
            warning() << "BiasSolver assumption failed.";
            m_playlist.append( m_mutationSource->getTrack() );
            continue;
        }

        // choose a track at random from our final subset
        int choice = KRandom::random() % finalSubset.size();
        m_playlist.append( finalSubset[choice] );

        if( optimal )
            emit statusUpdate( (int)(100.0 * (double)(m_n - n) / (double)n) );
    }

    return optimal;
}


Meta::TrackPtr Dynamic::BiasSolver::getMutation()
{
    return m_mutationSource->getTrack();
}

