/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2010, 2013 Ralf Engels <ralf-engels@gmx.de>                            *
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
#include "core/meta/forward_declarations.h"
#include "dynamic/TrackSet.h"

#include <QDateTime>
#include <QMutex>
#include <QWaitCondition>

#include <ThreadWeaver/Job>

namespace Dynamic
{

    /** A playlist helper class
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


     * Playlist generation is now done by adding tracks until we run out of time
     * or out of candidates.
     * We are back stepping a couple of times in such a case.
     *
     * The old bias solver that tried different optimization solutions is
     * not longer used. If you want to see the old code and/or re-use parts
     * of it see Amarok 2.7
     *
     * To use the solver:
     * Create an instance
     * enqueue the job. When the job is finished, call solution to get the
     * playlist produced.
     */
    class BiasSolver : public QObject, public ThreadWeaver::Job
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
            virtual void requestAbort() override;

            /**
             * Returns true if the solver was successful, false if it was
             * aborted or encountered some other error.
             */
            virtual bool success() const  override;

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
            void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0)  override;
            void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread)  override;
            void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread)  override;

        Q_SIGNALS:
            /** a job must implement the following signals for the progress bar
                BiasedPlaylist set's us as progress sender in startSolver.
            */

            /** Sets the total steps for the progress bar (which is always 100 for the bias solver).
                This signal is never emitted as the BiasedPlaylist already registers us with
                100 steps. */
            void totalSteps( int );
            void incrementProgress();
            void endProgressOperation( QObject * );

            /** This signal is emitted when this job is being processed by a thread. */
            void started(ThreadWeaver::JobPointer);
            /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
            void done(ThreadWeaver::JobPointer);
            /** This job has failed.
             * This signal is emitted when success() returns false after the job is executed. */
            void failed(ThreadWeaver::JobPointer);

        private Q_SLOTS:
            void biasResultReady( const Dynamic::TrackSet &set );

            void trackCollectionResultsReady( QStringList );
            void trackCollectionDone();


        private:
            /** Returns the TrackSet that would match the end of the given playlist
                The function blocks until the result is received */
            TrackSet matchingTracks( const Meta::TrackList& playlist ) const;

            /** Query for the universe set (the set of all tracks in the collection being considered.
                This function needs to be called from a thread with an event loop.
            */
            void getTrackCollection();

            /** Try to recursive add tracks to the current solver list up to m_n tracks.
             *  The function will return with partly filled lists.
             */
            void addTracks( SolverList *list );

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

            /** Returns a TrackSet with all duplicates removed (except the one at "position")
                This helper function can be used in special cases if needed and
                AmarokConfig::dynamicDuplicates() == false
                Normally the BiasSolver will call it at for the top most bias.
            */
            static TrackSet withoutDuplicate( int position,
                                              const Meta::TrackList& playlist,
                                              const Dynamic::TrackSet& oldSet );

            /** Emits the required progress signals */
            void updateProgress( const SolverList* list );

            int m_n;                    //!< size of playlist to generate
            Dynamic::BiasPtr m_bias; // bias used to determine tracks. not owned by solver
            Meta::TrackList m_context;  //!< tracks that precede the playlist

            Meta::TrackList m_solution;

            bool m_abortRequested; //!< flag set when the thread is aborted

            QDateTime m_startTime;

            mutable QMutex m_biasResultsMutex;
            mutable QWaitCondition m_biasResultsReady;
            mutable Dynamic::TrackSet m_tracks; // tracks just received from the bias.

            mutable QMutex m_collectionResultsMutex;
            mutable QWaitCondition m_collectionResultsReady;

            /** All uids of all the tracks in the collection */
            QStringList m_collectionUids;
            TrackCollectionPtr m_trackCollection;

            bool m_allowDuplicates;

            int m_currentProgress;

            /** The maximum time we should try to spend generating the playlist */
            static const int MAX_TIME_MS = 5000;
    };
}

#endif
