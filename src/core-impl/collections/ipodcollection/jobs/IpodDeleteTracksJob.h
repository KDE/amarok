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

#ifndef IPODDELETETRACKSJOB_H
#define IPODDELETETRACKSJOB_H

#include "IpodCollection.h"
#include "core/meta/forward_declarations.h"

#include <ThreadWeaver/Job>


class IpodDeleteTracksJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        explicit IpodDeleteTracksJob( const Meta::TrackList &sources,
                                      const QPointer<IpodCollection> &collection );
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override;

    Q_SIGNALS:
        // signals for progress operation:
        void incrementProgress();
        void endProgressOperation( QObject *obj );
        void totalSteps( int steps ); // not used, defined to keep QObject::connect warning quiet

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

    private:
        Meta::TrackList m_sources;
        QPointer<IpodCollection> m_coll;
};

#endif // IPODDELETETRACKSJOB_H
