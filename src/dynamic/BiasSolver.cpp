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

const int    Dynamic::BiasSolver::ITERATION_LIMIT = 50000;

// TODO: Actually, this should be dependant on the playlist size.
const double Dynamic::BiasSolver::EPSILON = 0.0001;

/* These number are black magic. The best values can only be obtained through
 * exhaustive trial and error or writing another optimization program to
 * optimize this optimization program. They are very sensitive. Be carefull */
const double Dynamic::BiasSolver::INITIAL_TEMPERATURE = 0.1;
const double Dynamic::BiasSolver::COOLING_RATE        = 0.6;


Dynamic::BiasSolver::BiasSolver( int n, QList<Bias*> biases, RandomPlaylist* randomSource )
    : m_biases(biases), m_n(n), m_mutationSource( randomSource )
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
    if( m_playlist.empty() ) return;

    int i = ITERATION_LIMIT;
    while( i-- && m_E > EPSILON )  
    {
        iterate();

        if( i % 100 == 0 )
        {
            debug() 
                << "BiasSolver: (i, E, T) = (" 
                << ITERATION_LIMIT - i << ", "
                << m_E << ", " 
                << m_T << ")";
        }
    }

    setFinished( true );
}

Meta::TrackList Dynamic::BiasSolver::solution()
{
    return m_playlist;
}


void Dynamic::BiasSolver::initialize()
{
    CollectionDependantBias* cb;
    foreach( Bias* b, m_biases )
    {
        if( (cb = dynamic_cast<CollectionDependantBias*>( b ) ) )
        {
            if( cb->needsUpdating() )
                cb->update();
        }
    }

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
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
       sum += (m_biasEnergy[i] = m_biases[i]->energy( m_playlist ));
    }

    return sum / (double)m_biases.size();
}


double Dynamic::BiasSolver::recalculateEnergy( Meta::TrackPtr mutation, int mutationPos )
{
    double sum = 0.0;
    for( int i = 0; i < m_biases.size(); ++i )
    {
        m_biasEnergy[i] = m_biases[i]->reevaluate( m_E, m_playlist, mutation, mutationPos );
        sum += m_biasEnergy[i];
    }

    return sum / (double)m_biases.size();
}


void Dynamic::BiasSolver::generateInitialPlaylist()
{
    // TODO: implement a greedy heuristic that prefers tracks that are rare but
    // in high demand by global biases.
   
    m_playlist = m_mutationSource->getTracks( m_n );
}


Meta::TrackPtr Dynamic::BiasSolver::getMutation()
{
    return m_mutationSource->getTrack();
}

