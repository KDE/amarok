/***************************************************************************
 *   Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                *
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

#include "TestASXPlaylist.h"

#include "config-amarok-test.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/asx/ASXPlaylist.h"
#include "EngineController.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

#include <ThreadWeaver/Queue>


QTEST_GUILESS_MAIN( TestASXPlaylist )

TestASXPlaylist::TestASXPlaylist()
{
}

QString
TestASXPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void
TestASXPlaylist::initTestCase()
{
    m_tempDir = new QTemporaryDir;
    QVERIFY( m_tempDir->isValid() );

    // EngineController is used in a connection in MetaProxy::Track; avoid null sender
    // warning
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    /* Collection manager needs to be instantiated in the main thread, but
     * MetaProxy::Tracks used by playlist may trigger its creation in a different thread.
     * Pre-create it explicitly */
    CollectionManager::instance();

    const QUrl url = QUrl::fromLocalFile(dataPath( "data/playlists/test2.asx" ));
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QString tempPath = m_tempDir->path() + "/test.asx";
    QFile::remove( tempPath );
    QVERIFY( QFile::copy( url.toLocalFile(), tempPath ) );
    QVERIFY( QFile::exists( tempPath ) );

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    m_testPlaylist = new Playlists::ASXPlaylist( QUrl::fromLocalFile(tempPath) );
    QVERIFY( m_testPlaylist );
    QVERIFY( m_testPlaylist->load( playlistStream ) );
    QCOMPARE( m_testPlaylist->tracks().size(), 2 );
    playlistFile1.close();
}

void
TestASXPlaylist::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete m_tempDir;
    delete m_testPlaylist;
    delete Amarok::Components::setEngineController( nullptr );
}

void
TestASXPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist->prettyName(), QString( "test.asx" ) );

    QCOMPARE( m_testPlaylist->name(), QString( "test.asx" ) );

    m_testPlaylist->setName( "set name test.asx" );
    QCOMPARE( m_testPlaylist->name(), QString( "set name test.asx" ) );

    m_testPlaylist->setName( "set name test aäoöuüß.asx" );
    QCOMPARE( m_testPlaylist->name(), QString( "set name test aäoöuüß.asx" ) );

    m_testPlaylist->setName( "test" );
    m_testPlaylist->setName( "" );
    QCOMPARE( m_testPlaylist->name(), QString( "test.asx" ) );
}

void
TestASXPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist->tracks();

    QCOMPARE( tracklist.size(), 2 );
    QCOMPARE( tracklist.at( 0 )->name(), QString( ":: Willkommen bei darkerradio - Tune in, turn on, burn out" ) );
}

void
TestASXPlaylist::testUidUrl()
{
    QString tempPath = m_tempDir->path() + "/test.asx";
    //we have changed the name around so much, better reset it
    m_testPlaylist->setName( "test" );
    QCOMPARE( m_testPlaylist->uidUrl().toLocalFile(), tempPath );
}

void
TestASXPlaylist::testSetAndGetGroups()
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

void
TestASXPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist->isWritable() );
}

void
TestASXPlaylist::testSave()
{
    QVERIFY( m_testPlaylist->save( false ) );
}

void
TestASXPlaylist::testSaveAndReload()
{
    QVERIFY( m_testPlaylist->save( false ) );
    Meta::TrackList tracklist = m_testPlaylist->tracks();
    const QString testTrack1Url = tracklist.at( 0 )->uidUrl();
    const QString testTrack2Url = tracklist.at( 1 )->uidUrl();

    const QUrl url = m_testPlaylist->uidUrl();
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    delete m_testPlaylist;
    m_testPlaylist = new Playlists::ASXPlaylist( url );
    QVERIFY( m_testPlaylist );
    QVERIFY( m_testPlaylist->load( playlistStream ) );
    playlistFile1.close();

    tracklist = m_testPlaylist->tracks();
    QCOMPARE( tracklist.size(), 2 );
    QCOMPARE( tracklist.at( 0 )->name(), QString( ":: Willkommen bei darkerradio - Tune in, turn on, burn out" ) );

    QCOMPARE( tracklist.at( 0 )->uidUrl(), testTrack1Url );
    QCOMPARE( tracklist.at( 1 )->uidUrl(), testTrack2Url );
}
