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

#include "TestSqlAlbum.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlMountPointManagerMock.h"

#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "covermanager/CoverFetcher.h"

#include <KLocalizedString>

#include <QFile>
#include <QImage>

QTEST_MAIN( TestSqlAlbum )

QTemporaryDir *TestSqlAlbum::s_tmpDir = nullptr;

TestSqlAlbum::TestSqlAlbum()
    : QObject()
    , m_collection( nullptr )
    , m_storage( nullptr )
{
    KLocalizedString::setApplicationDomain("amarok-test");
    std::atexit([]() { delete TestSqlAlbum::s_tmpDir; } );
}

TestSqlAlbum::~TestSqlAlbum()
{ }

void
TestSqlAlbum::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    QVERIFY( s_tmpDir->isValid() );
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );
}

void
TestSqlAlbum::cleanupTestCase()
{
    delete m_collection;
}

void
TestSqlAlbum::init()
{
    //setup base data
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (1, 'artist1');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (2, 'artist2');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (3, 'artist3');") );

    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (1, 'composer1');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (1, 'genre1');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (1, '1');") );

    m_storage->query( QStringLiteral("INSERT INTO directories(id, deviceid, dir) VALUES (1, -1, './');") );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (1, -1, './IDoNotExist.mp3', 1, 'uid://1');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (2, -1, './IDoNotExistAsWell.mp3', 1, 'uid://2');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (3, -1, './MeNeither.mp3', 1, 'uid:/3');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (4, -1, './MeNeitherToo.mp3', 1, 'uid:/4');") );

    CoverFetcher::destroy();

    m_collection->registry()->emptyCache();
}

void
TestSqlAlbum::cleanup()
{
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls_labels;") );
}

void
TestSqlAlbum::testTracks()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2, 'album2',1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,3,'trackInOtherAlbum',1,2,1,1,1 );") );

    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );

    // check two tracks
    Meta::TrackList tracks = sqlAlbum->tracks();
    QCOMPARE( sqlAlbum->tracks().count(), 2 );

    // remove one track
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( tracks.first().data() );
    sqlTrack->setAlbum( QStringLiteral("newAlbum") );
    QCOMPARE( sqlAlbum->tracks().count(), 1 );
}

void
TestSqlAlbum::testIsCompilation()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2, 'compilation',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,1,1,1,1 );") );

    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    QVERIFY( !album->isCompilation() );
    QVERIFY( album->hasAlbumArtist() );

    album = m_collection->registry()->getAlbum( 2 );
    QVERIFY( album->isCompilation() );
    QVERIFY( !album->hasAlbumArtist() );
}

void
TestSqlAlbum::testAlbumArtist()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);") );

    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    QVERIFY( !album->isCompilation() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( album->albumArtist() );
    QCOMPARE( album->albumArtist()->name(), QStringLiteral( "artist1" ) );
}

void
TestSqlAlbum::testImage()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'imageTestAlbum',1);") );

    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    sqlAlbum->setSuppressImageAutoFetch( true );

    QVERIFY( !album->hasImage() );
    QVERIFY( !album->hasImage(100) );
    QVERIFY( !album->hasImage(0) );

    QVERIFY( album->canUpdateImage() );

    // set an image
    sqlAlbum->setImage( QImage( QSize(100, 100), QImage::Format_Mono ) );
    QVERIFY( album->hasImage() );
    QVERIFY( album->hasImage(100) );
    QVERIFY( album->hasImage(0) );
    QCOMPARE( album->image(47).width(), 47 );
    QString largeImagePath = album->imageLocation().path();
    QVERIFY( QFile::exists( largeImagePath ) );

    // remove the image
    album->removeImage();
    QVERIFY( !album->hasImage() );
    QVERIFY( !album->hasImage(100) );
    QVERIFY( !album->hasImage(0) );
    QVERIFY( !QFile::exists( largeImagePath ) );

    // IMPROVEMENT: also test embedded image and an image outside the cache directory
}

void
TestSqlAlbum::testCapabilities()
{
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( QStringLiteral("Test album"), QStringLiteral("test artist") );

    QVERIFY( album->hasCapabilityInterface( Capabilities::Capability::Actions ) );
    Capabilities::ActionsCapability *ac = album->create<Capabilities::ActionsCapability>();
    QVERIFY( ac );
    QVERIFY( ac->actions().count() > 4 );

    QVERIFY( album->hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    Capabilities::BookmarkThisCapability *btc = album->create<Capabilities::BookmarkThisCapability>();
    QVERIFY( btc );
    QVERIFY( btc->bookmarkAction() );

    // need to delete the actions. They hold pointers that prevent the registry from cleaning
    // it's state.
    for( auto action : ac->actions() )
        delete action;
    delete btc->bookmarkAction();
    delete ac;
    delete btc;
}

void
TestSqlAlbum::testSetCompilationWithoutExistingCompilation()
{
    m_collection->registry()->emptyCache();

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
//     auto newAlbum = m_collection->registry()->getAlbum(1);
    Meta::AlbumPtr album = track->album();
    auto sqlAlbum = AmarokSharedPointer<Meta::SqlAlbum>::staticCast( album );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( sqlAlbum->id(), 1 );
    QCOMPARE( album->tracks().count(), 1 );
    sqlAlbum->setCompilation( true );

    // now the track should be in the compilation
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // a new album
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
}

void
TestSqlAlbum::testSetCompilationWithExistingCompilation()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name) VALUES (2, 'albumAndCompilation1');") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );
    Meta::AlbumPtr compilation = m_collection->registry()->getAlbum( 2 );
    QVERIFY( compilation->isCompilation() );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( sqlAlbum->id(), 1 );
    QCOMPARE( album->tracks().count(), 1 );
    sqlAlbum->setCompilation( true );

    // now the track should be in the compilation
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // the existing compilation album
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
    QCOMPARE( album, compilation ); //track returns new album, but the same object that we retrieved above

    QStringList albumResult = m_storage->query( QStringLiteral("SELECT name, artist FROM albums WHERE id = 1") );
}

void
TestSqlAlbum::testUnsetCompilationWithoutExistingAlbum()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
    sqlAlbum->setCompilation( false );

    // now the track should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), QStringLiteral( "album1" ) ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
}

void
TestSqlAlbum::testUnsetCompilationWithExistingAlbum()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2, 'albumAndCompilation1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    // now the track should be in a normal album
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
}

void
TestSqlAlbum::testUnsetCompilationWithMultipleExistingAlbums()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (3,'albumAndCompilation1',2);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,3,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,3,'track3',1,2,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one

    // tracks 2 and 3 should just stay
}

void
TestSqlAlbum::testUnsetCompilationWithArtistAFeaturingB()
{
    m_storage->query( QStringLiteral("INSERT INTO artists(id,name) VALUES (4,'artist1 feat. artist2');") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',4,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,2,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( track->artist()->name(), QStringLiteral( "artist1 feat. artist2" ) );
    QCOMPARE( album->albumArtist()->name(), QStringLiteral("artist1") ); // artist is the same
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one
}

void
TestSqlAlbum::testUnsetCompilationWithMultipleArtists()
{
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1,'album1',NULL);") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,1,1,1,1 );") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QVERIFY( sqlAlbum->id() != 1 ); // the albums should be a new one

    track = m_collection->registry()->getTrack( 2 );
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QVERIFY( sqlAlbum->id() != 1 ); // the albums should be a new one
}

