/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef STATSYNCING_SYNCHRONIZETRACKSJOB_H
#define STATSYNCING_SYNCHRONIZETRACKSJOB_H

#include "core/meta/forward_declarations.h"
#include "statsyncing/Options.h"
#include "statsyncing/ScrobblingService.h"
#include "statsyncing/Track.h"

#include <ThreadWeaver/Job>

#include <QMap>

namespace StatSyncing
{
    class TrackTuple;

    /**
     * A job to call TrackTuple::synchronize() in order not to make delays in the main
     * loop.
     */
    class SynchronizeTracksJob : public QObject, public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            explicit SynchronizeTracksJob( const QList<TrackTuple> &tuples,
                                           const TrackList &trackToScrobble,
                                           const Options &options, QObject *parent = nullptr );

            /**
             * Return count of tracks that were updated during synchronization
             */
            int updatedTracksCount() const;

            /**
             * Return scrobble counts per scrobbling service and their status.
             */
            QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> > scrobbles();

        public Q_SLOTS:
            /**
             * Abort the job as soon as possible.
             */
            void abort();

        Q_SIGNALS:
            /**
             * Emitted when matcher gets to know total number of steps it will take to
             * match all tracks.
             */
            void totalSteps( int steps );

            /**
             * Emitted when one progress step has been finished.
             */
            void incrementProgress();

            /**
             * Emitted from worker thread when all time-consuming operations are done.
             */
            void endProgressOperation( QObject *owner );

            /**
             * Helper to cross thread boundary between this worker thread and main thread
             * where StatSyncing::Controller lives.
             */
            void scrobble( const Meta::TrackPtr &track, double playedFraction,
                           const QDateTime &time );

            /** This signal is emitted when this job is being processed by a thread. */
            void started(ThreadWeaver::JobPointer);
            /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
            void done(ThreadWeaver::JobPointer);
            /** This job has failed.
             * This signal is emitted when success() returns false after the job is executed. */
            void failed(ThreadWeaver::JobPointer);

        protected:
            void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
            void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
            void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override;

        private Q_SLOTS:
            void slotTrackScrobbled( const ScrobblingServicePtr &service, const Meta::TrackPtr &track );
            void slotScrobbleFailed( const ScrobblingServicePtr &service, const Meta::TrackPtr &track, int error );

        private:
            bool m_abort;
            QList<TrackTuple> m_tuples;
            TrackList m_tracksToScrobble;
            QSet<Meta::TrackPtr> m_scrobbledTracks;
            QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> > m_scrobbles;
            int m_updatedTracksCount;
            const Options m_options;
    };

} // namespace StatSyncing

#endif // STATSYNCING_SYNCHRONIZETRACKSJOB_H
