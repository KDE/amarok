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

#ifndef AMAROK_BIASSOLVER_H
#define AMAROK_BIASSOLVER_H

#include "Bias.h"
#include "core/meta/Meta.h"
#include "TrackSet.h"

#include <threadweaver/Job.h>

#include <QMutex>
#include <QWaitCondition>

namespace Dynamic
{

    /** A playlist/energy pair, used by ga_optimize to sort lists of playlists by their energy.
    */
    class SolverList;

    /**
     * A class to implement the optimization algorithm used to generate
     * playlists from a set of biases. The method used here is slightly
     * complicated. It uses a a combination of heuristics, genetic algorithms,
     * and simulated annealing. The steps involved are documented in the source
     * code of BiasSolver::run.

       The whole operation is a little bit tricky since the bias solver runs as
       separate job without it's own event queue.

       The signals and slots are all handled via the UI threads event queue.
       Since the BiasSolver and all biases are created from the UI thread this
       is also the event queue of all objects.
       Bottom line: don't create objects in the bias solver that use signals.


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
     * Create an instance
     * enqueue the job. When the job is finished, call solution to get the
     * playlist produced.
     */
    class BiasSolver : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:

            /**
             * Create and prepare the solver. The constructor returns
             *
             * @param n The size of the playlist to generate.
             * @param biases The system of biases being applied.
             * @param context The tracks (if any) that precede the playlist
             * being generated.
             */
            BiasSolver( int n, BiasPtr bias, Meta::TrackList context );

            ~BiasSolver();

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
             * Choose whether the BiasSolver instance should delete itself after the query.
             * By passing true the instance will delete itself after emitting done, failed.
             * Otherwise it is the responsibility of the owner to delete the instance
             * when it is not needed anymore.
             *
             * Defaults to false, i.e. the BiasSolver instance will not delete itself.
             */
            void setAutoDelete( bool autoDelete );

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
            /// A [0,100] value emitted occasionally to indicate progress being made.
            void statusUpdate( int progress );

        private slots:
            void biasResultReady( const Dynamic::TrackSet &set );

            void trackCollectionResultsReady( QStringList );
            void trackCollectionDone();


        private:
            double epsilon() const
            { return 0.05; }

            /** Returns the TrackSet of tracks fitting in the indicated position.
                The function blocks until the result is received */
            TrackSet matchingTracks( int position, const Meta::TrackList& playlist ) const;

            /** Query for the universe set (the set of all tracks in the collection being considered.
                This function needs to be called from a thread with an event loop.
            */
            void getTrackCollection();


            /**
             * Try to produce better playlist by replacing all tracks by tracks that fulfill the bias
             */
            void simpleOptimize( SolverList *list );

            /**
             * Optimize a playlist using simulated annealing.
             * If the given initial playlist is already optimal it does not do anything.
             *
             * @param initialPlaylist The starting playlist for the algorithm.
             * @param iterationLimit Maximum iterations allowed.
             * @param updateStatus Enable/disable statusUpdate signals.
             */
            void annealingOptimize( SolverList *list, int iterationLimit, bool updateStatus = true );

            /**
             * Try to produce an optimal playlist using a genetic algorithm, and
             * return the best playlist produced.
             *
             * @param iterationLimit Maximum iterations allowed.
             * @param updateStatus Enable/disable statusUpdate signals.
             */
            void geneticOptimize( SolverList *list, int iterationLimit, bool updateStatus = true );

            /** Returns a simple random playlist without caring about any bias */
            SolverList generateInitialPlaylist() const;

            /**
             * Get the track referenced by the uid stored in the given
             * QByteArray.
             * @param uid The uid
             */
            Meta::TrackPtr trackForUid( const QString& uid ) const;

            /**
             * Return a random track from the domain.
             */
            Meta::TrackPtr getMutation();

            /**
             * Return a random track from the given subset.
             * @param subset A list (representing a set) of uids stored in
             * QByteArrays.
             */
            Meta::TrackPtr getRandomTrack( const TrackSet& subset ) const;


            /**
             * Used each iteration of the genetic algorithm. Choose
             * MATING_POPULATION_SIZE playlists from the given population to
             * produce offspring.
             */
            QList<int> generateMatingPopulation( const QList<SolverList>& );

            /** Returns a TrackSet with all duplicates removed (except the one at "position")
                This helper function can be used in special cases if needed and
                AmarokConfig::dynamicDuplicates() == false
                Normally the BiasSolver will call it at for the top most bias.
                @param onlyBackwards if true it will not consider tracks after "position"
            */
            static TrackSet withoutDuplicate( int position,
                                              const Meta::TrackList& playlist,
                                              const Dynamic::TrackSet& oldSet,
                                              bool onlyBackwards = true );

            int m_n;                    //!< size of playlist to generate
            Dynamic::BiasPtr m_bias; // bias used to determine tracks. not owned by solver
            Meta::TrackList m_context;  //!< tracks that precede the playlist

            Meta::TrackList m_solution;
            Meta::TrackList m_mutationPool; //!< a queue of tracks used by getMutation

            bool m_abortRequested; //!< flag set when the thread is aborted

            mutable QMutex m_biasResultsMutex;
            mutable QWaitCondition m_biasResultsReady;
            mutable Dynamic::TrackSet m_tracks; // tracks just received from the bias.

            mutable QMutex m_collectionResultsMutex;
            mutable QWaitCondition m_collectionResultsReady;

            /** All uids of all the tracks in the collection */
            QStringList m_collectionUids;
            TrackCollectionPtr m_trackCollection;

            bool m_allowDuplicates;


            // GENETIC ALGORITHM CONSTANTS

            static const int    GA_ITERATION_LIMIT;        //!< iteration limit for the genetic phase
            static const int    GA_POPULATION_SIZE;        //!< population size for genetic phase
            static const int    GA_MATING_POPULATION_SIZE; //!< how many offspring are produced each generation
            static const double GA_MUTATION_PROBABILITY;   //!< the chance that an offspring gets mutated
            static const int    GA_GIVE_UP_LIMIT;          //!< if we can't reduce the energy after this many iterations, give up

            // SIMULATE ANNEALING CONSTANTS

            static const int    SA_ITERATION_LIMIT;     //!< iteration limit for the annealing phase
            static const double SA_INITIAL_TEMPERATURE; //!< initial value of T
            static const double SA_COOLING_RATE;        //!< the factor by which T is multiplied each iteration.
            static const int    SA_GIVE_UP_LIMIT;       //!< if we con't reduce the energy after this many iterations, give up

    };

}

#endif
