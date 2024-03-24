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

#include "TestTrackForUrlWorker.h"

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "mocks/MockTrackForUrlWorker.h"

#include <KLocalizedString>

#include <QMetaType>
#include <QSignalSpy>
#include <QUrl>

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>

QTEST_GUILESS_MAIN( TestTrackForUrlWorker )

void
TestTrackForUrlWorker::initTestCase()
{
    KLocalizedString::setApplicationDomain("amarok");
    // To make queued signals/slots work with custom payload
    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );
    qRegisterMetaType<ThreadWeaver::Job*>( "ThreadWeaver::Job*" );
    AmarokConfig::instance("amarokrc");
}

QString
TestTrackForUrlWorker::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void
TestTrackForUrlWorker::testCompleteJobQUrl_data()
{
    testCompleteJobInternal_data();
}

void
TestTrackForUrlWorker::testCompleteJobQUrl()
{
    QUrl url;

    MockTrackForUrlWorker *trackForUrlWorker = new MockTrackForUrlWorker( url );
    QVERIFY( trackForUrlWorker );

    testCompleteJobInternal( trackForUrlWorker );
}

void TestTrackForUrlWorker::testCompleteJobQString_data()
{
    testCompleteJobInternal_data();
}

void
TestTrackForUrlWorker::testCompleteJobQString()
{
    QString url;

    MockTrackForUrlWorker *trackForUrlWorker = new MockTrackForUrlWorker( url );
    QVERIFY( trackForUrlWorker );

    testCompleteJobInternal( trackForUrlWorker );
}

void
TestTrackForUrlWorker::testCompleteJobInternal_data()
{
    QTest::addColumn<Meta::TrackPtr>( "track" );

    QTest::newRow( "track 1" ) << CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album/Track01.ogg" )) );
    QTest::newRow( "track 2" ) << CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album/Track02.ogg" )) );
    QTest::newRow( "track 3" ) << CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album/Track03.ogg" )) );
}

void
TestTrackForUrlWorker::testCompleteJobInternal( MockTrackForUrlWorker *trackForUrlWorker )
{
    // Connect finishedLookup with setEmittedTrack() that will store the emitted track
    connect( trackForUrlWorker, &MockTrackForUrlWorker::finishedLookup,
             this, &TestTrackForUrlWorker::setEmittedTrack );

    QSignalSpy spyFinishedLookup( trackForUrlWorker, &MockTrackForUrlWorker::finishedLookup );
    QSignalSpy spyDone( trackForUrlWorker, &MockTrackForUrlWorker::done );

    // Enqueue the job for execution and verify that it emits done when finished, which triggers completeJob
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>( trackForUrlWorker ) );
    bool receivedDone = spyDone.wait( 1000 );
    QVERIFY( receivedDone );

    // Verify that finishedLookup was emitted
    QCOMPARE( spyFinishedLookup.count(), 1 );

    // Verify that the track emitted with finishedLookup is indeed the track set by run()
    QFETCH( Meta::TrackPtr, track );
    QCOMPARE( m_emittedTrack, track );
}

void
TestTrackForUrlWorker::setEmittedTrack( Meta::TrackPtr track )
{
    m_emittedTrack = track;
}

