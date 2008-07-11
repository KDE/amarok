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

#include <cmath>
#include <typeinfo>
#include <KRandom>

const int    Dynamic::BiasSolver::ITERATION_LIMIT = 5000;

/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be carefull */
const double Dynamic::BiasSolver::INITIAL_TEMPERATURE = 0.1;
const double Dynamic::BiasSolver::COOLING_RATE        = 0.8;


Dynamic::BiasSolver::BiasSolver(
        int n, int domainSize,
        QList<Bias*> biases, RandomPlaylist* randomSource,
        Meta::TrackList context )
    : m_biases(biases)
    , m_n(n)
    , m_context(context)
    , m_mutationSource( randomSource )
    , m_domainSize((double)domainSize)
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

    int i = ITERATION_LIMIT;
    double epsilon = 1.0 / (double)m_n;
    while( i-- && m_E >= epsilon  )  
    {
        iterate();
    }

    debug() << "BiasSolver: System solved in " << (ITERATION_LIMIT - i) << " iterations.";
    debug() << "with E = " << m_E;

    setFinished( true );
}

Meta::TrackList Dynamic::BiasSolver::solution()
{
    return m_playlist;
}


void Dynamic::BiasSolver::initialize()
{
    DEBUG_BLOCK

    // TODO:
    // - filter out absolute global biases (those with weights of 0.0 or 1.0
    // - filter out infeasible biases

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
                        m_E, m_playlist, mutation, 
                        mutationPos, m_context );
            sum += qAbs( m_biasEnergy[i] );
            activeBiases++;
        }
    }

    return sum / (double)activeBiases;
}


void Dynamic::BiasSolver::generateInitialPlaylist()
{
    DEBUG_BLOCK
    // This confusing bit of code is a greedy heuristic that tries to choose
    // tracks that are rare but in high demand by global biases. That way we
    // don't eat up a lot of iterations looking for them.

    QList<Dynamic::GlobalBias*> globalBiases;
    QList<double> weights;
    double totalWeight = 0.0;

    // Ahhh...empty collection!
    if( m_domainSize == 0.0 ) 
        return;


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
            {
                globalBiases.append( gb );
                // this is the the difference between the desired proportion and
                // the actual probability a track with that propery is chosen.
                double deviance =
                    gb->weight() - (double)gb->propertySet().size()/m_domainSize;

                debug() << "deviance = " << deviance;
                weights.append( deviance );
                totalWeight += qAbs( weights.last() );
            }
        }
    }

    if( globalBiases.isEmpty() )
    {
        m_playlist = m_mutationSource->getTracks( m_n );
        return;
    }

    // whatever, we'll just use a random sample
    if( globalBiases.isEmpty() )
        m_playlist = m_mutationSource->getTracks( m_n );

    // we need these as lists to get random value from
    QList< Meta::TrackList > propertySets;
    foreach( Dynamic::GlobalBias* gb, globalBiases )
    {
        propertySets.append( gb->propertySet().toList() );
    }

    m_playlist.clear();
    int n = m_n;
    int active;
    double activeDecider;
    while( n-- )
    {
        // choose the active bias
        activeDecider = totalWeight * (double)KRandom::random() / (((double)RAND_MAX) + 1.0);

        active = 0;
        while( activeDecider > 0.0 && active < globalBiases.size()-1 )
        {
            if( activeDecider > weights[active] )
            {
                activeDecider -= weights[active];
                active++;
            }
            else break;
        }

        if( weights[active] < 0.0 )
        {
            // TODO: choose a random track from the set's complement
            m_playlist.append( m_mutationSource->getTrack() );
        }
        else
        {
            int choice = KRandom::random() % propertySets[active].size();
            m_playlist.append( propertySets[active][choice] );
        }
    }
}


Meta::TrackPtr Dynamic::BiasSolver::getMutation()
{
    return m_mutationSource->getTrack();
}

