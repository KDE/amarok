/****************************************************************************************
 * Copyright (c) 2012 Tatjana Gornak <t.gornak@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestPlaylistObserver.h"

#include "EngineController.h"
#include "config-amarok-test.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>
#include <QTimer>

#include <ThreadWeaver/Queue>

QTEST_GUILESS_MAIN( TestPlaylistObserver )

TestPlaylistObserver::TestPlaylistObserver()
    : m_observer( nullptr )
{
}

QString
TestPlaylistObserver::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QStringLiteral("/data/playlists/") + relPath );
}

void
TestPlaylistObserver::initTestCase()
{
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
    CollectionManager::instance();

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );
}

void
TestPlaylistObserver::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete Amarok::Components::setEngineController( nullptr );
}

void
TestPlaylistObserver::init()
{
    const QString testXspf = QStringLiteral("test.xspf");
    const QUrl url = QUrl::fromLocalFile(dataPath( testXspf ));

    m_testPlaylist = new Playlists::XSPFPlaylist( url );

    // test that behaviour before loading is correct
    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "test.xspf" ) );
    QCOMPARE( m_testPlaylist->trackCount(), -1 );

    m_observer = new Observer();
    m_observer->subscribeTo( m_testPlaylist );
}

void
TestPlaylistObserver::cleanup()
{
    delete m_observer;
    m_observer = nullptr;
    m_testPlaylist = nullptr;
}

void
TestPlaylistObserver::testMetadataChanged( )
{
    QSKIP( "Functionality this test tests has not yet been implemented", SkipAll );
    QSignalSpy spy( m_observer, &Observer::metadataChangedSignal );
    m_testPlaylist->triggerTrackLoad();
    QSignalSpy spyTracksLoaded(m_observer, &Observer::tracksLoadedSignal);
    QVERIFY( spyTracksLoaded.wait( 10000 ) );

    QVERIFY( spy.count() > 0 );
    // changed methadata means that we should get new name
    QCOMPARE( m_testPlaylist->name(), QStringLiteral( "my playlist" ) );
}

void
TestPlaylistObserver::testTracksLoaded()
{
    m_testPlaylist->triggerTrackLoad();
    QSignalSpy spyTracksLoaded(m_observer, &Observer::tracksLoadedSignal);
    QVERIFY( spyTracksLoaded.wait( 10000 ) );

    QCOMPARE( m_testPlaylist->trackCount(), 23 );
}

void
TestPlaylistObserver::testTrackAdded( )
{
    QSignalSpy spy( m_observer, &Observer::trackAddedSignal );
    m_testPlaylist->triggerTrackLoad();
    QSignalSpy spyTracksLoaded(m_observer, &Observer::tracksLoadedSignal);
    QVERIFY( spyTracksLoaded.wait( 10000 ) );
    QCOMPARE( spy.count(), 23 );
}

void
TestPlaylistObserver::testTrackRemoved()
{
    m_testPlaylist->triggerTrackLoad();
    QSignalSpy spyTracksLoaded( m_observer, &Observer::tracksLoadedSignal );
    QVERIFY( spyTracksLoaded.wait( 10000 ) );

    QString newName = QStringLiteral("test playlist written to.xspf");
    m_testPlaylist->setName( newName ); // don't overwrite original playlist
    QSignalSpy spyTrackRemoved( m_observer, &Observer::trackRemovedSignal );
    QCOMPARE( m_testPlaylist->trackCount(), 23 );
    m_testPlaylist->removeTrack( -1 ); // no effect
    m_testPlaylist->removeTrack( 0 ); // has effect
    m_testPlaylist->removeTrack( 22 ); // no effect, too far
    m_testPlaylist->removeTrack( 21 ); // has effect
    QCOMPARE( m_testPlaylist->trackCount(), 21 );
    QCOMPARE( spyTrackRemoved.count(), 2 );

    qDebug() << dataPath( newName );
    QVERIFY( QFile::remove( dataPath( newName ) ) );
}
