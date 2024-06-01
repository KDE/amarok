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

#include "TestMetaMultiTrack.h"

#include "core/support/Components.h"
#include "EngineController.h"
#include "config-amarok-test.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/multi/MultiTrack.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include <ThreadWeaver/Queue>

QTEST_GUILESS_MAIN( TestMetaMultiTrack )

TestMetaMultiTrack::TestMetaMultiTrack()
    : m_testMultiTrack( nullptr )
{
}

void
TestMetaMultiTrack::tracksLoaded( Playlists::PlaylistPtr playlist )
{
    Q_EMIT tracksLoadedSignal( playlist );
}

void TestMetaMultiTrack::initTestCase()
{
    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    /* Collection manager needs to be instantiated in the main thread, but
     * MetaProxy::Tracks used by playlist may trigger its creation in a different thread.
     * Pre-create it explicitly */
    CollectionManager::instance();

    const QString path = QString( AMAROK_TEST_DIR ) + "/data/playlists/test.pls";
    const QFileInfo file( QDir::toNativeSeparators( path ) );
    QVERIFY( file.exists() );
    const QString filePath = file.absoluteFilePath();
    m_playlist = Playlists::loadPlaylistFile( QUrl::fromLocalFile(filePath) ).data();
    QVERIFY( m_playlist ); // no playlist -> no test. that's life ;)
    subscribeTo( m_playlist );
    m_playlist->triggerTrackLoad();
    QSignalSpy spy( this, &TestMetaMultiTrack::tracksLoadedSignal );
    if( m_playlist->trackCount() < 0 )
        QVERIFY( spy.wait( 5000 ) );

    QCOMPARE( m_playlist->name(), QString("test.pls") );
    QCOMPARE( m_playlist->trackCount(), 4 );

    // now wait for all MetaProxy::Tracks to actually load their real tracks:
    Meta::TrackList tracks = m_playlist->tracks();
    NotifyObserversWaiter wainter( QSet<Meta::TrackPtr> ( tracks.begin(), tracks.end() ) );
    QSignalSpy spyDone( &wainter, &NotifyObserversWaiter::done );
    QVERIFY( spyDone.wait( 5000 ) );
}

void
TestMetaMultiTrack::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();
}

void TestMetaMultiTrack::init()
{
    m_testMultiTrack = new Meta::MultiTrack( m_playlist );
}

void TestMetaMultiTrack::cleanup()
{
    delete m_testMultiTrack;
}

void TestMetaMultiTrack::testSources()
{
    QStringList sources = m_testMultiTrack->sources();
    QCOMPARE( sources.size(), 4 );
    QCOMPARE( sources.at( 0 ), QString( "http://85.214.44.27:8000" ) );
    QCOMPARE( sources.at( 1 ), QString( "http://217.20.121.40:8000" ) );
    QCOMPARE( sources.at( 2 ), QString( "http://85.214.44.27:8100" ) );
    QCOMPARE( sources.at( 3 ), QString( "http://85.214.44.27:8200" ) );
}

void TestMetaMultiTrack::testSetSourceCurrentNextUrl()
{
    QCOMPARE( m_testMultiTrack->current(), 0 );
    QCOMPARE( m_testMultiTrack->playableUrl(), QUrl("http://85.214.44.27:8000") );
    QCOMPARE( m_testMultiTrack->nextUrl(), QUrl("http://217.20.121.40:8000") );

    m_testMultiTrack->setSource( 1 );
    QCOMPARE( m_testMultiTrack->current(), 1 );
    QCOMPARE( m_testMultiTrack->playableUrl(), QUrl("http://217.20.121.40:8000") );
    QCOMPARE( m_testMultiTrack->nextUrl(), QUrl("http://85.214.44.27:8100") );

    m_testMultiTrack->setSource( 2 );
    QCOMPARE( m_testMultiTrack->current(), 2 );
    QCOMPARE( m_testMultiTrack->playableUrl(), QUrl("http://85.214.44.27:8100") );
    QCOMPARE( m_testMultiTrack->nextUrl(), QUrl("http://85.214.44.27:8200") );

    m_testMultiTrack->setSource( 3 );
    QCOMPARE( m_testMultiTrack->current(), 3 );
    QCOMPARE( m_testMultiTrack->playableUrl(), QUrl("http://85.214.44.27:8200") );
    QCOMPARE( m_testMultiTrack->nextUrl(), QUrl() );

    m_testMultiTrack->setSource( 4 );
    QCOMPARE( m_testMultiTrack->current(), 3 );
    m_testMultiTrack->setSource( -1 );
    QCOMPARE( m_testMultiTrack->current(), 3 );
}

void TestMetaMultiTrack::testHasCapabilityInterface()
{
    QVERIFY( m_testMultiTrack->hasCapabilityInterface( Capabilities::Capability::MultiSource ) );
}

NotifyObserversWaiter::NotifyObserversWaiter( const QSet<Meta::TrackPtr> &tracks, QObject *parent )
    : QObject( parent )
    , m_tracks( tracks )
{
    // we need to filter already resolved tracks in the next event loop iteration because
    // the user wouldn't be able to get the done() signal yet.
    QTimer::singleShot( 0, this, &NotifyObserversWaiter::slotFilterResolved );
}

void
NotifyObserversWaiter::slotFilterResolved()
{
    QMutexLocker locker( &m_mutex );
    QMutableSetIterator<Meta::TrackPtr> it( m_tracks );
    while( it.hasNext() )
    {
        const Meta::TrackPtr &track = it.next();
        const MetaProxy::Track *proxyTrack = dynamic_cast<const MetaProxy::Track *>( track.data() );
        Q_ASSERT( proxyTrack );
        if( proxyTrack->isResolved() )
            it.remove();
        else
            subscribeTo( track );
    }
    if( m_tracks.isEmpty() )
        Q_EMIT done();
}

void
NotifyObserversWaiter::metadataChanged( const Meta::TrackPtr &track )
{
    QMutexLocker locker( &m_mutex );
    m_tracks.remove( track );
    if( m_tracks.isEmpty() )
        Q_EMIT done();
}
