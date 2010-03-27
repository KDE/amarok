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

#include "Components.h"
#include "config-amarok-test.h"
#include "core/playlists/impl/file/pls/PLSPlaylist.h"
#include "EngineController.h"

#include <QtTest/QTest>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestPLSPlaylist )

TestPLSPlaylist::TestPLSPlaylist()
{}

QString
TestPLSPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestPLSPlaylist::initTestCase()
{
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
  
    const KUrl url = dataPath( "data/playlists/test.pls" );
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream1;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream1.setDevice( &playlistFile1 );
    QVERIFY( playlistStream1.device() );

    m_testPlaylist1 = new Meta::PLSPlaylist( url );
    QVERIFY( m_testPlaylist1->load( playlistStream1 ) );
    playlistFile1.close();
}

void TestPLSPlaylist::cleanupTestCase()
{
    delete m_testPlaylist1;
}

void TestPLSPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1->name(), QString( "test.pls" ) );

    m_testPlaylist1->setName( "set name test" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test" ) );

    m_testPlaylist1->setName( "set name test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test aäoöuüß" ) );

    m_testPlaylist1->setName( "" );
    QCOMPARE( m_testPlaylist1->name(), QString( "playlists" ) );
}

void TestPLSPlaylist::testPrettyName()
{
    QCOMPARE( m_testPlaylist1->prettyName(), QString( "playlists" ) );
}

void TestPLSPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "Stream (http://85.214.44.27:8000)" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "Stream (http://217.20.121.40:8000)" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "Stream (http://85.214.44.27:8100)" ) );
    QCOMPARE( tracklist.at( 3 ).data()->name(), QString( "Stream (http://85.214.44.27:8200)" ) );
}

void TestPLSPlaylist::testRetrievableUrl()
{
    m_testPlaylist1->setName( "test.pls" );
    QCOMPARE( m_testPlaylist1->retrievableUrl().pathOrUrl(), dataPath( "data/playlists/test.pls" ) );
}

void TestPLSPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1->isWritable() );
}

void TestPLSPlaylist::testSave()
{
    QFile::remove( QDir::tempPath() + QDir::separator() + "test.pls" );
    QVERIFY( m_testPlaylist1->save( QDir::tempPath() + QDir::separator() + "test.pls", false ) );
}
