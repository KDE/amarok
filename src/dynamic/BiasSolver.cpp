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

#include <cmath>
#include <typeinfo>

#include <QBitArray>
#include <QHash>

#include <KRandom>

const int    Dynamic::BiasSolver::ITERATION_LIMIT = 10000;

/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be carefull */
const double Dynamic::BiasSolver::INITIAL_TEMPERATURE = 0.1;
const double Dynamic::BiasSolver::COOLING_RATE        = 0.8;


// Logical xor. It comes in handy.
static bool LXOR( bool a, bool b )
{
    return (a && !b) || (!a && b);
}

Dynamic::BiasSolver::BiasSolver( int n, QList<Bias*> biases, RandomPlaylist* randomSource, Meta::TrackList context )
    : m_biases(biases)
    , m_n(n)
    , m_context(context)
    , m_mutationSource( randomSource )
{
    int i = m_biases.size();
    while( i-- )
        m_biasEnergy.append( 0.0 );
}

void Dynamic::BiasSolver::run()
{
    initialize();

    // test for the empty collection case
    m_playlist.removeAll( Meta::TrackPtr() );
    if( m_playlist.empty() )
    {
        debug() << "Empty collection, aborting BiasSolver.";
        return;
    }

    const double E_0 = m_E;

    int i = ITERATION_LIMIT;
    double epsilon = 1.0 / (double)m_n;
    while( i-- && m_E >= epsilon  )  
    {
        iterate();

        if( i % 250 == 0 )
        {
            debug() << "E = " << m_E;
            int progress = (int)(100.0 * (1.0 - m_E/E_0));
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


void Dynamic::BiasSolver::initialize()
{
    generateInitialPlaylist();

    m_E = energy();
    m_T = INITIAL_TEMPERATURE;
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


void
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
    }

    // Empty collection
    if( PlaylistBrowserNS::DynamicModel::instance()->universe().size() == 0 )
        return;

    // No feasible global biases
    if( globalBiases.size() == 0 )
    {
        m_playlist = m_mutationSource->getTracks( m_n );
        return;
    }


    // We are going to be computing a lot of track set intersections so we will
    // memoize to try and save time (if not memory).
    QHash< QBitArray, QSet<Meta::TrackPtr> > memoizedIntersections;


    // As we build up the playlist the weights for each bias will change to
    // reflect what proportion of the tracks that remain to be chosen should
    // have the property in question.
    double* movingWeights = new double[globalBiases.size()];
    for( int i = 0; i < globalBiases.size(); ++i )
        movingWeights[i] = globalBiases[i]->weight();


    double decider;
    int n = m_n;
    while( --n )
    {
        // For each bias, we must make a decision whether the track being chosen
        // should belong to it or not. This is simply a probabalistic choice
        // based on the current weight. 

        // The bit array represents the choice made at each branch. (1 = accept
        // the bias, 0 = reject the bias).
        QBitArray intersect;
        QSet<Meta::TrackPtr> S = PlaylistBrowserNS::DynamicModel::instance()->universe();
        for( int i = 0; i < globalBiases.size(); ++i )
        {

            // Decide whether we should 'accept' or 'reject' a bias.
            intersect.resize( intersect.size() + 1 );
            decider = (double)KRandom::random() / (((double)RAND_MAX) + 1.0);
            if( decider < movingWeights[i] )
            {
                //debug() << "+?";
                intersect.setBit( intersect.size()-1, true );
                if( !memoizedIntersections.contains(intersect) )
                    memoizedIntersections[intersect] = S.intersect( globalBiases[i]->propertySet() );
            }
            else
            {
                //debug() << "-?";
                intersect.setBit( intersect.size()-1, false );
                if( !memoizedIntersections.contains(intersect) )
                    memoizedIntersections[intersect] = S.subtract( globalBiases[i]->propertySet() );
            }

            int newSize = memoizedIntersections[intersect].size();

            // Now we have to make sure our decision doesn't land us with an
            // empty set. If that's the case, we have to choose the other
            // branch, even if it does defy the probability. (This is how we
            // deal with infeasible systems.)

            // the 'accept' branch
            if( LXOR( intersect.testBit(intersect.size()-1), newSize == 0 ) )
            {
                //debug() << "+";
                movingWeights[i] = (movingWeights[i]*(double)(n+1)-1.0)/(double)n;
                intersect.setBit( intersect.size()-1, true );
                if( !memoizedIntersections.contains(intersect) )
                    memoizedIntersections[intersect] = S.intersect( globalBiases[i]->propertySet() );
            }
            // the 'reject' branch
            else
            {
                //debug() << "-";
                movingWeights[i] = (movingWeights[i]*(double)(n+1))/(double)n;
                intersect.setBit( intersect.size()-1, false );
                if( !memoizedIntersections.contains(intersect) )
                    memoizedIntersections[intersect] = S.subtract( globalBiases[i]->propertySet() );
            }

            //debug() << "i now has weight: " << movingWeights[i];

            S = memoizedIntersections[intersect];
            //debug() << "S.size = " << S.size();
        }

        // Now just convert the set we are left with into a list and choose a
        // random track from it.
        QList<Meta::TrackPtr> SList = S.toList();
        int choice = KRandom::random() % S.size();
        m_playlist.append( SList[choice] );
    }
}


Meta::TrackPtr Dynamic::BiasSolver::getMutation()
{
    return m_mutationSource->getTrack();
}

