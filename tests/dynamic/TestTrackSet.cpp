/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "TestTrackSet.h"

#include "dynamic/TrackSet.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"


QTEST_GUILESS_MAIN( TestTrackSet )

TestTrackSet::TestTrackSet()
{
    QStringList testUids;
    testUids << QStringLiteral("uid-1") << QStringLiteral("uid-2") << QStringLiteral("uid-3") << QStringLiteral("uid-4") << QStringLiteral("uid-5") << QStringLiteral("uid-6");
    m_trackCollection = new Dynamic::TrackCollection( testUids );

    QCOMPARE( m_trackCollection->count(), 6 );
}

void
TestTrackSet::init()
{ }

void
TestTrackSet::cleanup()
{
    QCOMPARE( m_trackCollection->count(), 6 );
}

void
TestTrackSet::testOutstanding()
{
    Dynamic::TrackSet set;
    QCOMPARE( set.isOutstanding(), true );

    Dynamic::TrackSet set2( m_trackCollection, true );
    QCOMPARE( set2.isOutstanding(), false );
}

void
TestTrackSet::testEmptyFull()
{
    Dynamic::TrackSet set2( m_trackCollection, true );
    QCOMPARE( set2.isEmpty(), false );
    QCOMPARE( set2.isFull(), true );

    set2.reset( false );
    QCOMPARE( set2.isEmpty(), true );
    QCOMPARE( set2.isFull(), false );

    set2.reset( true );
    QCOMPARE( set2.isEmpty(), false );
    QCOMPARE( set2.isFull(), true );


    QCOMPARE( set2.contains(QStringLiteral("uid-1")), true );
    QStringList testUids;
    testUids << QStringLiteral("uid-1");
    set2.subtract( testUids );
    QCOMPARE( set2.isEmpty(), false );
    QCOMPARE( set2.isFull(), false );
    QCOMPARE( set2.contains(QStringLiteral("uid-1")), false );

    Dynamic::TrackSet set3( m_trackCollection, false );
    QCOMPARE( set3.isEmpty(), true );
    QCOMPARE( set3.isFull(), false );
}

void
TestTrackSet::testUnite()
{
    Dynamic::TrackSet set( m_trackCollection, false );

    // -- use string lists
    QStringList testUids;
    testUids << QStringLiteral("uid-1") << QStringLiteral("uid-2");
    set.unite( testUids );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );

    QStringList testUids2;
    testUids2 << QStringLiteral("uid-2") << QStringLiteral("uid-3");
    set.unite( testUids2 );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), true );

    // -- use another set
    QStringList testUids3;
    testUids3 << QStringLiteral("uid-3") << QStringLiteral("uid-4");
    Dynamic::TrackSet set2( m_trackCollection, false );
    set2.unite( testUids3 );

    set.unite( set2 );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-4")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-5")), false );
}

void
TestTrackSet::testIntersect()
{
    Dynamic::TrackSet set( m_trackCollection, true );

    // -- use string lists
    QStringList testUids;
    testUids << QStringLiteral("uid-1") << QStringLiteral("uid-2");
    set.intersect( testUids );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );

    QStringList testUids2;
    testUids2 << QStringLiteral("uid-2") << QStringLiteral("uid-3");
    set.intersect( testUids2 );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );

    // -- use another set
    QStringList testUids3;
    testUids3 << QStringLiteral("uid-2") << QStringLiteral("uid-3");
    Dynamic::TrackSet set2( m_trackCollection, false );
    set2.unite( testUids3 );

    set.intersect( set2 );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), true );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-4")), false );
}

void
TestTrackSet::testSubtract()
{
    Dynamic::TrackSet set( m_trackCollection, true );

    // -- use string lists
    QStringList testUids;
    testUids << QStringLiteral("uid-1") << QStringLiteral("uid-2");
    set.subtract( testUids );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), true );

    QStringList testUids2;
    testUids2 << QStringLiteral("uid-2") << QStringLiteral("uid-3");
    set.subtract( testUids2 );

    QCOMPARE( set.contains(QStringLiteral("uid-1")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-2")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-4")), true );

    // -- use another set
    QStringList testUids3;
    testUids3 << QStringLiteral("uid-3") << QStringLiteral("uid-4");
    Dynamic::TrackSet set2( m_trackCollection, false );
    set2.unite( testUids3 );

    set.subtract( set2 );
    QCOMPARE( set.contains(QStringLiteral("uid-3")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-4")), false );
    QCOMPARE( set.contains(QStringLiteral("uid-5")), true );

}

void
TestTrackSet::testEqual()
{

}

