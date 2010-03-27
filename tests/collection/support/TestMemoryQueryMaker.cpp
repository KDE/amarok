/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestMemoryQueryMaker.h"

#include "core/support/Debug.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "MemoryCollection.h"
#include "MemoryQueryMaker.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <QVariantMap>
#include <QSharedPointer>
#include <QSignalSpy>

#include <KCmdLineArgs>
#include <KGlobal>

#include <qtest_kde.h>

#include <gmock/gmock.h>

using ::testing::AnyNumber;
using ::testing::Return;

QTEST_KDEMAIN_CORE( TestMemoryQueryMaker )

TestMemoryQueryMaker::TestMemoryQueryMaker()
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
}

void
TestMemoryQueryMaker::testDeleteQueryMakerWhileQueryIsRunning()
{
    QSharedPointer<MemoryCollection> mc( new MemoryCollection() );
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    Meta::MockTrack *mock = new Meta::MockTrack();
    EXPECT_CALL( *mock, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( "track3" ) );
    Meta::TrackPtr trackPtr( mock );
    mc->addTrack( trackPtr );

    MemoryQueryMaker *qm = new MemoryQueryMaker( mc.toWeakRef(), "test" );
    qm->setQueryType( QueryMaker::Track );

    qm->run();
    delete qm;
    //we cannot wait for a signal here....
    //QTest::qWait( 500 );
}

void
TestMemoryQueryMaker::testDeleteCollectionWhileQueryIsRunning()
{
    QSharedPointer<MemoryCollection> mc( new MemoryCollection() );
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));

    MemoryQueryMaker *qm = new MemoryQueryMaker( mc, "test" );
    qm->setQueryType( QueryMaker::Track );

    QSignalSpy spy( qm, SIGNAL(queryDone()));

    qm->run();
    mc.clear();
    QTest::qWait( 500 );
    QCOMPARE( spy.count(), 1 );

    delete qm;
}
