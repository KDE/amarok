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

#include "TestPLSPlaylist.h"

#include <KStandardDirs>

TestPLSPlaylist::TestPLSPlaylist( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "PLSPlaylist.log" );
    QTest::qExec( this, testArgumentList );
}

void TestPLSPlaylist::initTestCase()
{
    QFile playlistFile1( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.pls" ) );
    QTextStream playlistStream1;

    if( !playlistFile1.open(QFile::ReadOnly) )
        QVERIFY( false );

    playlistStream1.setDevice( &playlistFile1 );

    m_testPlaylist1.load( playlistStream1 );
    playlistFile1.close();
}


void TestPLSPlaylist::setAndGetname()
{
    /* fails as setName() seems to be unimplemented */
    QCOMPARE( m_testPlaylist1.name(), QString( "Playlist_1pls" ) );

    m_testPlaylist1.setName( "test" );
    QCOMPARE( m_testPlaylist1.name(), QString( "test" ) );

    m_testPlaylist1.setName( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1.name(), QString( "test aäoöuüß" ) );

    m_testPlaylist1.setName( "" );
    QCOMPARE( m_testPlaylist1.name(), QString( "" ) );
}

void TestPLSPlaylist::prettyName()
{
    QCOMPARE( m_testPlaylist1.prettyName(), QString( "Playlist_1pls" ) );
}

void TestPLSPlaylist::tracks()
{
    Meta::TrackList tracklist = m_testPlaylist1.tracks();

    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "Stream (http://85.214.44.27:8000)" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "Stream (http://217.20.121.40:8000)" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "Stream (http://85.214.44.27:8100)" ) );
    QCOMPARE( tracklist.at( 3 ).data()->name(), QString( "Stream (http://85.214.44.27:8200)" ) );
}

void TestPLSPlaylist::retrievableUrl()
{
    QCOMPARE( m_testPlaylist1.retrievableUrl().pathOrUrl(), KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.pls" ) );
}

void TestPLSPlaylist::isWritable()
{
    QVERIFY( !m_testPlaylist1.isWritable() );
}

void TestPLSPlaylist::save()
{
    QFile::remove( QDir::tempPath() + QDir::separator() + "test.pls" );
    QVERIFY( m_testPlaylist1.save( QDir::tempPath() + QDir::separator() + "test.pls", false ) );
}
