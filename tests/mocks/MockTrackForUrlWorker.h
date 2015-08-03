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

#ifndef MOCKTRACKFORURLWORKER_H
#define MOCKTRACKFORURLWORKER_H

#include "core/collections/support/TrackForUrlWorker.h"

#include <QtTest>

using namespace Amarok;

class MockTrackForUrlWorker : public TrackForUrlWorker
{
    Q_OBJECT

    public:
        MockTrackForUrlWorker(const QUrl &url);
        MockTrackForUrlWorker(const QString &url);

        /**
         * Mock implementation that fetches the track from QTest data-driven testing
         * variable named track and assigns it to m_track
         */
        virtual void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0);
};

#endif // MOCKTRACKFORURLWORKER_H
