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

#include "Debug.h"

#include "collection/QueryMaker.h"
#include "meta/Meta.h"
#include "MemoryCollection.h"
#include "MemoryQueryMaker.h"

#include "mocks/MetaMock.h"

#include <QVariantMap>
#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestMemoryQueryMaker )

//required for Debug.h
QMutex Debug::mutex;

TestMemoryQueryMaker::TestMemoryQueryMaker()
{
}

void
TestMemoryQueryMaker::testDeleteQueryMakerWhileQueryIsRunning()
{
    MemoryCollection mc;
    mc.addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc.addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));

    MemoryQueryMaker *qm = new MemoryQueryMaker( &mc, "test" );
    qm->setQueryType( QueryMaker::Track );

    qm->run();
    delete qm;
    //we cannot wait for a signal here....
    QTest::qWait( 500 );
}

void
TestMemoryQueryMaker::testDeleteCollectionWhileQueryIsRunning()
{
    MemoryCollection *mc = new MemoryCollection();
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));

    MemoryQueryMaker *qm = new MemoryQueryMaker( mc, "test" );
    qm->setQueryType( QueryMaker::Track );

    QSignalSpy spy( qm, SIGNAL(queryDone()));

    qm->run();
    delete mc;
    QTest::qWait( 500 );
    QCOMPARE( spy.count(), 1 );

    delete qm;
}
