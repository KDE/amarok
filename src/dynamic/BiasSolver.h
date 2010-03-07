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

#ifndef AMAROK_BIASSOLVER_H
#define AMAROK_BIASSOLVER_H

#include "Bias.h"
#include "Meta.h"
#include "TrackSet.h"

#include <threadweaver/Job.h>

#include <QWaitCondition>

namespace Dynamic
{

    struct TrackListEnergyPair;

    /**
     * A class to implement the optimization algorithm used to generate
     * playlists from a set of biases. The method used here is slightly
     * complicated. It uses a a combination of heuristics, genetic algorithms,
     * and simulated annealing. The steps involved are documented in the source
     * code of BiasSolver::run.
     *
     * Playlist generation is treated as a minimization problem. We try to
     * produce a playlist with the lowest "energy", which can be thought of as
     * the badness of the playlist. High energy means the biases are poorly
     * satisfied, low energy means they are well satisfied. The energy value
     * is an average of all the energy values from all the bias in the system.
     *
     * For a better, more comprehensive description of the problem of playlist
     * generation, read M.P.H. Vossen's masters thesis: "Local Search for
     * Automatic Playlist Generation."
     *
     * To use the solver:
     * Create an instance, call prepareToRun, and when readyToRun is emitted,
     * enqueue the job. When the job is finished, call solution to get the
     * playlist produced.
     */
    class BiasSolver : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:

            /**
             * Create and prepare the solver. The constructor returns
             * immediately, but the solver will not be fully constructed until
             * readyToRun is emitted. Do not enqueue the job before the signal is
             * emitted.
             *
             * @param n The size of the playlist to generate.
             * @param biases The system of biases being applied.
             * @param context The tracks (if any) that precede the playlist
             * being generated.
             */
            BiasSolver( 
                    int n, QList<Bias*> biases,
                    Meta::TrackList context = Meta::TrackList() );

            ~BiasSolver();
            
            /**
             * Before the solver is run, it must prepared. This function returns
             * immediately, but the preparation is not finished until readyToRun
             * is emitted. Do not enqueue the job before then.
             */
            void prepareToRun();

            /// Returns the playlist generated after the job has finished.
            Meta::TrackList solution();

            /// Politely asks the thread to give up and finish ASAP.
            void requestAbort();

            /**
             * Returns true if the solver was successful, false if it was
             * aborted or encountered some other error.
             */
            bool success() const;

            /**
             * Set the collection that will be used when generating the
             * playlist.
             */
            static void setUniverseCollection( Amarok::Collection* );

            /**
             * Return the universe set (a list of the uid of every track being
             * considered) being used.
             */
            static const QList<QByteArray>& universe();

            /**
             * Mark the universe set as out of date (i.e. it needs to be
             * updated).
             */
            static void outdateUniverse();

        protected:
            void run();

        signals:
            /**
             * A signal emitted after prepareToRun is called and the solver is
             * prepared and ready to run.
             */
            void readyToRun();

            /// A [0,100] value emitted occasionally to indicate progress being made.
            void statusUpdate( int progress );

        private slots:
            void biasUpdated();
            void universeResults( QString collectionId, QStringList );
            void universeUpdated();


            /**
             * Query for the universe set (the set of all tracks in the
             * collection being considered).
             */
            void updateUniverse();
            
        private:
            /** 
             * Try to produce an optimal playlist using a genetic algorithm, and
             * return the best playlist produced.
             *
             * @param iterationLimit Maximum iterations allowed.
             * @param updateStatus Enable/disable statusUpdate signals.
             */

            Meta::TrackList ga_optimize( int iterationLimit, bool updateStatus = true );
            /**
             * Optimize a playlist using simulated annealing.
             *
             * @param initialPlaylist The starting playlist for the algorithm.
             * @param iterationLimit Maximum iterations allowed.
             * @param updateStatus Enable/disable statusUpdate signals.
             */
            void sa_optimize( Meta::TrackList& initialPlaylist, int iterationLimit, bool updateStatus = true );


            /**
             * Figures out what subset of the collection eligible tracks come
             * from. If there are global biases set at 0 or 100 percent, we can
             * exclude all those tracks, or include only those tracks,
             * respectively.
             */
            void computeDomain();


            /**
             * Calculate and return the composite energy ("badness") of the
             * given playlist.
             *
             * @param playlist The playlist to calculate the energy of.
             */
            double energy( const Meta::TrackList& playlist );


            /**
             * If we are just swapping one track (which we do a lot of), some
             * biases can take advantage of that, and avoid completely
             * recalculating the energy.
             *
             * @param playlist The playlist to calculate the energy of.
             * @param mutation The track being swapped into playlist.
             * @param mutationPos The proposed position in the playlist for
             * mutation.
             */
            double recalculateEnergy( const Meta::TrackList& playlist, Meta::TrackPtr mutation, int mutationPos );


            /**
             * Get the track referenced by the uid stored in the given
             * QByteArray.
             * @param uid The uid stored numerically as a QByteArray.
             */
            Meta::TrackPtr trackForUid( const QByteArray& uid );

            /**
             * Return a random track from the domain.
             */
            Meta::TrackPtr getMutation();

            /**
             * Return a random track from the given subset.
             * @param subset A list (representing a set) of uids stored in
             * QByteArrays.
             */
            Meta::TrackPtr getRandomTrack( const QList<QByteArray>& subset );


            /**
             * Try to choose a good starting playlist using heuristics. The
             * details are a bit complicated, the algorithm is documented in the
             * source code.
             *
             * @parem optimal A flag that is set if the generated playlist is
             * known to be optimal.
             */
            Meta::TrackList generateInitialPlaylist( bool& optimal );

            /**
             * Used each iteration of the genetic algorithm. Choose
             * MATING_POPULATION_SIZE playlists from the given population to
             * produce offspring.
             */
            QList<int> generateMatingPopulation( const QList<TrackListEnergyPair>& );

            Meta::TrackList m_solution;

            Meta::TrackList m_mutationPool; //! a queue of tracks used by getMutation

            QList<Bias*>  m_biases;     //! current energy for the whole system
            QList<double> m_biasEnergy; //! current energy of each individual bias
            QList<double> m_biasMutationEnergy; //! individual bias energy for the mutation

            int m_n;                    //! size of playlist to generate
            Meta::TrackList m_context;  //! tracks that precede the playlist
            double m_epsilon;           //! highest energy we consider optimal

            int m_pendingBiasUpdates;

            QList<QByteArray> m_domain; //! set of tracks being considered, potentially different than s_universe.

            /** List of biases which are global biases and are feasible (their
             * sets are non-empty). Set by computeDomain, but stored here so
             * generateInitialPlaylist can make use of it.
             */
            QList<CollectionFilterCapability*> m_feasibleCollectionFilters;
            QList<TrackSet> m_feasibleCollectionFilterSets;

            bool m_abortRequested; //! flag set when the thread is aborted

            /**
             * We keep a list here of the uid of every track in the set
             * collection being considered. This is unfortunately necessary
             * because the algorithm in generateInitialPlaylist performs many
             * set subtractions and intersections which would be impractical and
             * inefficient to perform using database queries. Instead we
             * represent a set of tracks as a bit list, where the n'th bit
             * indicates whether the n'th track in s_universe is included in the
             * set. Set operations can then be performed extremely quickly using
             * bitwise operations, rather than tree operations which QSet would
             * use.
             */
            static QList<QByteArray>  s_universe;
            static QMutex             s_universeMutex;
            static QueryMaker*        s_universeQuery;
            static Amarok::Collection*  s_universeCollection;
            static bool               s_universeOutdated;
            static unsigned int       s_uidUrlProtocolPrefixLength;


            // GENETIC ALGORITHM CONSTANTS

            static const int    GA_ITERATION_LIMIT;        //! iteration limit for the genetic phase
            static const int    GA_POPULATION_SIZE;        //! population size for genetic phase
            static const int    GA_MATING_POPULATION_SIZE; //! how many offspring are produced each generation
            static const double GA_MUTATION_PROBABILITY;   //! the chance that an offspring gets mutated
            static const int    GA_GIVE_UP_LIMIT;          //! if we can't reduce the energy after this many iterations, give up

            // SIMULATE ANNEALING CONSTANTS

            static const int    SA_ITERATION_LIMIT;     //! iteration limit for the annealing phase
            static const double SA_INITIAL_TEMPERATURE; //! initial value of T
            static const double SA_COOLING_RATE;        //! the factor by which T is multiplied each iteration.
            static const int    SA_GIVE_UP_LIMIT;       //! if we con't reduce the energy after this many iterations, give up

    };

}

#endif
