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

#ifndef AMAROK_BIASSOLVER_H
#define AMAROK_BIASSOLVER_H

#include "Bias.h"
#include "Meta.h"
#include "RandomPlaylist.h"

#include <threadweaver/Job.h>

namespace Dynamic
{

    /**
     * A class to implement the optimization algorithm used to generate
     * playlists from a set of biases. The method used here is called 
     * "Simulated Annealing". Read about it before trying to work on this.
     */
    class BiasSolver : public ThreadWeaver::Job
    {
        public:
            BiasSolver( int n, QList<Bias*> biases, RandomPlaylist* randomSource );

            Meta::TrackList solution();

        protected:
            void run();


        private:
            void initialize();
            void iterate();

            double energy();
            double recalculateEnergy( Meta::TrackPtr mutation, int mutationPos );

            void generateInitialPlaylist();
            Meta::TrackPtr getMutation();

            QList<Bias*>  m_biases;     //! current energy for the whole system
            QList<double> m_biasEnergy; //! current energy of each indivial bias

            int m_n;                    //! size of playlist to generate
            double m_T;                 //! temperature
            double m_E;                 //! energy
            Meta::TrackList m_playlist; //! playlist being generated

            RandomPlaylist* m_mutationSource;

            static const int    ITERATION_LIMIT; //! give up after this many iterations

            static const double INITIAL_TEMPERATURE;
            static const double COOLING_RATE;
    };

}

#endif
