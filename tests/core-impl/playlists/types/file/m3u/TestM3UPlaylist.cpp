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
#include "config-amarok-test.h"
#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QTest>

#include <threadweaver/ThreadWeaver.h>
#include <qtest_kde.h>


QTEST_KDEMAIN( TestM3UPlaylist, GUI )

TestM3UPlaylist::TestM3UPlaylist()
{}

QString
TestM3UPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestM3UPlaylist::initTestCase()
{
    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    const QUrl url = dataPath( "data/playlists/test.m3u" );
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    m_testPlaylist = new Playlists::M3UPlaylist( url );
    QVERIFY( m_testPlaylist );
    QVERIFY( m_testPlaylist->load( playlistStream ) );
    QCOMPARE( m_testPlaylist->tracks().size(), 10 );
    playlistFile1.close();
}

void TestM3UPlaylist::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Weaver::instance()->finish();

    delete m_testPlaylist;
}

void TestM3UPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist->prettyName(), QString( "test.m3u" ) );

    QCOMPARE( m_testPlaylist->name(), QString( "test.m3u" ) );

    m_testPlaylist->setName( "set name test" );
    QCOMPARE( m_testPlaylist->name(), QString( "set name test" ) );

    m_testPlaylist->setName( "set name test aäoöuüß" );
    QCOMPARE( m_testPlaylist->name(), QString( "set name test aäoöuüß" ) );

    m_testPlaylist->setName( "" );
    QCOMPARE( m_testPlaylist->name(), QString( "playlists" ) );
}

void TestM3UPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist->tracks();

    QCOMPARE( tracklist.size(), 10 );
    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "Platz 01" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "Platz 02" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "Platz 03" ) );
    QCOMPARE( tracklist.at( 9 ).data()->name(), QString( "Platz 10" ) );
}

void TestM3UPlaylist::testUidUrl()
{
    QCOMPARE( m_testPlaylist->uidUrl().pathOrUrl(), dataPath( "data/playlists/" ) );
}

void TestM3UPlaylist::testSetAndGetGroups()
{
    QStringList grouplist;
    QStringList newGrouplist;

    grouplist = m_testPlaylist->groups();
    QCOMPARE( grouplist.size(), 0 );

    newGrouplist.append( "test" );
    m_testPlaylist->setGroups( newGrouplist );
    grouplist = m_testPlaylist->groups();
    QCOMPARE( grouplist.size(), 1 );
    QCOMPARE( grouplist.at(0), QString( "test" ) );
}

void TestM3UPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist->isWritable() );
}

void TestM3UPlaylist::testSave()
{
    QTemporaryFile temp( QDir::tempPath() + "/XXXXXX.m3u" );
    QVERIFY( temp.open() );
    QVERIFY( m_testPlaylist->save( temp.fileName(), false ) );
    temp.close();
}
