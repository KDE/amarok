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

#include "TestQueryMaker.h"

#include "mocks/MockQueryMaker.h"

#include <QSignalSpy>

using namespace Collections;


QTEST_MAIN( TestQueryMaker )

void
TestQueryMaker::initTestCase()
{
    m_mockQueryMaker = new MockQueryMaker();
    QVERIFY( m_mockQueryMaker );
}

void
TestQueryMaker::cleanupTestCase()
{
    if( !m_mockQueryMaker.isNull() )
        delete m_mockQueryMaker;
}

void
TestQueryMaker::testSetAutoDelete_data()
{
    QTest::addColumn<bool>( "autoDelete" );

    QTest::newRow( "false value" ) << false;
    QTest::newRow( "true value" ) << true;
}

void
TestQueryMaker::testSetAutoDelete()
{
    QFETCH( bool, autoDelete );

    QSignalSpy spyQueryDone( m_mockQueryMaker, &MockQueryMaker::queryDone );
    QSignalSpy spyDestroyed( m_mockQueryMaker, &MockQueryMaker::destroyed );

    m_mockQueryMaker->setAutoDelete( autoDelete );
    QVERIFY( m_mockQueryMaker );

    m_mockQueryMaker->emitQueryDone();

    // Ensure that queryDone() was indeed emitted
    QCOMPARE( spyQueryDone.count(), 1 );

    spyDestroyed.wait( 5000 );

    if( autoDelete )
    {
        // Signal queryDone() is connected to slot deleteLater()
        // and the destroyed() signal is emitted
        QCOMPARE( spyDestroyed.count(), 1 );
    }
    else
    {
        // Signal queryDone() is disconnected from slot deleteLater()
        // and no destroyed() signal is emitted
        QCOMPARE( spyDestroyed.count(), 0 );
    }
}
