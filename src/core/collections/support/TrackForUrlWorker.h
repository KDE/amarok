/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef TRACKFORURLWORKER_H
#define TRACKFORURLWORKER_H

#include "core/amarokcore_export.h"
#include "core/support/Amarok.h"
#include "core/meta/forward_declarations.h"

#include <QUrl>

#include <ThreadWeaver/Job>

namespace Amarok
{
/**
 * Derive from this class and implement the run() method to set mTrack.
 * @author Casey Link
 */
class AMAROK_CORE_EXPORT TrackForUrlWorker : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    TrackForUrlWorker( const QUrl &url );
    TrackForUrlWorker( const QString &url );
    ~TrackForUrlWorker();

    virtual void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) = 0;

Q_SIGNALS:
    void finishedLookup( const Meta::TrackPtr &track );

    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed. */
    void failed(ThreadWeaver::JobPointer);
    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);

protected:
    QUrl m_url;
    Meta::TrackPtr m_track;

private Q_SLOTS:
    void completeJob();
};

}
#endif // TRACKFORURLWORKER_H
