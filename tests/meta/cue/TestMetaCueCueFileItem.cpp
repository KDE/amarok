/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
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

#include "TestMetaCueCueFileItem.h"
#include "core/meta/impl/cue/Cue.h"

#include <QtTest/QTest>

MetaCue::CueFileItem testItem1( "", "", "", 0, 0 );
MetaCue::CueFileItem testItem2( "title", "artist", "album", 10, 10 );
MetaCue::CueFileItem testItem3( " title ", " artist ", " album ", 100, 100 );
MetaCue::CueFileItem testItem4( "ti tle", "ar tist", "al bum", 1000, 1000 );

TestMetaCueCueFileItem::TestMetaCueCueFileItem( const QStringList args, const QString &logPath )
    : TestBase( "MetaCueCueFileItem" )
{
    QStringList combinedArgs = args;
    addLogging( combinedArgs, logPath );
    QTest::qExec( this, combinedArgs );
}

void TestMetaCueCueFileItem::testSetAndGetLength()
{
    MetaCue::CueFileItem testItem;

    QCOMPARE( testItem.getLength(), (long)-1 );

    testItem.setLength( 0 );
    QCOMPARE( testItem.getLength(), (long)0 );

    testItem.setLength( 1 );
    QCOMPARE( testItem.getLength(), (long)1 );

    testItem.setLength( 5000 );
    QCOMPARE( testItem.getLength(), (long)5000 );
}

void TestMetaCueCueFileItem::testGetTitle()
{
    QCOMPARE( testItem1.getTitle(), QString( "" ) );
    QCOMPARE( testItem2.getTitle(), QString( "title" ) );
    QCOMPARE( testItem3.getTitle(), QString( " title " ) );
    QCOMPARE( testItem4.getTitle(), QString( "ti tle" ) );
}

void TestMetaCueCueFileItem::testGetArtist()
{
    QCOMPARE( testItem1.getArtist(), QString( "" ) );
    QCOMPARE( testItem2.getArtist(), QString( "artist" ) );
    QCOMPARE( testItem3.getArtist(), QString( " artist " ) );
    QCOMPARE( testItem4.getArtist(), QString( "ar tist" ) );
}

void TestMetaCueCueFileItem::testGetAlbum()
{
    QCOMPARE( testItem1.getAlbum(), QString( "" ) );
    QCOMPARE( testItem2.getAlbum(), QString( "album" ) );
    QCOMPARE( testItem3.getAlbum(), QString( " album " ) );
    QCOMPARE( testItem4.getAlbum(), QString( "al bum" ) );
}

void TestMetaCueCueFileItem::testGetTrackNumber()
{
    QCOMPARE( testItem1.getTrackNumber(), 0 );
    QCOMPARE( testItem2.getTrackNumber(), 10 );
    QCOMPARE( testItem3.getTrackNumber(), 100 );
    QCOMPARE( testItem4.getTrackNumber(), 1000 );
}

void TestMetaCueCueFileItem::testGetIndex()
{
    QCOMPARE( testItem1.getIndex(), (long)0 );
    QCOMPARE( testItem2.getIndex(), (long)10 );
    QCOMPARE( testItem3.getIndex(), (long)100 );
    QCOMPARE( testItem4.getIndex(), (long)1000 );
}
