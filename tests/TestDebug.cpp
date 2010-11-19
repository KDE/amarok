/***************************************************************************
 *   Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>            *
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

#define DEBUG_PREFIX "TestDebug"

#include "core/support/Debug.h"
#include "config-amarok-test.h"

#include <QtTest/QTest>

class TestDebug : public QObject
{
Q_OBJECT

public:
    TestDebug() {}

private slots:
    void benchDebugBlock();
    void benchDebugBlock_data();

private:
    void work();
    void work2();
};

void TestDebug::benchDebugBlock_data()
{
    QTest::addColumn<bool>("debugEnabled");
    QTest::addColumn<bool>("colorEnabled");

    QTest::newRow("debug with color")   << true  << true;
    QTest::newRow("debug nocolor")      << true  << false;
    QTest::newRow("nodebug with color") << false << true;
    QTest::newRow("nodebug nocolor")    << false << false;
}

void TestDebug::benchDebugBlock()
{
    QFETCH(bool, debugEnabled);
    QFETCH(bool, colorEnabled);

    Debug::setDebugEnabled( debugEnabled );
    Debug::setColoredDebug( colorEnabled );

    QVERIFY( Debug::debugEnabled() == debugEnabled );
    QVERIFY( Debug::debugColorEnabled() == colorEnabled );

    QBENCHMARK {
        work();
    }
}

void TestDebug::work()
{
    DEBUG_BLOCK
    debug() << "level 1";

    for( int i = 0; i < 100; ++i )
    {
        DEBUG_BLOCK
        debug() << "level 2";
        work2();
    }
}

void TestDebug::work2()
{
    DEBUG_BLOCK
    for( int j = 0; j < 100; ++j )
        debug() << "limbo";
}


QTEST_MAIN( TestDebug )

#include "TestDebug.moc"
