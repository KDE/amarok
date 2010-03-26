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

#include "TestScanResultProcessorFull.h"

#include <QDebug>
#include "Debug.h"

#include "core/meta/support/MetaConstants.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "SqlCollection.h"
#include "DatabaseUpdater.h"
#include "ScanResultProcessor.h"
#include "collection/SqlStorage.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "collection/sqlcollection/SqlRegistry.h"


#include "config-amarok-test.h"
#include "SqlMountPointManagerMock.h"

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QProcess>
#include <QVariantMap>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestScanResultProcessorFull )

TestScanResultProcessorFull::TestScanResultProcessorFull()
    : QObject()
{
}

void
TestScanResultProcessorFull::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    SqlMountPointManagerMock *mpm = new SqlMountPointManagerMock();
    m_collection->setMountPointManager( mpm );

    DatabaseUpdater *updater = new DatabaseUpdater();
    m_collection->setUpdater( updater );
    updater->setStorage( m_storage );
    updater->setCollection( m_collection );
    updater->update();

    // registry needed when updating urls and nobody checks for NULL
    SqlRegistry *registry = new SqlRegistry( m_collection );
    registry->setStorage( m_storage );
    m_collection->setRegistry( registry );
}

void
TestScanResultProcessorFull::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;
}

void
TestScanResultProcessorFull::cleanup()
{
    m_storage->query( "BEGIN" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE urls;" );
    m_storage->query( "TRUNCATE TABLE directories;" );
    m_storage->query( "COMMIT" );
}

/**
 * Check that a single insert really inserts all the information
 */
void
TestScanResultProcessorFull::testSingleInsert()
{
    QString mockMountPoint = ".";

    QVariantMap track;
    track.insert( Meta::Field::TITLE, "The Morning After" );
    track.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://459234596663341");
    track.insert( Meta::Field::URL, "/home/ralf/songs/Audio Einzeln/Soundtrack/Maureen Mcgovern - The Morning After.mp3" );
    track.insert( Meta::Field::ALBUM, "The Poseidon Adventure Soundtrack");
    track.insert( Meta::Field::ARTIST, "Maureen McGovern" );
    track.insert( Meta::Field::COMPOSER, "Al Kasha and Joel Hirshorn" );
    track.insert( Meta::Field::GENRE, "Soundtrack" );
    track.insert( Meta::Field::YEAR, "1972" );
    track.insert( Meta::Field::COMMENT, "Academy Award 1972" );
    track.insert( Meta::Field::TRACKNUMBER, "2" );
    track.insert( Meta::Field::DISCNUMBER, "1" );
    track.insert( Meta::Field::BITRATE, "128" );
    track.insert( Meta::Field::LENGTH, "321" );
    track.insert( Meta::Field::SAMPLERATE, "44000" );
    track.insert( Meta::Field::FILESIZE, "123456" );
    track.insert( Meta::Field::BPM, "100" );
    track.insert( Meta::Field::ALBUMGAIN, "1" );
    track.insert( Meta::Field::ALBUMPEAKGAIN, "5" );
    track.insert( Meta::Field::TRACKGAIN, "-1" );
    track.insert( Meta::Field::TRACKPEAKGAIN, "-5" );

    QList<QVariantMap> tracksInDir;
    tracksInDir << track;

    // -- scan the track
    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );

    scp.processDirectory( tracksInDir );
    scp.commit();

    // -- check the commit
    QStringList result;
    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );
    QString urlId = result.first();

    result = m_storage->query( QString("SELECT uniqueid FROM urls WHERE id='%1';").arg(urlId));
    QCOMPARE( track.value(Meta::Field::UNIQUEID).toString(), result.first() );

    result = m_storage->query( QString("SELECT title, comment, tracknumber, discnumber, artist, album, genre, composer, year, bitrate, length, filesize, "
                "samplerate, albumgain, albumpeakgain, trackgain, trackpeakgain FROM tracks WHERE url='%1';").arg(urlId));
    QCOMPARE( track.value(Meta::Field::TITLE).toString(), result.at(0) );
    QCOMPARE( track.value(Meta::Field::COMMENT).toString(), result.at(1) );
    QCOMPARE( track.value(Meta::Field::TRACKNUMBER).toString(), result.at(2) );
    QCOMPARE( track.value(Meta::Field::DISCNUMBER).toString(), result.at(3) );
    QString artistId = result.at(4);
    QString albumId = result.at(5);
    QString genreId = result.at(6);
    QString composerId = result.at(7);
    QString yearId = result.at(8);
    QCOMPARE( track.value(Meta::Field::BITRATE).toString(), result.at(9) );
    QCOMPARE( track.value(Meta::Field::LENGTH).toString(), result.at(10) );
    QCOMPARE( track.value(Meta::Field::FILESIZE).toString(), result.at(11) );

    result = m_storage->query( QString("SELECT name FROM artists WHERE id='%1';").arg(artistId));
    QCOMPARE( track.value(Meta::Field::ARTIST).toString(), result.at(0) );

    result = m_storage->query( QString("SELECT name FROM albums WHERE id='%1';").arg(albumId));
    QCOMPARE( track.value(Meta::Field::ALBUM).toString(), result.at(0) );

    result = m_storage->query( QString("SELECT name FROM genres WHERE id='%1';").arg(genreId));
    QCOMPARE( track.value(Meta::Field::GENRE).toString(), result.at(0) );
}


/**
 * After adding a directory all NOT scanned files should be considered removed.
 * New directories should be added to the table.
 */
void
TestScanResultProcessorFull::testAddDirectory()
{
#ifdef Q_OS_WIN32
    QSKIP( "test uses unix commands to create files", QTest::SkipAll );
#endif

    QString mockMountPoint = ".";
    KTempDir tempDir;

    QVariantMap track1;
    track1.insert( Meta::Field::TITLE, "The Morning After" );
    track1.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567891");
    track1.insert( Meta::Field::URL, tempDir.name() + "Maureen Mcgovern - The Morning After.mp3" );

    QVariantMap track2;
    track2.insert( Meta::Field::TITLE, "The Ectasy of Gold" );
    track2.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567892");
    track2.insert( Meta::Field::URL, tempDir.name() + "Ennio Morricone - The Ecstasy of Gold.mp3" );

    QVariantMap track3;
    track3.insert( Meta::Field::TITLE, "Hot Night" );
    track3.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567893");
    track3.insert( Meta::Field::URL, tempDir.name() + "/home/ralf/songs/Audio Einzeln/Soundtrack/Ghostbusters - Laura Branigan - Hot Night.mp3" );


    QList<QVariantMap> tracksInDir;
    tracksInDir << track1 << track2;

    // -- create the original entries
    cleanup();

    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );

    scp.addDirectory( tempDir.name(), 1000 /*time*/ );
    scp.processDirectory( tracksInDir );
    scp.commit();

    // now two tracks
    QStringList result;
    result = m_storage->query( "select count(*) from tracks" );
    QCOMPARE( result.first().toInt(), 2 );

    // -- now overwrite track 2 and add track 3

    tracksInDir.clear();
    tracksInDir << track2 << track3;

    // -- Overwrite here
    // TODO: make it possible to re-use the old processor. It does seem to delete it's temporary tables upon commit
    ScanResultProcessor scp2( m_collection );
    scp2.setSqlStorage( m_storage );
    scp2.setScanType( ScanResultProcessor::FullScan );

    scp2.addDirectory( tempDir.name(), 2000 /*time*/ );
    scp2.processDirectory( tracksInDir );
    scp2.commit();

    // -- check the commit
    // Now we should have two tracks. Track 2 and 3. Track 1 should have been removed
    result = m_storage->query( "select count(*) from tracks" );
    QCOMPARE( result.first().toInt(), 2 );

    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track2.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );

    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track3.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );
}


/**
 * Test merging of the result with an incremental scan.
 * New files should be inserted
 * Existing files should be merged
 */
void
TestScanResultProcessorFull::testMerges()
{
    QString mockMountPoint = ".";

    QVariantMap track1;
    track1.insert( Meta::Field::TITLE, "The Morning After" );
    track1.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567891");
    track1.insert( Meta::Field::URL, "/home/ralf/songs/Audio Einzeln/Soundtrack/Maureen Mcgovern - The Morning After.mp3" );
    track1.insert( Meta::Field::ALBUM, "The Poseidon Adventure Soundtrack");
    track1.insert( Meta::Field::ARTIST, "Maureen McGovern" );

    QVariantMap track2;
    track2.insert( Meta::Field::TITLE, "The Ectasy of Gold" );
    track2.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567892");
    track2.insert( Meta::Field::URL, "/home/ralf/songs/Audio Einzeln/Soundtrack/Ennio Morricone - The Ecstasy of Gold.mp3" );
    track2.insert( Meta::Field::ALBUM, "Kill Bill 2 Soundtrack");
    track2.insert( Meta::Field::ARTIST, "Ennio Morricone" );

    QVariantMap track3;
    track3.insert( Meta::Field::TITLE, "Hot Night" );
    track3.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1234567893");
    track3.insert( Meta::Field::URL, "/home/ralf/songs/Audio Einzeln/Soundtrack/Ghostbusters - Laura Branigan - Hot Night.mp3" );
    track3.insert( Meta::Field::ALBUM, "Ghostbusters Soundtrack");
    track3.insert( Meta::Field::ARTIST, "Laura Branigan" );

    QList<QVariantMap> tracksInDir;
    tracksInDir << track1;
    tracksInDir << track2;

    QStringList result;

    // -- create the original entries
    cleanup();

    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );

    scp.processDirectory( tracksInDir );
    scp.commit();

    result = m_storage->query( "select count(*) from tracks" );
    QCOMPARE( result.first().toInt(), 2 );

    // -- create statistics. They must not be modified
    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track2.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );
    QString urlId = result.first();

    // result = m_storage->query( QString("INSERT id FROM statistics WHERE rpath='%1';").arg(mockMountPoint+track1.value(Meta::Field::URL).toString()));

    // -- now overwrite track 2 and add track 3

    track2.insert( Meta::Field::TITLE, "The Ecstasy of Gold" );
    track2.insert( Meta::Field::ALBUM, "The Good the Bad and the Ugly Soundtrack");
    // changing the path should not be a problem as long as the unique id stays the same
    track2.insert( Meta::Field::URL, "/home/ralf/songs/Audio Einzeln/Soundtrack/Ennio Morricone - The Good The Bad and The Ugly Soundtrack - The Ecstasy of Gold.mp3" );

    tracksInDir.clear();
    tracksInDir << track2 << track3;

    // -- Overwrite here
    // TODO: make it possible to re-use the old processor. It does seem to delete it's temporary tables upon commit
    ScanResultProcessor scp2( m_collection );
    scp2.setSqlStorage( m_storage );
    scp2.setScanType( ScanResultProcessor::IncrementalScan);

    scp2.processDirectory( tracksInDir );
    scp2.commit();

    // -- check the commit

    // now three tracks
    result = m_storage->query( "select count(*) from tracks" );
    QCOMPARE( result.first().toInt(), 3 );

    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track2.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );
    urlId = result.first();

    // --- check that the changed informations are really changed.
    // --- note that overwriting with empty information is undefined
    result = m_storage->query( QString("SELECT title, album FROM tracks WHERE url='%1';").arg(urlId));
    QCOMPARE( result.count(), 2 );
    QCOMPARE( track2.value(Meta::Field::TITLE).toString(), result.at(0) );
    QString albumId = result.at(1);

    result = m_storage->query( QString("SELECT name FROM albums WHERE id='%1';").arg(albumId));
    QCOMPARE( result.count(), 1 );
    QCOMPARE( track2.value(Meta::Field::ALBUM).toString(), result.at(0) );

    result = m_storage->query( QString("SELECT id FROM urls WHERE rpath='%1';").arg(mockMountPoint+track3.value(Meta::Field::URL).toString()));
    QCOMPARE( result.count(), 1 );
    urlId = result.first();

}


void
TestScanResultProcessorFull::testLargeInsert()
{
#ifdef Q_OS_WIN32
    QSKIP( "test uses unix commands to create files", QTest::SkipAll );
#endif

    const int TRACK_COUNT = AMAROK_SQLCOLLECTION_STRESS_TEST_TRACK_COUNT;


    qDebug() << "Please be patient, this test simulates the scan of " << TRACK_COUNT << " files and might take a while";

    KTempDir collection;

    //setup test data

    QList<QPair<QString, uint> > directories;

    QList<QList<QVariantMap> > tracks;

    QList<QVariantMap> tracksInDir;

    int currentArtist = 0;
    int currentAlbum = 0;
    int currentGenre = 0;
    int currentYear = 0;
    int currentComposer = 0;

    QDir currentDir;

    QStringList currentFileNames;

    for( int i = 0; i < TRACK_COUNT; i++ )
    {
        if( i % 20 == 0 )
        {
            if( !currentFileNames.isEmpty() )
            {
                QVERIFY( !QProcess::execute( "touch", currentFileNames ) );
                currentFileNames.clear();
            }
            currentAlbum++;
            currentDir = QDir( collection.name() );
            QVERIFY( currentDir.mkdir( QString::number( currentAlbum ) ) );

            QVERIFY( currentDir.cd( QString::number( currentAlbum ) ) );
            QFileInfo fi( currentDir, "." );
            QPair<QString, uint> dirData( currentDir.absolutePath(), fi.lastModified().toTime_t() );

            if( !tracksInDir.isEmpty() )
            {
                tracks << tracksInDir;
                tracksInDir.clear();
            }

        }
        if( i % 100 == 0 ) currentArtist++;
        if( i % 40 == 0 ) currentGenre++;
        if( i % 500 == 0 ) currentYear++;
        if( i % 25 == 0 ) currentComposer++;


        QString url =currentDir.filePath( QString::number( i ) + ".mp3" );
        currentFileNames << url;

        QVariantMap track;
        track.insert( Meta::Field::TITLE, QString::number( i ) );
        track.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://" + QString::number( i ) );
        track.insert( Meta::Field::URL, url );
        track.insert( Meta::Field::ALBUM, "album" + QString::number( currentAlbum ) );
        track.insert( Meta::Field::ARTIST, "artist" + QString::number( currentArtist ) );
        track.insert( Meta::Field::COMPOSER, "composer" + QString::number( currentComposer ) );
        track.insert( Meta::Field::GENRE, "genre" + QString::number( currentGenre ) );
        track.insert( Meta::Field::YEAR, QString::number( currentYear ) );
        track.insert( Meta::Field::COMMENT, "comment" + QString::number( i ) );

        tracksInDir << track;
    }
    QVERIFY( !QProcess::execute( "touch", currentFileNames ) );

    if( !tracksInDir.isEmpty() )
    {
        tracks << tracksInDir;
        tracksInDir.clear();
    }

    for( int i = 0; i < 5; i++ )
    {
    //here we go...
    QBENCHMARK
    {
        cleanup();

    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );

    typedef QPair<QString, uint> DirMtime;

    foreach( const DirMtime &dir, directories )
    {
        scp.addDirectory( dir.first, dir.second );
    }

    foreach( const QList<QVariantMap> &dir, tracks )
    {
        scp.processDirectory( dir );
    }
    scp.commit();

    QStringList rs1 = m_storage->query( "select count(*) from urls" );
    QCOMPARE( rs1.first(), QString::number( TRACK_COUNT ) );

    QStringList rs2 = m_storage->query( "select count(*) from tracks" );
    QCOMPARE( rs2.first(), QString::number( TRACK_COUNT ) );

    QStringList rs3 = m_storage->query( "select count(*) from years" );
    QCOMPARE( rs3.first(), QString::number( (int) TRACK_COUNT / 500 ) );

    QStringList rs4 = m_storage->query( "select count(*) from composers" );
    QCOMPARE( rs4.first(), QString::number( (int) TRACK_COUNT / 25 ) );

    QStringList rs5 = m_storage->query( "select count(*) from genres" );
    QCOMPARE( rs5.first(), QString::number( (int) TRACK_COUNT / 40 ) );

    QStringList rs6 = m_storage->query( "select count(*) from albums" );
    QCOMPARE( rs6.first(), QString::number( (int) TRACK_COUNT / 20 ) );

    QStringList rs7 = m_storage->query( "select count(*) from artists" );
    QCOMPARE( rs7.first(), QString::number( (int) TRACK_COUNT / 100 ) );
    }
}
}

void
TestScanResultProcessorFull::testIdentifyCompilationInMultipleDirectories()
{
    //Compilations where each is track is from a different artist
    //are often stored as one track per directory, e.g.
    // /artistA/compilation/track1
    // /artistB/compilation/track2
    //
    //this is how Amarok 1 (after using Organize Collection) and iTunes are storing
    //these albums on disc
#ifdef Q_OS_WIN32
    QSKIP( "test uses unix commands to create files", QTest::SkipAll );
#endif

    KTempDir collection;

    QVariantMap dataTemplate;
    dataTemplate.insert( Meta::Field::COMPOSER, "composer" );
    dataTemplate.insert( Meta::Field::GENRE, "genre" );
    dataTemplate.insert( Meta::Field::YEAR, "year" );
    dataTemplate.insert( Meta::Field::ALBUM, "album" );

    QVariantMap file1 = dataTemplate;
    file1.insert( Meta::Field::TITLE, "track1" );
    file1.insert( Meta::Field::ARTIST, "artist1" );
    file1.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1" );
    file1.insert( Meta::Field::URL, collection.name() + "subdir1/file1.mp3" );

    QVariantMap file2 = dataTemplate;
    file2.insert( Meta::Field::TITLE, "track2" );
    file2.insert( Meta::Field::ARTIST, "artist2" );
    file2.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://2" );
    file2.insert( Meta::Field::URL, collection.name() + "subdir2/file2.mp3" );

    QVariantMap file3 = dataTemplate;
    file3.insert( Meta::Field::TITLE, "track3" );
    file3.insert( Meta::Field::ARTIST, "artist3" );
    file3.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://3" );
    file3.insert( Meta::Field::URL, collection.name() + "subdir3/file3.mp3" );

    QList<QVariantMap> allTracks;
    allTracks << file1 << file2 << file3;

    QList<DirMtime> mtimes = setupFileSystem( allTracks );

    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );
    foreach( const DirMtime &mtime, mtimes )
    {
        scp.addDirectory( mtime.first, mtime.second );
    }
    QList<QVariantMap> tracks;
    tracks << file1;
    scp.processDirectory( tracks );
    tracks.clear();
    tracks << file2;
    scp.processDirectory( tracks );
    tracks.clear();
    tracks << file3;
    scp.processDirectory( tracks );

    scp.commit();

    QStringList rs = m_storage->query( "select artist from albums where name = 'album'" );
    QCOMPARE( rs.count(), 1 );
    QVERIFY( rs.first().isEmpty() ); //albums.artist should be null for compilations

    QStringList count = m_storage->query( "select count(*) from albums" );
    QCOMPARE( count.count(), 1 );
    QCOMPARE( count.first(), QString( "1" ) );
}

QList<DirMtime>
TestScanResultProcessorFull::setupFileSystem( const QList<QVariantMap> &trackData )
{
    QList<DirMtime> result;
    QStringList files;
    int count = 0;
    foreach( const QVariantMap &map, trackData )
    {
        QString url = map.value( Meta::Field::URL ).toString();
        if( url.isEmpty() )
        {
            qDebug() << "Warning: track " << map.value( Meta::Field::TITLE ).toString() << " has empty url";
            continue;
        }
        files << url;
        QFileInfo track( url );
        QDir dir = track.dir();
        if( !dir.exists() )
        {
            dir.mkpath( "." );
            QFileInfo dirInfo( dir, "." );
            DirMtime mtime( dir.absolutePath(), dirInfo.lastModified().toTime_t() );
            result << mtime;
        }

        count++;
        if( ( count % 50 == 0) && !files.isEmpty() )
        {
            QProcess::execute( "touch", files );
            files.clear();
        }
    }

    if( !files.isEmpty() )
    {
        QProcess::execute( "touch", files );
        files.clear();
    }

    return result;
}

void
TestScanResultProcessorFull::testAFeatBDetectionInSingleDirectory()
{
    //test whether tracks with the same album name and with artists
    //similar to A featuring B are detected as belonging to one album
    //with the album artist A.

    //Please note: this does not check all variants that Amarok supports
    //like A ft. B or A feat. C. This detection should be extracted into
    //a strategy and tested separately.

#ifdef Q_OS_WIN32
    QSKIP( "test uses unix commands to create files", QTest::SkipAll );
#endif

    KTempDir collection;

    QVariantMap dataTemplate;
    dataTemplate.insert( Meta::Field::COMPOSER, "composer" );
    dataTemplate.insert( Meta::Field::GENRE, "genre" );
    dataTemplate.insert( Meta::Field::YEAR, "year" );
    dataTemplate.insert( Meta::Field::ALBUM, "album" );

    QList<QVariantMap> tracks;

    QVariantMap track1 = dataTemplate;
    track1.insert( Meta::Field::TITLE, "track1" );
    track1.insert( Meta::Field::ARTIST, "artistA feat. artistB" );
    track1.insert( Meta::Field::URL, collection.name() + "dir/track1.mp3" );
    track1.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://1" );
    tracks << track1;

    QVariantMap track2 = dataTemplate;
    track2.insert( Meta::Field::TITLE, "track2" );
    track2.insert( Meta::Field::ARTIST, "artistA" );
    track2.insert( Meta::Field::URL, collection.name() + "dir/track2.mp3" );
    track2.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://2" );
    tracks << track2;

    QVariantMap track3 = dataTemplate;
    track3.insert( Meta::Field::TITLE, "track3" );
    track3.insert( Meta::Field::ARTIST, "artistA featuring artistC" );
    track3.insert( Meta::Field::URL, collection.name() + "dir/track3.mp3" );
    track3.insert( Meta::Field::UNIQUEID, "amarok-sqltrackuid://3" );
    tracks << track3;

    QList<DirMtime> mtimes = setupFileSystem( tracks );

    ScanResultProcessor scp( m_collection );
    scp.setSqlStorage( m_storage );
    scp.setScanType( ScanResultProcessor::FullScan );

    foreach( const DirMtime &mtime, mtimes )
    {
        scp.addDirectory( mtime.first, mtime.second );
    }
    scp.processDirectory( tracks );
    scp.commit();

    //there should be three artists (A feat. B, A, A featuring B) in the artists table
    //and one entry (album) with artistA as albumartist in the albums table

    QStringList artists = m_storage->query( "select name from artists" );
    QCOMPARE( artists.count(), 3 );
    QVERIFY( artists.contains( track1.value( Meta::Field::ARTIST ).toString() ) );
    QVERIFY( artists.contains( track2.value( Meta::Field::ARTIST ).toString() ) );
    QVERIFY( artists.contains( track3.value( Meta::Field::ARTIST ).toString() ) );

    QStringList albums = m_storage->query( "select albums.name, albums.artist, artists.name "
                                           "from albums inner join artists on albums.artist = artists.id" );

    QCOMPARE( albums.count(), 3 );
    QCOMPARE( albums.value( 0 ), QString( "album" ) );
    QCOMPARE( albums.value( 2 ), QString( "artistA" ) );
    QVERIFY( albums.value( 1 ).toInt() != 0 ); //not NULL and not 0; both values result in QString::toInt == 0
}
