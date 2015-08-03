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

#ifndef TESTTRACKFORURLWORKER_H
#define TESTTRACKFORURLWORKER_H

#include "core/meta/forward_declarations.h"
#include "core/meta/Meta.h"

#include <QtTest>

class MockTrackForUrlWorker;

class TestTrackForUrlWorker : public QObject
{
    Q_OBJECT

    public Q_SLOTS:
        /**
         * Stores the track emitted with finishedLookup
         */
        void setEmittedTrack( Meta::TrackPtr track );

    private Q_SLOTS:
        void initTestCase();

        /**
         * Test slot completeJob() for both QUrl and QString types of urls
         */
        void testCompleteJobQUrl();
        void testCompleteJobQString();

        /**
         * Both use testCompleteJobInternal_data() to add test data to avoid redundancy
         */
        void testCompleteJobKUrl_data();
        void testCompleteJobQString_data();

    private:
         /**
         * This method does the main testing
         */
        void testCompleteJobInternal( MockTrackForUrlWorker *trackForUrlWorker );

        void testCompleteJobInternal_data();

        /**
         * For portability while specifying path for fetching tracks from disk
         */
        QString dataPath( const QString &relPath );

        Meta::TrackPtr m_emittedTrack;
};

#endif // TESTTRACKFORURLWORKER_H
