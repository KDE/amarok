/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "TestSqlScanManager.h"

#include "amarokconfig.h"
#include "MetaTagLib.h"
#include "scanner/GenericScanManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/db/sql/SqlQueryMaker.h"
#include "core-impl/collections/db/sql/SqlRegistry.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"

#include "config-amarok-test.h"
#include "SqlMountPointManagerMock.h"

#include <QBuffer>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>

#include <KLocalizedString>
#include <ThreadWeaver/Queue>


QTEST_GUILESS_MAIN( TestSqlScanManager )

TestSqlScanManager::TestSqlScanManager()
    : QObject()
{
    QString help = i18n("Amarok"); // prevent a bug when the scanner is the first thread creating a translator
}

void
TestSqlScanManager::initTestCase()
{
    AmarokConfig::instance("amarokrc");
    m_autoGetCoverArt = AmarokConfig::autoGetCoverArt();
    AmarokConfig::setAutoGetCoverArt( false );

    // setenv( "LC_ALL", "", 1 ); // this breaks the test
    // Amarok does not force LC_ALL=C but obviously the test does it which
    // will prevent scanning of files with umlauts.

    //Tell GenericScanManager that we want to use the recently built scanner, not an installed version.
    const QString overridePath = QString( AMAROK_OVERRIDE_UTILITIES_PATH );
    qApp->setProperty( "overrideUtilitiesPath", overridePath );

    // that is the original mp3 file that we use to generate the "real" tracks
    m_sourcePath = QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + "/data/audio/Platz 01.mp3" );
    QVERIFY( QFile::exists( m_sourcePath ) );

    m_tmpDatabaseDir = new QTemporaryDir();
    QVERIFY( m_tmpDatabaseDir->isValid() );
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( m_tmpDatabaseDir->path() ) );

    m_collection = new Collections::SqlCollection( m_storage );
    connect( m_collection, &Collections::SqlCollection::updated, this, &TestSqlScanManager::slotCollectionUpdated );

    // TODO: change the mock mount point manager so that it doesn't pull
    //       in all the devices. Not much of a mock like this.
    SqlMountPointManagerMock *mock = new SqlMountPointManagerMock( this, m_storage );
    m_collection->setMountPointManager( mock );
    m_scanManager = m_collection->scanManager();

    AmarokConfig::setScanRecursively( true );
    AmarokConfig::setMonitorChanges( false );

    // switch on writing back so that we can create the test files with all the information
    AmarokConfig::setWriteBack( true );
    AmarokConfig::setWriteBackStatistics( true );
    AmarokConfig::setWriteBackCover( true );

    // I just need the table and not the whole playlist manager
    /*
    m_storage->query( QString( "CREATE TABLE playlist_tracks ("
            " id " + m_storage->idType() +
            ", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url " + m_storage->exactTextColumnType() +
            ", title " + m_storage->textColumnType() +
            ", album " + m_storage->textColumnType() +
            ", artist " + m_storage->textColumnType() +
            ", length INTEGER "
            ", uniqueid " + m_storage->textColumnType(128) + ") ENGINE = MyISAM;" ) );
            */
}

void
TestSqlScanManager::cleanupTestCase()
{
    // aborts a ThreadWeaver job that would otherwise cause next statement to stall
    delete m_collection;

    // we cannot simply call WeaverInterface::finish(), it stops event loop
//     QSignalSpy spy( ThreadWeaver::Queue::instance(), &ThreadWeaver::Queue::finished );
//     if( !ThreadWeaver::Queue::instance()->isIdle() )
//         QVERIFY2( spy.wait( 5000 ), "threads did not finish in timeout" );

    delete m_tmpDatabaseDir;

    AmarokConfig::setAutoGetCoverArt( m_autoGetCoverArt );
}

void
TestSqlScanManager::init()
{
    m_tmpCollectionDir = new QTemporaryDir();
    QVERIFY( m_tmpCollectionDir->isValid() );

    QStringList collectionFolders;
    collectionFolders << m_tmpCollectionDir->path();
    m_collection->mountPointManager()->setCollectionFolders( collectionFolders );
}

void
TestSqlScanManager::cleanup()
{
    m_scanManager->abort();

    m_storage->query( "BEGIN" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE urls;" );
    m_storage->query( "TRUNCATE TABLE statistics;" );
    m_storage->query( "TRUNCATE TABLE directories;" );
    m_storage->query( "COMMIT" );
    m_collection->registry()->emptyCache();

    delete m_tmpCollectionDir;
}

void
TestSqlScanManager::testScanSingle()
{
    m_collectionUpdatedCount = 0;
    createSingleTrack();
    fullScanAndWait();

    QVERIFY( m_collectionUpdatedCount > 0 );

    // -- check the commit
    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    QVERIFY( track );
    QCOMPARE( track->name(), QString("Theme From Armageddon") );
    QVERIFY( track->artist() );
    QCOMPARE( track->artist()->name(), QString("Soundtrack & Theme Orchestra") );
    QVERIFY( track->album() );
    QCOMPARE( track->album()->name(), QString("Big Screen Adventures") );
    QVERIFY( track->album()->albumArtist() );
    QCOMPARE( track->album()->albumArtist()->name(), QString("Theme Orchestra") );
    QVERIFY( !track->album()->isCompilation() ); // One single track is not compilation
    QCOMPARE( track->composer()->name(), QString("Unknown Composer") );
    QCOMPARE( track->comment(), QString("Amazon.com Song ID: 210541237") );
    QCOMPARE( track->year()->year(), 2009 );
    QCOMPARE( track->type(), QString("mp3") );
    QCOMPARE( track->trackNumber(), 28 );
    QCOMPARE( track->bitrate(), 257 ); // TagLib reports 257 kbit/s
    QCOMPARE( track->length(), qint64(12000) );
    QCOMPARE( track->sampleRate(), 44100 );
    QCOMPARE( track->filesize(), 389679 );
    QDateTime aDate = QDateTime::currentDateTime();
    QVERIFY( track->createDate().secsTo( aDate ) < 5 ); // I just imported the file
    QVERIFY( track->createDate().secsTo( aDate ) >= 0 );
    QVERIFY( track->modifyDate().secsTo( aDate ) < 5 ); // I just wrote the file
    QVERIFY( track->modifyDate().secsTo( aDate ) >= 0 );
    Meta::StatisticsPtr statistics = track->statistics();
    QVERIFY( qFuzzyCompare( statistics->score(), 0.875 ) );
    QCOMPARE( statistics->playCount(), 5 );
    QVERIFY( !statistics->firstPlayed().isValid() );
    QVERIFY( !statistics->lastPlayed().isValid() );
    QVERIFY( track->createDate().isValid() );

    // -- check that a further scan doesn't change anything
    m_collectionUpdatedCount = 0;

    fullScanAndWait();

    QCOMPARE( m_collectionUpdatedCount, 0 );
}

void
TestSqlScanManager::testScanDirectory()
{
    createAlbum();
    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->name(), QString("Thriller") );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );
    QVERIFY( !album->hasImage() );
}

void
TestSqlScanManager::testDuplicateUid()
{
    Meta::FieldHash values;

    // create two tracks with same uid
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96b") );
    values.insert( Meta::valUrl, QVariant("track1.mp3") );
    values.insert( Meta::valTitle, QVariant("Track 1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96b") );
    values.insert( Meta::valUrl, QVariant("track2.mp3") );
    values.insert( Meta::valTitle, QVariant("Track 2") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit (the database needs to have been updated correctly)
    m_collection->registry()->emptyCache();

    // -- both tracks should be present
    Meta::AlbumPtr album;
    album = m_collection->registry()->getAlbum( 1 );
    QVERIFY( album );
    QVERIFY( album->tracks().count() >= 1 );
}

void
TestSqlScanManager::testLongUid()
{
    Meta::FieldHash values;

    // create two tracks with different very long
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96bbbbbbbbbbbbbbc6c29f50279ab9523a0f44928bc1e96b1") );
    values.insert( Meta::valUrl, QVariant("track1.mp3") );
    values.insert( Meta::valTitle, QVariant("Track 1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96c6c29f50279ab9523a0f44928bc1e96bbbbbbbbbbbbbbc6c29f50279ab9523a0f44928bc1e96b2") );
    values.insert( Meta::valUrl, QVariant("track2.mp3") );
    values.insert( Meta::valTitle, QVariant("Track 2") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit (the database needs to have been updated correctly)
    m_collection->registry()->emptyCache();

    // both tracks should be present
    Meta::AlbumPtr album;
    album = m_collection->registry()->getAlbum( 1 );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 2 );
}


void
TestSqlScanManager::testCompilation()
{
    createAlbum();
    createCompilation();
    createCompilationLookAlikeAlbum();

    Meta::FieldHash values;

    // create one compilation track
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96b") );
    values.insert( Meta::valUrl, QVariant("Amazon MP3/The Sum Of All Fears (O.S.T.)/The Sum of All Fears/01 - If We Could Remember (O.S.T. LP Version).mp3") );
    values.insert( Meta::valTitle, QVariant("If We Could Remember (O.S.T. LP Version)") );
    values.insert( Meta::valArtist, QVariant("The Sum Of All Fears (O.S.T.)/Yolanda Adams") );
    values.insert( Meta::valAlbum, QVariant("The Sum of All Fears") );
    values.insert( Meta::valCompilation, QVariant(true) );
    createTrack( values );

    // create one various artists track
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6ae759476c34256ff1d06f0b5c964d75") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/06 - The Dream Of The Dolphin.mp3") );
    values.insert( Meta::valTitle, QVariant("The Dream Of The Dolphin") );
    values.insert( Meta::valArtist, QVariant("Various Artists") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valCompilation, QVariant(false) );
    createTrack( values );

    // create two tracks in the same directory with different albums
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("7957bc25521c1dc91351d497321c27a6") );
    values.insert( Meta::valUrl, QVariant("01 - Solid.mp3") );
    values.insert( Meta::valTitle, QVariant("Solid") );
    values.insert( Meta::valArtist, QVariant("Ashford & Simpson") );
    values.insert( Meta::valAlbum, QVariant("Solid") );
    createTrack( values );

    // create one none compilation track
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("b88c3405cfee64c50768b75eb6e3feea") );
    values.insert( Meta::valUrl, QVariant("In-Mood feat. Juliette - The Last Unicorn (Elemental Radio Mix).mp3") );
    values.insert( Meta::valTitle, QVariant("The Last Unicorn (Elemental Radio Mix)") );
    values.insert( Meta::valArtist, QVariant("In-Mood") );
    values.insert( Meta::valAlbum, QVariant("The Last Unicorn") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;

    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QCOMPARE( album->name(), QString("Top Gun") );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );

    album = m_collection->registry()->getAlbum( "The Sum of All Fears", QString() );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( album->isCompilation() );

    album = m_collection->registry()->getAlbum( "The Cross Of Changes", QString() );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( album->isCompilation() ); // the album is by various artists

    album = m_collection->registry()->getAlbum( "Solid", "Ashford & Simpson" );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "The Last Unicorn", "In-Mood" );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( !album->isCompilation() );

    // this album is a little tricky because it has some nasty special characters in it.
    Meta::TrackPtr track = m_collection->registry()->getTrackFromUid( m_collection->uidUrlProtocol() + "://" + "0969ea6128444e128cfcac95207bd525" );
    QVERIFY( track );
    album = track->album();
    QCOMPARE( album->tracks().count(), 13 );
    QVERIFY( !album->isCompilation() );
}

void
TestSqlScanManager::testBlock()
{
    /** TODO: do we need blocking at all?

    createSingleTrack();
    Meta::TrackPtr track;

    m_scanManager->blockScan(); // block the incremental scanning
    m_scanManager->requestFullScan();

    QTest::qWait( 100 );
    track = m_collection->registry()->getTrack( 1 );
    QVERIFY( !track );
    QVERIFY( !m_scanManager->isRunning() );

    m_scanManager->unblockScan(); // block the incremental scanning
    // now the actual behaviour is not defined.
    // it might or might not continue with the old scan

    waitScannerFinished(); // in case it does continue after all
    */
}

void
TestSqlScanManager::testAddDirectory()
{
    createAlbum();
    fullScanAndWait();

    createCompilation();
    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );
}

void
TestSqlScanManager::testRemoveDir()
{
    Meta::AlbumPtr album;

    createAlbum();
    createCompilation();
    fullScanAndWait();

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );

    // -- remove one album

    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    foreach( Meta::TrackPtr t, album->tracks() )
        QVERIFY( QFile::remove( t->playableUrl().path() ) );
    QVERIFY( QDir( m_tmpCollectionDir->path() ).rmdir( QFileInfo( album->tracks().first()->playableUrl().path() ).path() ) );

    fullScanAndWait();

    // this one is still here
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    // this one is gone
    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 0 );

    // -- remove the second album
    // this time it's a directory inside a directory
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    foreach( Meta::TrackPtr t, album->tracks() )
        QVERIFY( QFile::remove( t->playableUrl().path() ) );

    QVERIFY( QDir( m_tmpCollectionDir->path() ).rmdir( QFileInfo( album->tracks().first()->playableUrl().path() ).path() ) );

    incrementalScanAndWait();

    // this time both are gone
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 0 );
    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 0 );

}

void
TestSqlScanManager::testUidChangeMoveDirectoryIncrementalScan()
{
    createAlbum();
    fullScanAndWait();

    Meta::AlbumPtr album;
    Meta::TrackList tracks;

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    tracks = album->tracks();
    QCOMPARE( tracks.count(), 9 );
    QCOMPARE( tracks.first()->uidUrl(), QString("amarok-sqltrackuid://1dc7022c52a3e4c51b46577da9b3c8ff") );
    QVERIFY( !album->isCompilation() );

    // change all the track uids in a silly way
    QHash<int, QString> uidChanges; // uid hashed with track number
    foreach( const Meta::TrackPtr &track, tracks )
    {
        Meta::FieldHash uidChange;
        QString uid = track->uidUrl().remove( QString("amarok-sqltrackuid://") );
        QStringRef left = uid.leftRef( 10 );
        QStringRef right = uid.rightRef( uid.size() - left.size() );
        QString newUid = QString("%1%2").arg( right.toString(), left.toString() );
        uidChange.insert( Meta::valUniqueId, newUid );
        uidChanges.insert( track->trackNumber(), newUid );

        QUrl url = track->playableUrl();
        QVERIFY( url.isLocalFile() );
        Meta::Tag::writeTags( url.path(), uidChange, true );
    }

    // move album directory
    const QUrl oldUrl = tracks.first()->playableUrl();
    const QString base = m_tmpCollectionDir->path() + "/Pop";
    QVERIFY( QFile::rename( base, base + "Albums" ) );

    // do an incremental scan
    incrementalScanAndWait();

    // recheck album
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    tracks = album->tracks();
    QCOMPARE( tracks.count(), 9 );

    // check changed uids
    foreach( const Meta::TrackPtr &track, tracks )
    {
        QString uid = track->uidUrl().remove( QString("amarok-sqltrackuid://") );
        QCOMPARE( uid, uidChanges.value( track->trackNumber() ) );
    }
}

void
TestSqlScanManager::testRemoveTrack()
{
    Meta::AlbumPtr album;
    Meta::TrackPtr track;
    QDateTime aDate = QDateTime::currentDateTime();

    createAlbum();
    fullScanAndWait();

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );
    track = album->tracks().first(); // the tracks are sorted, so this is always the same track
    QCOMPARE( track->trackNumber(), 1 );
    QVERIFY( !track->statistics()->firstPlayed().isValid() );
    static_cast<Meta::SqlTrack*>(track.data())->setFirstPlayed( aDate );

    // -- remove one track
    QVERIFY( QFile::remove( track->playableUrl().path() ) );

    fullScanAndWait();

    // -- check that the track is really gone
    QCOMPARE( album->tracks().count(), 8 );
}

void
TestSqlScanManager::testMove()
{
    createAlbum();
    createCompilation();

    // we use the created and first played attributes for identifying the moved tracks.
    // currently those are not written back to the track

    Meta::AlbumPtr album;
    Meta::TrackPtr track;
    QDateTime aDate = QDateTime::currentDateTime();

    fullScanAndWait();
    if( qgetenv("AMAROK_RUN_LONG_TESTS").isNull() )
        QSKIP( "takes too long to be run by default;\nDefine AMAROK_RUN_LONG_TESTS "
        "environment variable to run all tests.", SkipAll );
    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );
    track = album->tracks().first();
    QCOMPARE( track->trackNumber(), 1 );
    QDateTime createDate = track->createDate();
    QDateTime modifyDate = track->modifyDate();

    // --- move one track
    static_cast<Meta::SqlTrack*>(track.data())->setFirstPlayed( aDate );
    const QString targetPath = m_tmpCollectionDir->path() + "/moved.mp3";
    QVERIFY( QFile::rename( track->playableUrl().path(), targetPath ) );

    fullScanAndWait();

    // -- check that the track is moved
    QVERIFY( createDate == track->createDate() ); // create date should not have changed
    QVERIFY( modifyDate == track->modifyDate() ); // we just changed the track. it should have changed
    QCOMPARE( track->statistics()->firstPlayed(), aDate );
    QCOMPARE( track->playableUrl().path(), targetPath );

    // --- move a directory
    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 10 );
    track = album->tracks().first();
    QUrl oldUrl = track->playableUrl();

    QVERIFY( QFile::rename( m_tmpCollectionDir->path() + "/Top Gun",
                            m_tmpCollectionDir->path() + "/Top Gun - Soundtrack" ) );

    // do an incremental scan
    incrementalScanAndWait();

    // check that the track is now moved (but still the old object)
    QCOMPARE( album->tracks().count(), 10 ); // no doublicate tracks
    QVERIFY( oldUrl != track->playableUrl() );
}

void
TestSqlScanManager::testFeat()
{
    Meta::FieldHash values;

    // create one compilation track
    values.clear();
    values.insert( Meta::valUniqueId, QVariant("b88c3405cfee64c50768b75eb6e3feea") );
    values.insert( Meta::valUrl, QVariant("In-Mood feat. Juliette - The Last Unicorn (Elemental Radio Mix).mp3") );
    values.insert( Meta::valTitle, QVariant("The Last Unicorn (Elemental Radio Mix)") );
    values.insert( Meta::valArtist, QVariant("In-Mood feat. Juliette") );
    values.insert( Meta::valAlbum, QVariant("The Last Unicorn") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;
    album = m_collection->registry()->getAlbum( "The Last Unicorn", "In-Mood" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 1 );
}

void
TestSqlScanManager::testAlbumImage()
{
    createSingleTrack();
    createAlbum();
    createCompilation();

    // put an image into the album directory
    QString imageSourcePath = QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + "/data/playlists/no-playlist.png" );
    QVERIFY( QFile::exists( imageSourcePath ) );
    QString targetPath;
    targetPath = m_tmpCollectionDir->path() + "/Pop/Thriller/cover.png";
    QVERIFY( QFile::copy( m_sourcePath, targetPath ) );

    // put an image into the compilation directory
    targetPath = m_tmpCollectionDir->path() + "/Top Gun/front.png";
    QVERIFY( QFile::copy( m_sourcePath, targetPath ) );

    // set an embedded image
    targetPath = m_tmpCollectionDir->path() + "/Various Artists/Big Screen Adventures/28 - Theme From Armageddon.mp3";
    Meta::Tag::setEmbeddedCover( targetPath, QImage( 200, 200, QImage::Format_RGB32 ) );

    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;

    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QVERIFY( album->hasImage() );

    album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QVERIFY( album->hasImage() );

    album = m_collection->registry()->getAlbum( "Big Screen Adventures", "Theme Orchestra" );
    QVERIFY( album );
    QVERIFY( album->hasImage() );
}

void
TestSqlScanManager::testMerges()
{
    // songs from same album but different directory
    // check that images are merged
    // check that old image is not overwritten

    Meta::FieldHash values;

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("123456d040d5dd9b5b45c1494d84cc82") );
    values.insert( Meta::valUrl, QVariant("Various Artists/Big Screen Adventures/28 - Theme From Armageddon.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Unnamed track") );
    values.insert( Meta::valArtist, QVariant("Unknown artist") );
    createTrack( values );

    // -- check the commit
    fullScanAndWait();

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    QVERIFY( track );
    QCOMPARE( track->name(), QString("Unnamed track") );

    // -- now overwrite the track with changed information and a new uid
    // - remove one track
    QVERIFY( QFile::remove( track->playableUrl().path() ) );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("794b1bd040d5dd9b5b45c1494d84cc82") );
    values.insert( Meta::valUrl, QVariant("Various Artists/Big Screen Adventures/28 - Theme From Armageddon.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Theme From Armageddon") );
    values.insert( Meta::valArtist, QVariant("Soundtrack & Theme Orchestra") );
    values.insert( Meta::valAlbum, QVariant("Big Screen Adventures") );
    values.insert( Meta::valComposer, QVariant("Unknown Composer") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 210541237") );
    values.insert( Meta::valGenre, QVariant("Broadway & Vocalists") );
    values.insert( Meta::valYear, QVariant(2009) );
    values.insert( Meta::valTrackNr, QVariant(28) );
    values.insert( Meta::valScore, QVariant(0.875) );
    values.insert( Meta::valPlaycount, QVariant(5) );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit
    QCOMPARE( track->name(), QString("Theme From Armageddon") );
    QVERIFY( track->artist() );
    QCOMPARE( track->artist()->name(), QString("Soundtrack & Theme Orchestra") );
    QVERIFY( track->album() );
    QCOMPARE( track->album()->name(), QString("Big Screen Adventures") );if( qgetenv("AMAROK_RUN_LONG_TESTS").isNull() )
    QSKIP( "takes too long to be run by default;\nDefine AMAROK_RUN_LONG_TESTS "
    "environment variable to run all tests.", SkipAll );
    QCOMPARE( track->composer()->name(), QString("Unknown Composer") );
    QCOMPARE( track->comment(), QString("Amazon.com Song ID: 210541237") );
    QCOMPARE( track->year()->year(), 2009 );
    QCOMPARE( track->type(), QString("mp3") );
    QCOMPARE( track->trackNumber(), 28 );
    QCOMPARE( track->bitrate(), 257 );
    QCOMPARE( track->length(), qint64(12000) );
    QCOMPARE( track->sampleRate(), 44100 );
    QCOMPARE( track->filesize(), 389679 );
    Meta::StatisticsPtr statistics = track->statistics();
    QVERIFY( qFuzzyCompare( statistics->score(), 0.875 ) );
    QCOMPARE( statistics->playCount(), 5 );
    QVERIFY( !statistics->firstPlayed().isValid() );
    QVERIFY( !statistics->lastPlayed().isValid() );
    QVERIFY( track->createDate().isValid() );


    // -- now do an incremental scan
    createAlbum(); // add a new album
    incrementalScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;

    // the old track is still there
    album = m_collection->registry()->getAlbum( "Big Screen Adventures", "Soundtrack & Theme Orchestra" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 1 );

    // the new album is now here
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );
}

void
TestSqlScanManager::testLargeInsert()
{
    if( qgetenv("AMAROK_RUN_LONG_TESTS").isNull() )
        QSKIP( "takes too long to be run by default;\nDefine AMAROK_RUN_LONG_TESTS "
               "environment variable to run all tests.", SkipAll );
    // the old large insert test was misleading as the problems with
    // the insertion started upwards of 20000 tracks.
    //
    // For now here are the "ok" numbers on a sensible fast computer:
    // Scanning   10000 files <3 min
    // Committing 10000 files <30 sec
    // Scanning   50000 files <13 min
    // Committing 50000 files <1 min

    QDateTime aDate = QDateTime::currentDateTime();

    // -- create the input data
    QByteArray byteArray;
    QBuffer *buffer = new QBuffer(&byteArray);
    buffer->open(QIODevice::ReadWrite);

    QXmlStreamWriter writer( buffer );

    writer.writeStartElement( "scanner" );

    int trackCount = 0;

    // some simulated normal albums
    for( int dirId = 0; dirId < 2000; dirId++ )
    {
        writer.writeStartElement( "directory" );
        writer.writeTextElement( "path", QString::number(dirId) );
        writer.writeTextElement( "rpath", '/' + QString::number(dirId) );
        writer.writeTextElement( "mtime", QString::number(aDate.toSecsSinceEpoch()) );

        for( int trackId = 0; trackId < 20; trackId++ )
        {
            writer.writeStartElement( "track" );
            writer.writeTextElement( "uniqueid", "uid" + QString::number(trackCount) );
            writer.writeTextElement( "path", "/path" + QString::number(trackCount) );
            writer.writeTextElement( "rpath", "path" + QString::number(trackCount) );
            trackCount++;
            writer.writeTextElement( "title", "track" + QString::number(trackCount) );
            writer.writeTextElement( "artist", "artist" + QString::number(dirId) );
            writer.writeTextElement( "album", QString::number(dirId) );
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    // a simulated genre folders
    for( int dirId = 0; dirId < 7; dirId++ )
    {
        writer.writeStartElement( "directory" );
        writer.writeTextElement( "path", "genre" + QString::number(dirId) );
        writer.writeTextElement( "rpath", "/genre" + QString::number(dirId) );
        writer.writeTextElement( "mtime", QString::number(aDate.toSecsSinceEpoch()) );

        for( int albumId = 0; albumId < 1000; albumId++ )
        {
            writer.writeStartElement( "track" );
            writer.writeTextElement( "uniqueid", "uid" + QString::number(trackCount) );
            writer.writeTextElement( "path", "/path" + QString::number(trackCount) );
            writer.writeTextElement( "rpath", "path" + QString::number(trackCount) );
            trackCount++;
            writer.writeTextElement( "title", "track" + QString::number(trackCount) );
            writer.writeTextElement( "artist",
                                      "artist" + QString::number(dirId) +
                                      "xx" + QString::number(albumId) );
            writer.writeTextElement( "album",
                                      "genre album" + QString::number(dirId) +
                                      "xx" + QString::number(albumId) );
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    // A simulated amarok 1.4 collection folder
    for( int dirId = 0; dirId < 3000; dirId++ )
    {
        writer.writeStartElement( "directory" );
        writer.writeTextElement( "path", "collection" + QString::number(dirId) );
        writer.writeTextElement( "rpath", "/collection" + QString::number(dirId) );
        writer.writeTextElement( "mtime", QString::number(aDate.toSecsSinceEpoch()) );

        writer.writeStartElement( "track" );
        writer.writeTextElement( "uniqueid", "uid" + QString::number(trackCount) );
        writer.writeTextElement( "path", "/path" + QString::number(trackCount) );
        writer.writeTextElement( "rpath", "path" + QString::number(trackCount) );
        trackCount++;
        writer.writeTextElement( "title", "track" + QString::number(trackCount) );
        writer.writeTextElement( "artist", "album artist" + QString::number(dirId % 200) );
        writer.writeTextElement( "album", "album" + QString::number(dirId % 300) );
        writer.writeEndElement();

        writer.writeEndElement();
    }

    writer.writeEndElement();

    aDate = QDateTime::currentDateTime();
    // -- feed the scanner in batch mode
    buffer->seek( 0 );
    importAndWait( buffer );

    qDebug() << "performance test secs:"<< aDate.secsTo( QDateTime::currentDateTime() );

    QVERIFY( aDate.secsTo( QDateTime::currentDateTime() ) < 120 );

    // -- get all tracks
    Collections::SqlQueryMaker *qm = static_cast< Collections::SqlQueryMaker* >( m_collection->queryMaker() );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->setBlocking( true );
    qm->run();
    Meta::TrackList tracks = qm->tracks();
    delete qm;

    for( int i = 0; i < trackCount; i++ )
    {
        Meta::TrackPtr track = m_collection->registry()->getTrackFromUid( m_collection->uidUrlProtocol() + "://uid" + QString::number(i) );
        QVERIFY( track );
    }

    qDebug() << "performance test secs:"<< aDate.secsTo( QDateTime::currentDateTime() ) << "tracks:" << trackCount;
    QCOMPARE( tracks.count(), trackCount );

    // -- scan the input a second time. that should be a lot faster (but currently isn't)
    aDate = QDateTime::currentDateTime();
    // -- feed the scanner in batch mode
    buffer = new QBuffer(&byteArray); // the old scanner deleted the old buffer.
    buffer->open(QIODevice::ReadWrite);
    importAndWait( buffer );

    qDebug() << "performance test secs:"<< aDate.secsTo( QDateTime::currentDateTime() );

    QVERIFY( aDate.secsTo( QDateTime::currentDateTime() ) < 80 );
}

void
TestSqlScanManager::testIdentifyCompilationInMultipleDirectories()
{
    // Compilations where each is track is from a different artist
    // are often stored as one track per directory, e.g.
    // /artistA/compilation/track1
    // /artistB/compilation/track2
    //
    // this is how Amarok 1 (after using Organize Collection) and iTunes are storing
    // these albums on disc
    // the bad thing is that Amarok 1 (as far as I know) didn't set the id3 tags

    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("5ef9fede5b3f98deb088b33428b0398e") );
    values.insert( Meta::valUrl, QVariant("Kenny Loggins/Top Gun/Top Gun - 01 - Kenny Loggins - Danger Zone.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Danger Zone") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    values.insert( Meta::valTrackNr, QVariant("1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("3e3970f52b0eda3f2a8c1b3a8c8d39ea") );
    values.insert( Meta::valUrl, QVariant("Cheap Trick/Top Gun/Top Gun - 02 - Cheap Trick - Mighty Wings.mp3") );
    values.insert( Meta::valTitle, QVariant("Mighty Wings") );
    values.insert( Meta::valArtist, QVariant("Cheap Trick") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6ea0bbd97ad8068df58ad75a81f271f7") );
    values.insert( Meta::valUrl, QVariant("Kenny Loggins/Top Gun/Top Gun - 03 - Kenny Loggins - Playing With The Boys.mp3") );
    values.insert( Meta::valTitle, QVariant("Playing With The Boys") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("f3ac2e15288361d779a0ae813a2018ba") );
    values.insert( Meta::valUrl, QVariant("Teena Marie/Top Gun/Top Gun - 04 - Teena Marie - Lead Me On.mp3") );
    values.insert( Meta::valTitle, QVariant("Lead Me On") );
    values.insert( Meta::valArtist, QVariant("Teena Marie") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( "Top Gun", QString() );
    QVERIFY( album );
    QCOMPARE( album->name(), QString("Top Gun") );
    QCOMPARE( album->tracks().count(), 4 );
    QVERIFY( album->isCompilation() );
}

void
TestSqlScanManager::testAlbumArtistMerges()
{
    // three tracks with the same artist but different album artist.
    // (one is unset)
    // Those should end up in different albums.

    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("1ef9fede5b3f98deb088b33428b0398e") );
    values.insert( Meta::valUrl, QVariant("test1/song1.mp3") );
    values.insert( Meta::valTitle, QVariant("title1") );
    values.insert( Meta::valArtist, QVariant("artist") );
    values.insert( Meta::valAlbumArtist, QVariant("albumArtist1") );
    values.insert( Meta::valAlbum, QVariant("test1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("2ef9fede5b3f98deb088b33428b0398b") );
    values.insert( Meta::valUrl, QVariant("test1/song2.mp3") );
    values.insert( Meta::valTitle, QVariant("title2") );
    values.insert( Meta::valArtist, QVariant("artist") );
    values.insert( Meta::valAlbumArtist, QVariant("albumArtist2") );
    values.insert( Meta::valAlbum, QVariant("test1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("3ef9fede5b3f98deb088b33428b0398c") );
    values.insert( Meta::valUrl, QVariant("test1/song3.mp3") );
    values.insert( Meta::valTitle, QVariant("title3") );
    values.insert( Meta::valArtist, QVariant("artist") );
    values.insert( Meta::valAlbum, QVariant("test1") );
    createTrack( values );

    fullScanAndWait();

    // -- check the commit
    Meta::AlbumPtr album;

    album = m_collection->registry()->getAlbum( "test1", QString() );
    QVERIFY( album );
    QCOMPARE( album->name(), QString("test1") );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( album->isCompilation() );

    album = m_collection->registry()->getAlbum( "test1", QString("albumArtist1") );
    QVERIFY( album );
    QCOMPARE( album->name(), QString("test1") );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "test1", QString("albumArtist2") );
    QVERIFY( album );
    QCOMPARE( album->name(), QString("test1") );
    QCOMPARE( album->tracks().count(), 1 );
    QVERIFY( !album->isCompilation() );
}

void
TestSqlScanManager::testCrossRenaming()
{
    createAlbum();

    // we use the created and first played attributes for identifying the moved tracks.
    // currently those are not written back to the track

    Meta::AlbumPtr album;
    Meta::TrackPtr track;

    fullScanAndWait();

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    // --- cross-rename two track
    track = album->tracks().at( 0 );
    static_cast<Meta::SqlTrack*>(track.data())->setRating( 1 );
    QString path1 = track->playableUrl().path();

    track = album->tracks().at( 1 );
    static_cast<Meta::SqlTrack*>(track.data())->setRating( 2 );
    QString path2 = track->playableUrl().path();

    QString targetPath = m_tmpCollectionDir->path() + "moved.mp3";
    QVERIFY( QFile::rename( path2, targetPath ) );
    QVERIFY( QFile::rename( path1, path2 ) );
    QVERIFY( QFile::rename( targetPath, path1 ) );

    fullScanAndWait();

    // -- check that the tracks are moved correctly
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QVERIFY( album );
    QCOMPARE( album->tracks().count(), 9 );

    track = album->tracks().at( 0 );
    QCOMPARE( track->statistics()->rating(), 1 );
    QCOMPARE( track->playableUrl().path(), path2 );

    track = album->tracks().at( 1 );
    QCOMPARE( track->statistics()->rating(), 2 );
    QCOMPARE( track->playableUrl().path(), path1 );
}


void
TestSqlScanManager::slotCollectionUpdated()
{
    m_collectionUpdatedCount++;
}

void
TestSqlScanManager::fullScanAndWait()
{
    QScopedPointer<Capabilities::CollectionScanCapability> csc( m_collection->create<Capabilities::CollectionScanCapability>());
    if( csc )
    {
        csc->startFullScan();
        waitScannerFinished();
    }
}

void
TestSqlScanManager::incrementalScanAndWait()
{
    // incremental scans use the modification time of the file system.
    // this time is only in seconds, so to be sure that the incremental scan
    // works we need to wait at least one second.
    QTest::qWait( 1000 );

    QScopedPointer<Capabilities::CollectionScanCapability> csc( m_collection->create<Capabilities::CollectionScanCapability>());
    if( csc )
        csc->startIncrementalScan();

    waitScannerFinished();
}

void
TestSqlScanManager::importAndWait( QIODevice* input )
{
    QScopedPointer<Capabilities::CollectionImportCapability> csc( m_collection->create<Capabilities::CollectionImportCapability>());
    if( csc )
        csc->import( input, 0 );

    waitScannerFinished();
}

void
TestSqlScanManager::waitScannerFinished()
{
    QVERIFY( m_scanManager->isRunning() );
    QSignalSpy succeedSpy( m_scanManager, &GenericScanManager::succeeded );
    QSignalSpy failSpy( m_scanManager, &GenericScanManager::failed );
    QSignalSpy resultSpy( this, &TestSqlScanManager::scanManagerResult );

    // connect the result signal *after* the spies to ensure they are updated first
    connect( m_scanManager, &GenericScanManager::succeeded, this, &TestSqlScanManager::scanManagerResult );
    connect( m_scanManager, &GenericScanManager::failed, this, &TestSqlScanManager::scanManagerResult);
    const bool ok = resultSpy.wait( 5000 );
    disconnect( m_scanManager, &GenericScanManager::succeeded, this, &TestSqlScanManager::scanManagerResult );
    disconnect( m_scanManager, &GenericScanManager::failed, this, &TestSqlScanManager::scanManagerResult );
    QVERIFY2( ok, "Scan Manager timed out without a result" );

    if( failSpy.count() > 0 )
    {
        QStringList errors;
        foreach( const QList<QVariant> &arguments, static_cast<QList<QList<QVariant> > >( failSpy ) )
            errors << arguments.value( 0 ).toString();
        // this will fire each time:
        qWarning() << "ScanManager failed with an error:" << errors.join( ", " );
    }
    QCOMPARE( qMakePair( succeedSpy.count(), failSpy.count() ), qMakePair( 1, 0 ) );

    QVERIFY( !m_scanManager->isRunning() );
}

void
TestSqlScanManager::createTrack( const Meta::FieldHash &values )
{
    // -- copy the file from our original
    QVERIFY( values.contains( Meta::valUrl ) );
    const QString targetPath = m_tmpCollectionDir->path() + '/' + values.value( Meta::valUrl ).toString();
    QVERIFY( QDir( m_tmpCollectionDir->path() ).mkpath( QFileInfo( values.value( Meta::valUrl ).toString() ).path() ) );
    QVERIFY( QFile::copy( m_sourcePath, targetPath ) );

    // -- set all the values that we need
    Meta::Tag::writeTags( targetPath, values, true );
}

void
TestSqlScanManager::createSingleTrack()
{
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("794b1bd040d5dd9b5b45c1494d84cc82") );
    values.insert( Meta::valUrl, QVariant("Various Artists/Big Screen Adventures/28 - Theme From Armageddon.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Theme From Armageddon") );
    values.insert( Meta::valArtist, QVariant("Soundtrack & Theme Orchestra") );
    values.insert( Meta::valAlbumArtist, QVariant("Theme Orchestra") );
    values.insert( Meta::valAlbum, QVariant("Big Screen Adventures") );
    values.insert( Meta::valComposer, QVariant("Unknown Composer") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 210541237") );
    values.insert( Meta::valGenre, QVariant("Broadway & Vocalists") );
    values.insert( Meta::valYear, QVariant(2009) );
    values.insert( Meta::valTrackNr, QVariant(28) );
    // values.insert( Meta::valBitrate, QVariant(216) ); // the bitrate can not be set. it's computed
    // values.insert( Meta::valLength, QVariant(184000) ); // also can't be set
    // values.insert( Meta::valSamplerate, QVariant(44100) ); // again
    // values.insert( Meta::valFilesize, QVariant(5094892) ); // again
    values.insert( Meta::valScore, QVariant(0.875) );
    values.insert( Meta::valPlaycount, QVariant(5) );
    // TODO: set an embedded cover

    createTrack( values );
}

void
TestSqlScanManager::createAlbum()
{
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("1dc7022c52a3e4c51b46577da9b3c8ff") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 01 - Michael Jackson - Track01.mp3") );
    values.insert( Meta::valTitle, QVariant("Wanna Be Startin' Somethin'") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("1dc708934a3e4c51b46577da9b3ab11") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 02 - Michael Jackson - Track02.mp3") );
    values.insert( Meta::valTitle, QVariant("Baby Be Mine") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(2) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("15a6b1bf79747fdc8e9c6b6f06203017") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 03 - Michael Jackson - Track03.mp3") );
    values.insert( Meta::valTitle, QVariant("The Girl Is Mine") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(3) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("4aba4c8b1d1893c03c112cc3c01221e9") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 04 - Michael Jackson - Track04.mp3") );
    values.insert( Meta::valTitle, QVariant("Thriller") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(4) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("cb44d2a3d8053829b04672723bf0bd6e") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 05 - Michael Jackson - Track05.mp3") );
    values.insert( Meta::valTitle, QVariant("Beat It") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(5) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("eba1858eeeb3c6d97fe3385200114d86") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 06 - Michael Jackson - Track06.mp3") );
    values.insert( Meta::valTitle, QVariant("Billy Jean") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(6) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("4623850290998486b0f7b39a2719904e") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 07 - Michael Jackson - Track07.mp3") );
    values.insert( Meta::valTitle, QVariant("Human Nature") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(7) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6d9a7de13af1e16bb13a6208e44b046d") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 08 - Michael Jackson - Track08.mp3") );
    values.insert( Meta::valTitle, QVariant("P.Y.T. (Pretty Young Thing)") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(8) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("91cf9a7c0d255399f9f6babfacae432b") );
    values.insert( Meta::valUrl, QVariant("Pop/Thriller/Thriller - 09 - Michael Jackson - Track09.mp3") );
    values.insert( Meta::valTitle, QVariant("The Lady In My Life") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(9) );
    createTrack( values );
}

void
TestSqlScanManager::createCompilation()
{
    // a compilation without the compilation flags values.insert( Meta::valCompilation, QVariant(true) );
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("5ef9fede5b3f98deb088b33428b0398e") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 01 - Kenny Loggins - Danger Zone.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Danger Zone") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("3e3970f52b0eda3f2a8c1b3a8c8d39ea") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 02 - Cheap Trick - Mighty Wings.mp3") );
    values.insert( Meta::valTitle, QVariant("Mighty Wings") );
    values.insert( Meta::valArtist, QVariant("Cheap Trick") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6ea0bbd97ad8068df58ad75a81f271f7") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 03 - Kenny Loggins - Playing With The Boys.mp3") );
    values.insert( Meta::valTitle, QVariant("Playing With The Boys") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("f3ac2e15288361d779a0ae813a2018ba") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 04 - Teena Marie - Lead Me On.mp3") );
    values.insert( Meta::valTitle, QVariant("Lead Me On") );
    values.insert( Meta::valArtist, QVariant("Teena Marie") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("ffe2bb3e6e2f698983c95e40937545ff") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 05 - Berlin - Take My Breath Away (Love Theme From _Top Gun_).mp3") );
    values.insert( Meta::valTitle, QVariant("Take My Breath Away (Love Theme From &quot;Top Gun&quot;)") );
    values.insert( Meta::valArtist, QVariant("Berlin") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c871dba16f92483898bcd6a1ed1bc14f") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 06 - Miami Sound Machine - Hot Summer Nights.mp3") );
    values.insert( Meta::valTitle, QVariant("Hot Summer Nights") );
    values.insert( Meta::valArtist, QVariant("Miami Sound Machine") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("80d157c36ed334192ed8df4c01bf0d4e") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 07 - Loverboy - Heaven In Your Eyes.mp3") );
    values.insert( Meta::valTitle, QVariant("Heaven In Your Eyes") );
    values.insert( Meta::valArtist, QVariant("Loverboy") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("1fe5897cdea860348c3a5eb40d47c382") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 08 - Larry Greene - Through The Fire.mp3") );
    values.insert( Meta::valTitle, QVariant("Through The Fire") );
    values.insert( Meta::valArtist, QVariant("Larry Greene") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("e0eacff604bfe38b5c275b45aa4f5323") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 09 - Marietta - Destination Unknown.mp3") );
    values.insert( Meta::valTitle, QVariant("Destination Unknown") );
    values.insert( Meta::valArtist, QVariant("Marietta") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("9f1b00dab2df7537b6c5b2be9f08b220") );
    values.insert( Meta::valUrl, QVariant("Top Gun/Top Gun - 10 - Harold Faltermeyer &amp; Steve Stevens - Top Gun Anthem.mp3") );
    values.insert( Meta::valTitle, QVariant("Top Gun Anthem") );
    values.insert( Meta::valArtist, QVariant("Harold Faltermeyer &amp; Steve Stevens") );
    values.insert( Meta::valAlbum, QVariant("Top Gun") );
    createTrack( values );
}

void
TestSqlScanManager::createCompilationLookAlikeAlbum()
{
    Meta::FieldHash values;

    // Some systems have problems with the umlauts in the file names.
    // That is the case where the system encoding when compiling does not
    // match the one of the file system.
    // the following is the original filename
    // values.insert( Meta::valUrl, QVariant( "Glen Hansard & Markéta Irglová/Once/01 Glen Hansard & Markéta Irglová - Falling Slowly.mp3" ) );

    values.insert( Meta::valUniqueId, QVariant( "8375aa24e0e0434ca0c36e382b6f188c" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/01 Glen Hansard & Marketa Irglova - Falling Slowly.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Falling Slowly" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "1" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "ff3f82b1c2e1434d9d1a7b6aec67ac9c" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/02 Glen Hansard & Marketa Irglova - If You Want Me.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "If You Want Me" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "2" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "8fb2396f8d974f6196d2b2ef93ba2551" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/03 Glen Hansard - Broken Hearted Hoover Fixer Sucker Guy.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Broken Hearted Hoover Fixer Sucker Guy" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "3" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "3a211546b91c4bf7a4ec9d41325e5a01" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/04 Glen Hansard & Marketa Irglova - When Your Mind's Made Up.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "When Your Mind's Made Up" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "4" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "e7a1ed52777c437582a217cd29cc35f7" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/05 Glen Hansard - Lies.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Lies" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "5" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "e0c88a85884d40c899522cd733718d9e" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/06 Interference - Gold.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Gold" ) );
    values.insert( Meta::valArtist, QVariant( "Interference" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "6" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "0969ea6128444e128cfcac95207bd525" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/07 Marketa Irglova - The Hill.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "The Hill" ) );
    values.insert( Meta::valArtist, QVariant( "Markéta Irglová" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "7" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "c1d6eff3cb6c42eaa0d63e186ef1b749" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/08 Glen Hansard - Fallen From the Sky.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Fallen From the Sky" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "8" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "b6611dbccd0e49bca8db5dc598b7bf4f" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/09 Glen Hansard - Leave.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Leave" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "9" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "46873076087f48dda553fc5ebd3c0fb6" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/10 Glen Hansard - Trying to Pull Myself Away.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Trying to Pull Myself Away" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "10" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "ea29de7b131c4cf28df177a8cda990ee" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/11 Glen Hansard - All the Way Down.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "All the Way Down" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "11" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "66259801d8ba4d50a2dfdf0129bc8792" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/12 Glen Hansard & Marketa Irglova - Once.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Once" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "12" ) );
    createTrack( values );

    values.insert( Meta::valUniqueId, QVariant( "a654e8c5afb14de7b55b6548ac02f724" ) );
    values.insert( Meta::valUrl, QVariant( "Glen Hansard & Marketa Irglova/Once/13 Glen Hansard - Say It to Me Now.mp3" ) );
    values.insert( Meta::valFormat, QVariant( "1" ) );
    values.insert( Meta::valTitle, QVariant( "Say It to Me Now" ) );
    values.insert( Meta::valArtist, QVariant( "Glen Hansard" ) );
    values.insert( Meta::valAlbum, QVariant( "Once" ) );
    values.insert( Meta::valAlbumArtist, QVariant( "Glen Hansard & Markéta Irglová" ) );
    values.insert( Meta::valTrackNr, QVariant( "13" ) );
    createTrack( values );
}

void
TestSqlScanManager::createCompilationTrack()
{
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96b") );
    values.insert( Meta::valUrl, QVariant("Amazon MP3/The Sum Of All Fears (O.S.T.)/The Sum of All Fears/01 - If We Could Remember (O.S.T. LP Version).mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("If We Could Remember (O.S.T. LP Version)") );
    values.insert( Meta::valArtist, QVariant("The Sum Of All Fears (O.S.T.)/Yolanda Adams") );
    values.insert( Meta::valAlbumArtist, QVariant("The Sum Of All Fears (O.S.T.)") );
    values.insert( Meta::valAlbum, QVariant("The Sum of All Fears") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 203452096") );
    values.insert( Meta::valGenre, QVariant("Soundtracks") );
    values.insert( Meta::valYear, QVariant("2002") );
    values.insert( Meta::valTrackNr, QVariant("1") );
    values.insert( Meta::valComposer, QVariant("Jerry Goldsmith") );
    values.insert( Meta::valScore, QVariant("0.875") );
    values.insert( Meta::valPlaycount, QVariant("6") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("2188afd457cd75a363905f411966b9a0") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/01 - Second Chapter.mp3") );
    values.insert( Meta::valFormat, QVariant(1) );
    values.insert( Meta::valTitle, QVariant("Second Chapter") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985325") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.54") );
    values.insert( Meta::valPlaycount, QVariant("2") );

    values.insert( Meta::valUniqueId, QVariant("637bee4fd456d2ff9eafe65c71ba192e") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/02 - The Eyes Of Truth.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("The Eyes Of Truth") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985326") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant("2004") );
    values.insert( Meta::valTrackNr, QVariant("2") );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.928572") );
    values.insert( Meta::valPlaycount, QVariant("1286469632") );

    values.insert( Meta::valUniqueId, QVariant("b4206da4bc0335d76c2bbc5d4c1b164c") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/03 - Return To Innocence.mp3") );
    values.insert( Meta::valFormat, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Return To Innocence") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985327") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant("2004") );
    values.insert( Meta::valTrackNr, QVariant("3") );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.75") );
    values.insert( Meta::valPlaycount, QVariant("1286469888") );

    values.insert( Meta::valUniqueId, QVariant("eb0061602f52d67140fd465dc275fbf2") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/04 - I Love You...I'Ll Kill You.mp3") );
    values.insert( Meta::valFormat, 1 );
    values.insert( Meta::valTitle, QVariant("I Love You...I'Ll Kill You") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985328") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(4) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant(0.5) );
    values.insert( Meta::valPlaycount, QVariant(1286470656) );

    values.insert( Meta::valUniqueId, QVariant("94dabc09509379646458f62bee7e41ed") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/05 - Silent Warrior.mp3") );
    values.insert( Meta::valFormat, 1 );
    values.insert( Meta::valTitle, QVariant("Silent Warrior") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985329") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(5) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant(0.96875) );
    values.insert( Meta::valPlaycount, QVariant(6) );

    values.insert( Meta::valUniqueId, QVariant("6ae759476c34256ff1d06f0b5c964d75") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/06 - The Dream Of The Dolphin.mp3") );
    values.insert( Meta::valTitle, QVariant("The Dream Of The Dolphin") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985330") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant("2004") );
    values.insert( Meta::valTrackNr, QVariant(6) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant(0.5) );
    values.insert( Meta::valPlaycount, QVariant(2) );

    values.insert( Meta::valUniqueId, QVariant("7957bc25521c1dc91351d497321c27a6") );
    values.insert( Meta::valUrl, QVariant("Amazon MP3/Ashford & Simpson/Solid/01 - Solid.mp3") );
    values.insert( Meta::valTitle, QVariant("Solid") );
    values.insert( Meta::valArtist, QVariant("Ashford &amp; Simpson") );
    values.insert( Meta::valAlbumArtist, QVariant("Ashford &amp; Simpson") );
    values.insert( Meta::valAlbum, QVariant("Solid") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 202265871") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2007) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    values.insert( Meta::valComposer, QVariant("Valerie Simpson") );
    values.insert( Meta::valRating, QVariant(0.898438) );
    values.insert( Meta::valScore, QVariant(0.875) );
    values.insert( Meta::valPlaycount, QVariant(12) );
}


