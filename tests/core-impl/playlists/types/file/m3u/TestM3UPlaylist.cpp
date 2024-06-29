/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
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
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <ThreadWeaver/ThreadWeaver>


QTEST_GUILESS_MAIN( TestM3UPlaylist )

TestM3UPlaylist::TestM3UPlaylist()
{
}

QString
TestM3UPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
}

void TestM3UPlaylist::initTestCase()
{
    m_tempDir = new QTemporaryDir;
    QVERIFY( m_tempDir->isValid() );

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    /* Collection manager needs to be instantiated in the main thread, but
     * MetaProxy::Tracks used by playlist may trigger its creation in a different thread.
     * Pre-create it explicitly */
    CollectionManager::instance();

    const QUrl url = QUrl::fromLocalFile(dataPath( QStringLiteral("data/playlists/test2.m3u") ));
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QString tempPath = m_tempDir->path() + QStringLiteral("/test.m3u");
    QVERIFY( QFile::copy( url.toLocalFile(), tempPath ) );
    QVERIFY( QFile::exists( tempPath ) );

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    m_testPlaylist = new Playlists::M3UPlaylist( QUrl::fromLocalFile(tempPath) );
    QVERIFY( m_testPlaylist );
    QVERIFY( m_testPlaylist->load( playlistStream ) );
    QCOMPARE( m_testPlaylist->tracks().size(), 11 );
    playlistFile1.close();
}

void TestM3UPlaylist::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete m_testPlaylist;
    delete m_tempDir;
}

void TestM3UPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist->prettyName(), QStringLiteral( "test.m3u" ) );

    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "test.m3u" ) );

    m_testPlaylist->setName( "set name test" );
    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "set name test.m3u" ) );

    m_testPlaylist->setName( "set name test aäoöuüß.m3u" );
    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "set name test aäoöuüß.m3u" ) );

    m_testPlaylist->setName( "test" );
    m_testPlaylist->setName( "" );
    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "test.m3u" ) );
}

void TestM3UPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist->tracks();

    QCOMPARE( tracklist.size(), 11 );
    QCOMPARE( tracklist.at( 0 )->name(), QStringLiteral( "Platz 01" ) );
    QCOMPARE( tracklist.at( 1 )->name(), QStringLiteral( "Platz 02" ) );
    QCOMPARE( tracklist.at( 2 )->name(), QStringLiteral( "Platz 03" ) );
    QCOMPARE( tracklist.at( 9 )->name(), QStringLiteral( "Platz 10" ) );
    QCOMPARE( tracklist.at( 10 )->name(), QStringLiteral( "Plätz 11" ) );
}

void TestM3UPlaylist::testUidUrl()
{
    QString tempPath = m_tempDir->path() + QStringLiteral("/test.m3u");
    //we have changed the name around so much, better reset it
    m_testPlaylist->setName( "test" );
    QCOMPARE( m_testPlaylist->uidUrl().toLocalFile(), tempPath );
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
    QCOMPARE( grouplist.at(0), QStringLiteral( "test" ) );
}

void TestM3UPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist->isWritable() );
}

void TestM3UPlaylist::testSave()
{
    QVERIFY( m_testPlaylist->save( false ) );
}

void TestM3UPlaylist::testSaveAndReload()
{
    QVERIFY( m_testPlaylist->save( false ) );
    Meta::TrackList tracklist = m_testPlaylist->tracks();
    const QString testTrack1Url = tracklist.at( 1 )->uidUrl();
    const QString testTrack2Url = tracklist.at( 10 )->uidUrl();

    const QUrl url = m_testPlaylist->uidUrl();
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    delete m_testPlaylist;
    m_testPlaylist = new Playlists::M3UPlaylist( url );
    QVERIFY( m_testPlaylist );
    QVERIFY( m_testPlaylist->load( playlistStream ) );
    playlistFile1.close();

    tracklist = m_testPlaylist->tracks();

    QCOMPARE( tracklist.size(), 11 );
    QCOMPARE( tracklist.at( 9 )->name(), QStringLiteral( "Platz 10" ) );
    QCOMPARE( tracklist.at( 10 )->name(), QStringLiteral( "Plätz 11" ) );
    QCOMPARE( tracklist.at( 1 )->uidUrl(), testTrack1Url );
    QCOMPARE( tracklist.at( 10 )->uidUrl(), testTrack2Url );
}
