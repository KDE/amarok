/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *   Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TestTrackLoader.h"

#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/support/TrackLoader.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"

#include <ThreadWeaver/Weaver>
#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestTrackLoader )

void
TestTrackLoader::initTestCase()
{
    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();
    CollectionManager::instance(); // create in the main thread
}

void
TestTrackLoader::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Weaver::instance()->finish();
}

void
TestTrackLoader::testInitAndInsertAtRow()
{
    The::playlistController()->clear(); // we need a clear playlist for those tests

    TrackLoader *loader1 = new TrackLoader();
    loader1->insertAtRow( 0 );
    QSignalSpy spy1( loader1, SIGNAL(finished(Meta::TrackList)) );
    loader1->init( QList<KUrl>() << KUrl( dataPath( "data/audio/album" ) ) );
    if( spy1.isEmpty() )
        QVERIFY2( QTest::kWaitForSignal( loader1, SIGNAL(finished(Meta::TrackList)), 5000 ),
                  "loader1 did not finish withing timeout" );
    QCOMPARE( spy1.first().first().value<Meta::TrackList>().count(), 3 );

    TrackLoader *loader2 = new TrackLoader();
    loader2->insertAtRow( 1 );
    QSignalSpy spy2( loader2, SIGNAL(finished(Meta::TrackList)) );
    loader2->init( QList<KUrl>() << KUrl( dataPath( "data/audio/album2" ) ) );
    if( spy2.isEmpty() )
        QVERIFY2( QTest::kWaitForSignal( loader2, SIGNAL(finished(Meta::TrackList)), 5000 ),
                  "loader2 did not finish withing timeout" );
    QCOMPARE( spy2.first().first().value<Meta::TrackList>().count(), 2 );

    QCOMPARE( Playlist::ModelStack::instance()->bottom()->rowCount(), 5 );

    QCOMPARE( Playlist::ModelStack::instance()->bottom()->trackAt( 0 )->prettyName(), QString( "Track01.ogg" ) );
    QCOMPARE( Playlist::ModelStack::instance()->bottom()->trackAt( 1 )->prettyName(), QString( "Track01.ogg" ) );
    QCOMPARE( Playlist::ModelStack::instance()->bottom()->trackAt( 2 )->prettyName(), QString( "Track02.ogg" ) );
    QCOMPARE( Playlist::ModelStack::instance()->bottom()->trackAt( 3 )->prettyName(), QString( "Track02.ogg" ) );
    QCOMPARE( Playlist::ModelStack::instance()->bottom()->trackAt( 4 )->prettyName(), QString( "Track03.ogg" ) );
}

QString
TestTrackLoader::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}
