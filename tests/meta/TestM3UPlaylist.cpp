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

#include "TestM3UPlaylist.h"

#include <KStandardDirs>

TestM3UPlaylist::TestM3UPlaylist( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "M3UPlaylist.xml" );
    QTest::qExec( this, testArgumentList );
}

void TestM3UPlaylist::initTestCase()
{
    QFile playlistFile1( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) );
    QTextStream playlistStream1;

    if( !playlistFile1.open( QFile::ReadOnly ) )
        QVERIFY( false );

    playlistStream1.setDevice( &playlistFile1 );

    m_testPlaylist1.load( playlistStream1 );
    playlistFile1.close();
}


void TestM3UPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1.name(), QString( "Playlist_1m3u" ) );

    m_testPlaylist1.setName( "test" );
    QCOMPARE( m_testPlaylist1.name(), QString( "test" ) );

    m_testPlaylist1.setName( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1.name(), QString( "test aäoöuüß" ) );

    m_testPlaylist1.setName( "" );
    QCOMPARE( m_testPlaylist1.name(), QString( "playlists" ) );
}

void TestM3UPlaylist::testPrettyName()
{
    QCOMPARE( m_testPlaylist1.prettyName(), QString( "playlists" ) );
}

void TestM3UPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1.tracks();

    QCOMPARE( tracklist.size(), 25 );
    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "Free Music Charts (One-Intro by darkermusic)" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "Reloj de arena" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "Get up!" ) );
    QCOMPARE( tracklist.at( 24 ).data()->name(), QString( "Raus" ) );
}

void TestM3UPlaylist::testRetrievableUrl()
{
    QCOMPARE( m_testPlaylist1.retrievableUrl().pathOrUrl(), KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) );
}

void TestM3UPlaylist::testSetAndGetGroups()
{
    QStringList grouplist;
    QStringList newGrouplist;

    grouplist = m_testPlaylist1.groups();
    QCOMPARE( grouplist.size(), 0 );

    newGrouplist.append( "test" );
    m_testPlaylist1.setGroups( newGrouplist );
    grouplist = m_testPlaylist1.groups();
    QCOMPARE( grouplist.size(), 1 );
    QCOMPARE( grouplist.at(0), QString( "test" ) );
}

void TestM3UPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1.isWritable() );
}

void TestM3UPlaylist::testSave()
{
    QFile::remove( QDir::tempPath() + QDir::separator() + "test.m3u" );
    QVERIFY( m_testPlaylist1.save( QDir::tempPath() + QDir::separator() + "test.m3u", false ) );
}
