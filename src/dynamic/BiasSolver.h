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
#include "TrackSet.h"

#include <threadweaver/Job.h>

#include <QWaitCondition>

namespace Dynamic
{

    class TrackListEnergyPair;

    /**
     * A class to implement the optimization algorithm used to generate
     * playlists from a set of biases. The method used here is called 
     * "Simulated Annealing". Read about it before trying to work on this.
     */
    class BiasSolver : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            BiasSolver( 
                    int n, QList<Bias*> biases,
                    Meta::TrackList context = Meta::TrackList() );

            Meta::TrackList solution();
            void requestAbort();
            bool success() const;

            void doWork();

            static void setUniverseCollection( Collection* );
            static const QList<QByteArray>& universe();
            static void outdateUniverse();

        protected:
            void run();

        signals:
            void statusUpdate( int progress );

        private slots:
            void biasUpdated();
            void universeResults( QString collectionId, QStringList );
            void universeUpdated();
            void iterateAnnealing( Meta::TrackPtr mutation );

        private:
            void anneal( int iterations, bool updateStatus = false ); //! run SA on m_playlist for a given number of itreations
            void computeDomain();
            void updateUniverse();
            double energy(); //! calculated the average energy for m_playlist
            double recalculateEnergy( Meta::TrackPtr mutation, int mutationPos );
            Meta::TrackPtr trackForUid( const QByteArray& );
            Meta::TrackPtr getMutation(); //! return a random track from the collection
            Meta::TrackPtr getRandomTrack( const QList<QByteArray>& subset );

            /**
             * Try to choose a good starting playlist using heuristics.
             * Returns true if the playlist generated is known to be optimal.
             */
            bool generateInitialPlaylist();

            /**
             * Choose MATING_POPULATION_SIZE playlists from the given population
             * to produce offpring.
             */
            QList<int> generateMatingPopulation( const QList<TrackListEnergyPair>& );


            QList<Bias*>  m_biases;     //! current energy for the whole system
            QList<double> m_biasEnergy; //! current energy of each indivial bias
            QList<double> m_biasMutationEnergy; //! individual bias energy for the mutation

            int m_n;                    //! size of playlist to generate
            double m_T;                 //! temperature
            double m_E;                 //! energy
            Meta::TrackList m_playlist; //! playlist being generated
            Meta::TrackList m_context;  //! tracks that precede the playlist
            double m_epsilon;           //! highest energy we consider optimal

            int m_pendingBiasUpdates;
            QMutex m_biasMutex;

            QList<QByteArray> m_domain;

            // set by computeDomain, used by generateInitialPlaylist
            QList<GlobalBias*> m_feasibleGlobalBiases;
            QList<TrackSet> m_feasibleGlobalBiasSets;

            bool m_abortRequested; //! flag set when the thread is aborted

            // The universe set: a list of every uid in the collection.
            static QList<QByteArray>  s_universe;
            static QMutex             s_universeMutex;
            static QueryMaker*        s_universeQuery;
            static Collection*        s_universeCollection;
            static bool               s_universeOutdated;
            static unsigned int       s_uidUrlProtocolPrefixLength;


            // CONSTANTS

            static const int    GA_ITERATION_LIMIT;     //! iteration limit for the genetic phase
            static const int    GA_POPULATION_SIZE;        //! population size for genetic phase
            static const int    GA_MATING_POPULATION_SIZE; //! how many offspring are produced each generation
            static const double GA_MUTATION_PROBABILITY; //! the chance that an offspring gets mutated
            static const int GA_GIVE_UP_LIMIT; //! if we can't reduce the energy after this many iteration, give up

            static const int    SA_ITERATION_LIMIT; //! iteration limit for the annealing phase
            static const double SA_INITIAL_TEMPERATURE;
            static const double SA_COOLING_RATE;
            static const int    SA_GIVE_UP_LIMIT;

    };

}

#endif
