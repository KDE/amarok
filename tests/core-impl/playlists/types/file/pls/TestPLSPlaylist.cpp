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

#include "TestPLSPlaylist.h"

#include "core/support/Components.h"
#include "config-amarok-test.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"
#include "EngineController.h"

#include <QTest>
#include <QFile>
#include <QDir>

#include <KStandardDirs>
#include <qtest_kde.h>
#include <ThreadWeaver/Queue>

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
    // EngineController is used in a connection in MetaProxy::Track; avoid null sender
    // warning
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    /* Collection manager needs to be instantiated in the main thread, but
     * MetaProxy::Tracks used by playlist may trigger its creation in a different thread.
     * Pre-create it explicitly */
    CollectionManager::instance();

    const QString testPls = "data/playlists/test.pls";
    const QUrl url = dataPath( testPls );
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QString tempPath = KStandardDirs::locateLocal( "tmp", "test.pls" );
    QFile::remove( tempPath );
    QVERIFY( QFile::copy( url.toLocalFile(), tempPath ) );
    QVERIFY( QFile::exists( tempPath ) );

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    m_testPlaylist1 = new Playlists::PLSPlaylist( tempPath );
    QVERIFY( m_testPlaylist1 );
    QVERIFY( m_testPlaylist1->load( playlistStream ) );
    QCOMPARE( m_testPlaylist1->tracks().size(), 4 );
    playlistFile1.close();
}

void TestPLSPlaylist::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete m_testPlaylist1;
    delete Amarok::Components::setEngineController( 0 );
}

void TestPLSPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1->name(), QString( "test.pls" ) );

    m_testPlaylist1->setName( "set name test" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test.pls" ) );

    m_testPlaylist1->setName( "set name test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->name(), QString( "set name test aäoöuüß.pls" ) );

    m_testPlaylist1->setName( "test" );
    m_testPlaylist1->setName( "" );
    QCOMPARE( m_testPlaylist1->name(), QString( "test.pls" ) );
}

void TestPLSPlaylist::testPrettyName()
{
    QCOMPARE( m_testPlaylist1->prettyName(), QString( "test.pls" ) );
}

void TestPLSPlaylist::testTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
    QCOMPARE( tracklist.at( 3 ).data()->name(), QString( "::darkerradio:: - DIE Alternative im Netz ::www.darkerradio.de:: Tune In, Turn On, Burn Out!" ) );
}

void TestPLSPlaylist::testUidUrl()
{
    QString tempPath = KStandardDirs::locateLocal( "tmp", "test.pls" );
    m_testPlaylist1->setName( "test" );
    QCOMPARE( m_testPlaylist1->uidUrl().pathOrUrl(), tempPath );
}
void TestPLSPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1->isWritable() );
}

void TestPLSPlaylist::testSave()
{
    QVERIFY( m_testPlaylist1->save( false ) );
}
