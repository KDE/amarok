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

#include "statsyncing/Options.h"

#include <ThreadWeaver/Job>

namespace StatSyncing
{

class TrackTuple;
    /**
     * A job to call TrackTuple::synchronize() in order not to make delays in the main
     * loop.
     */
    class SynchronizeTracksJob : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            explicit SynchronizeTracksJob( const QList<TrackTuple> &tuples,
                                           const Options &options, QObject *parent = 0 );

        public slots:
            /**
             * Abort the job as soon as possible.
             */
            void abort();

        signals:
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
            void endProgressOperation( QObject *owner, int updatedTracksCount = 0 );

        protected:
            virtual void run();

        private:
            bool m_abort;
            QList<TrackTuple> m_tuples;
            const Options m_options;
    };

} // namespace StatSyncing

#endif // STATSYNCING_SYNCHRONIZETRACKSJOB_H
