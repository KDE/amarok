/****************************************************************************************
 * Copyright (c) 2012 Jasneet Singh Bhatti <jazneetbhatti@gmail.com>                    *
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

#include "MockTrackForUrlWorker.h"

#include "core/meta/Meta.h"

MockTrackForUrlWorker::MockTrackForUrlWorker( const QUrl &url )
    : TrackForUrlWorker( url )
{
}

MockTrackForUrlWorker::MockTrackForUrlWorker( const QString &url )
    : TrackForUrlWorker( url )
{
}

void
MockTrackForUrlWorker::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(thread);
    
    Q_EMIT started(self);
    
    QFETCH( Meta::TrackPtr, track );
    m_track = track;
    
    Q_EMIT done(self);
}
