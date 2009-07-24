/***************************************************************************
 *   Copyright (c) 2009 Max Howell <max@last.fm>                           *
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

#include "../src/SmartPointerList.h"
#include "TestSmartPointerList.h"

// use a macro, as we don't want to test copy ctor early
#define THREE_TIMERS( x ) SmartPointerList<QTimer> x; x << new QTimer << new QTimer << new QTimer

TestSmartPointerList::TestSmartPointerList( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "SmartPointerList.log" );
    QTest::qExec( this, testArgumentList );
}

void TestSmartPointerList::testCount()
{
    THREE_TIMERS( objects );
    QCOMPARE( objects.count(), 3 );
}
    
void TestSmartPointerList::testCopy()
{
    THREE_TIMERS( objects1 );
    SmartPointerList<QTimer> objects2 = objects1;

    for (int x = 0; x < 3; ++x)
        QVERIFY( objects1[x] == objects2[x] );

    QCOMPARE( objects1.count(), 3 );
    QCOMPARE( objects2.count(), 3 );
    delete objects1.last();
    QCOMPARE( objects1.count(), 2 );
    QCOMPARE( objects2.count(), 2 );
}
    
void TestSmartPointerList::testCopyAndThenDelete()
{
    THREE_TIMERS( os1 );
    SmartPointerList<QTimer>* os2 = new SmartPointerList<QTimer>( os1 );
    SmartPointerList<QTimer> os3( *os2 );

    delete os2;

    QCOMPARE( os1.count(), 3 );
    QCOMPARE( os3.count(), 3 );

    delete os1[1];

    QCOMPARE( os1.count(), 2 );
    QCOMPARE( os3.count(), 2 );
}

void TestSmartPointerList::testRemove()
{
    THREE_TIMERS( objects );
    delete objects.last();
    QCOMPARE( objects.count(), 2 );
}

void TestSmartPointerList::testRemoveAt()
{
    THREE_TIMERS( os );
    QTimer* t = os[1];
    os.removeAt( 1 );
    os << t;
    QCOMPARE( os.count(), 3 );
    delete t;
    QCOMPARE( os.count(), 2 );
}

void TestSmartPointerList::testMultipleOrgasms()
{
    THREE_TIMERS( os );
    for (int x = 0; x < 10; ++x)
        os << os.last();
    QCOMPARE( os.count(), 13 );
    delete os.last();
    QCOMPARE( os.count(), 2 );
}
    
void TestSmartPointerList::testForeach()
{
    THREE_TIMERS( objects );
    int x = 0;
    foreach (QTimer* o, objects) {
        (void) o;
        x++;
    }
    QCOMPARE( x, 3 );
}

void TestSmartPointerList::testOperatorPlus()
{
    THREE_TIMERS( os1 );
    SmartPointerList<QTimer> os2 = os1;

    QCOMPARE( (os1 + os2).count(), 6 );
    delete os1.last();
    QCOMPARE( (os1 + os2).count(), 4 );
}

void TestSmartPointerList::testOperatorPlusEquals()
{
    THREE_TIMERS( os );
    os += os;
    os += os;
    QCOMPARE( os.count(), 12 );
    QTimer* t = os.takeLast();
    QCOMPARE( os.count(), 11 );
    delete t;
    QCOMPARE( os.count(), 8 );
}

#include "../tests/TestSmartPointerList.moc"
