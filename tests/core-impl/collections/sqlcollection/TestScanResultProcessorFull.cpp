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

#include "core/support/Debug.h"

#include "core/meta/support/MetaConstants.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "SqlCollection.h"
#include <collectionscanner/Directory.h>
#include <collectionscanner/Album.h>
#include <collectionscanner/Track.h>
#include <collectionscanner/Playlist.h>
#include "core-impl/collections/db/sql/SqlScanResultProcessor.h"
// #include "DatabaseUpdater.h"
#include "core/collections/support/SqlStorage.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "core-impl/collections/db/sql/SqlRegistry.h"

#include "config-amarok-test.h"
#include "SqlMountPointManagerMock.h"

#include <QTest>
#include <QString>
#include <QXmlStreamReader>

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
    m_collection = new Collections::SqlCollection( "testId", "testcollectionscanner", m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );

    // I just need the table and not the whole playlist manager
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

}

void
TestScanResultProcessorFull::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
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
    m_collection->registry()->emptyCache();
}

/**
 * Check that a single insert really inserts all the information
 */
void
TestScanResultProcessorFull::testSingleInsert()
{
    QXmlStreamReader reader(
    "<directory>"
    "<path>/home/ralf/Musik/Musical/</path>"
    "<mtime>1234</mtime>"
    "<rpath>../../../../Musik/Musical</rpath>"
    "<album>"
    "<name>Grease</name>"
    "<track>"
    "<uniqueid>ab1059a2407fed7d0597526ddd1fd630</uniqueid>"
    "<path>/home/Musik/Musical/John Travolta - You're The One That I Want.mp3</path>"
    "<rpath>../../../Musik/Musical/John Travolta - You're The One That I Want.mp3</rpath>"
    "<filetype>1</filetype>"
    "<title>You're The One That I Want</title>"
    "<artist>John Travolta</artist>"
    "<albumArtist>Grease Cast</albumArtist>"
    "<composer>Warren Casey, Jim Jacobs</composer>"
    "<compilation/>"
    "<hasCover/>"
    "<comment>Hitparade Nummer Eins Hit</comment>"
    "<genre>Musical</genre>"
    "<year>1978</year>"
    "<disc>1</disc>"
    "<track>4</track>"
    "<bpm>60</bpm>"
    "<bitrate>160</bitrate>"
    "<length>167000</length>"
    "<samplerate>44100</samplerate>"
    "<filesize>3362816</filesize>"
    "<trackGain>1.0</trackGain>"
    "<trackPeakGain>2.0</trackPeakGain>"
    "<albumGain>3.0</albumGain>"
    "<albumPeakGain>4.0</albumPeakGain>"
    "</track>"
    "</album>"
    "</directory>"
    );
    reader.readNext();
    reader.readNext();

    CollectionScanner::Directory *csDir = new CollectionScanner::Directory(&reader);

    // -- check that the directory structure is parsed ok
    QCOMPARE( csDir->path(), QString("/home/ralf/Musik/Musical/") );
    QCOMPARE( int(csDir->mtime()), 1234 );
    CollectionScanner::Album album = csDir->albums().first();
    QCOMPARE( album.name(), QString("Grease") );
    QCOMPARE( album.isCompilation(), true );
    CollectionScanner::Track track = album.tracks().first();
    QCOMPARE( track.artist(), QString("John Travolta") );
    QCOMPARE( track.hasCover(), true );
    QCOMPARE( track.bpm(), 60 );

    // -- scan the track
    SqlScanResultProcessor scp( m_collection);
    scp.setType( ScanResultProcessor::FullScan );
    scp.addDirectory( csDir );
    scp.commit();

    // m_collection->dumpDatabaseContent();

    // -- check the commit
    m_collection->registry()->emptyCache(); // -- everything needs to be re-read from database

    Meta::AlbumPtr albumPtr = m_collection->registry()->getAlbum( 1 );
    QVERIFY( albumPtr );
    QCOMPARE( albumPtr->name(), QString("Grease") );
    QCOMPARE( albumPtr->isCompilation(), true );
    // QCOMPARE( albumPtr->hasImage(), true ); // the new SqlAlbum tries to extract the image before adding it. As we don't have the requested file we also don't have an image

    Meta::TrackList tracks = albumPtr->tracks();
    QCOMPARE( tracks.count(), 1 );

    Meta::TrackPtr trackPtr = tracks.first();
    QVERIFY( trackPtr );
    QCOMPARE( trackPtr->name(), QString("You're The One That I Want"));
    QCOMPARE( trackPtr->artist()->name(), QString("John Travolta") );
    QCOMPARE( trackPtr->bpm(), 60.0 );
}


/**
 *  Test adding a whole directory
 */
void
TestScanResultProcessorFull::testAddDirectory()
{
    QXmlStreamReader reader(
        "    <directory>"
        "        <path>/tmp/nosuchdir/Punk/</path>"
        "        <rpath>../../tmp/nosuchdir/Punk</rpath>"
        "        <mtime>1202491675</mtime>"
        "        <album>"
        "            <name>ANThology</name>"
        "            <track>"
        "                <uniqueid>729c5ab527080254307fb03588f750a5</uniqueid>"
        "                <path>/tmp/nosuchdir/Punk/Alien Ant Farm-Smooth Criminal.mp3</path>"
        "                <rpath>../../tmp/nosuchdir/Punk/Alien Ant Farm-Smooth Criminal.mp3</rpath>"
        "                <filetype>1</filetype>"
        "                <title>Smooth Criminal</title>"
        "                <artist>Alien Ant Farm</artist>"
        "                <album>ANThology</album>"
        "            </track>"
        "        </album>"
        "        <album>"
        "            <name></name>"
        "            <track>"
        "                <uniqueid>99aa5e1590eb0bd3d0480afcb905a707</uniqueid>"
        "                <path>/tmp/nosuchdir/Punk/Bums - Punkrock Bier Und Hanf.mp3</path>"
        "                <rpath>../../tmp/nosuchdir/Punk/Bums - Punkrock Bier Und Hanf.mp3</rpath>"
        "                <filetype>1</filetype>"
        "                <title>Punkrock Bier und Hanf</title>"
        "                <artist>Bums</artist>"
        "            </track>"
        "            <track>"
        "                <uniqueid>13b98335a0741eba341426d65e6ce249</uniqueid>"
        "                <path>/tmp/nosuchdir/Punk/Korn - Here To Stay.mp3</path>"
        "                <rpath>../../tmp/nosuchdir/Punk/Korn - Here To Stay.mp3</rpath>"
        "                <filetype>1</filetype>"
        "                <title>Here To Stay (R-Rated)</title>"
        "                <artist>Korn</artist>"
        "            </track>"
        "        </album>"
        "    </directory>"
        );

    reader.readNext();
    reader.readNext();
    CollectionScanner::Directory *csDir = new CollectionScanner::Directory(&reader);

    // -- scan the directory
    SqlScanResultProcessor scp( m_collection);
    scp.setType( ScanResultProcessor::FullScan );
    scp.addDirectory( csDir );
    scp.commit();

    // -- check the commit
    m_collection->registry()->emptyCache(); // -- everything needs to be re-read from database

    Meta::TrackPtr trackPtr;
    Meta::AlbumPtr albumPtr;

    trackPtr = m_collection->registry()->getTrackFromUid( "amarok-sqltrackuid://729c5ab527080254307fb03588f750a5" );
    QVERIFY( trackPtr );
    QCOMPARE( trackPtr->name(), QString("Smooth Criminal"));
    albumPtr = trackPtr->album();
    QVERIFY( albumPtr );
    QCOMPARE( albumPtr->name(), QString("ANThology") );

    // -- the following tracks are in no specific album
    trackPtr = m_collection->registry()->getTrackFromUid( "amarok-sqltrackuid://99aa5e1590eb0bd3d0480afcb905a707" );
    QVERIFY( trackPtr );
    QCOMPARE( trackPtr->name(), QString("Punkrock Bier und Hanf"));

    trackPtr = m_collection->registry()->getTrackFromUid( "amarok-sqltrackuid://13b98335a0741eba341426d65e6ce249" );
    QVERIFY( trackPtr );
    QCOMPARE( trackPtr->name(), QString("Here To Stay (R-Rated)"));
}


/**
 * Test merging of the result with an incremental scan.
 * New files should be inserted
 * Existing files should be merged
 */
void
TestScanResultProcessorFull::testMerges()
{
    // songs from same album but different directory
    // check that images are merged
    // check that old image is not overwritten
}

void
TestScanResultProcessorFull::testLargeInsert()
{
    const int TRACK_COUNT = AMAROK_SQLCOLLECTION_STRESS_TEST_TRACK_COUNT;

    qDebug() << "Please be patient, this test simulates the scan of"<<TRACK_COUNT<<"files and might take a while";

    // -- setup test data
    QList<CollectionScanner::Directory> directories;

    int currentDir = 0;
    int currentArtist = 0;
    int currentAlbum = 0;
    int currentGenre = 0;
    int currentYear = 0;
    int currentComposer = 0;
    int currentTrack = 0;

    for( int dirNr = 0; currentTrack < TRACK_COUNT; dirNr++ )
    {
        currentDir++;
        QString xmlStr = QString("<directory>"
                                 "<path>/%1</path>"
                                 "<rpath>../%2</rpath>"
                                 "<mtime>%3</mtime>").arg( currentDir ).arg( currentDir ).
                                 arg( QDateTime::currentDateTime().toTime_t() );

        for( int albumNr = 0; currentTrack < TRACK_COUNT && albumNr < 20; albumNr++ )
        {
            // There should be a realistic number of doublicates for the following values
            currentAlbum = (currentAlbum+1) % 893;
            currentArtist = (currentArtist+1) % 123;
            currentGenre = (currentGenre+1) % 13;
            currentYear = (currentYear+1) % 47;
            currentComposer = (currentComposer+1) % 31;

            xmlStr += QString( "<album>"
                               "<name>%1</name>" ).arg( currentAlbum );

            for( int trackNr = 0; currentTrack < TRACK_COUNT && trackNr < 20; trackNr++ )
            {
                currentTrack++;

                xmlStr += QString(
                   "<track>"
                   "<uniqueid>amarok-sqltrackuid://%1</uniqueid>"
                   "<path>%2</path>"
                   "<rpath>./%3</rpath>"
                   "<filetype>1</filetype>"
                   "<title>%4</title>"
                   "<artist>%5</artist>"
                   "<album>%6</album>"
                   "<genre>%7</genre>"
                   "<year>%8</year>"
                   "<composer>%9</composer>"
                   "</track>" ).arg( currentTrack ).arg( currentTrack ).arg( currentTrack ).arg( currentTrack ).
                    arg( currentArtist ).arg( currentAlbum ).arg( currentGenre ).arg( currentYear ).arg( currentComposer );
            }

            xmlStr += "</album>";
        }

        xmlStr += "</directory>";

        QXmlStreamReader reader(xmlStr);
        reader.readNext();
        reader.readNext();
        directories.append( CollectionScanner::Directory(&reader) );
    }

    //here we go...
    for( int i = 0; i < 5; i++ )
    {
        QBENCHMARK
        {
            cleanup();

            SqlScanResultProcessor scp( m_collection);
            scp.setType( ScanResultProcessor::FullScan );

            foreach( const CollectionScanner::Directory &dir, directories )
            {
                CollectionScanner::Directory *dirPtr = new CollectionScanner::Directory(dir);
                scp.addDirectory( dirPtr );
            }
            scp.commit();

            QStringList result;
            result = m_storage->query( "select count(*) from urls" );
            QCOMPARE( result.first(), QString::number( TRACK_COUNT ) );

            result = m_storage->query( "select count(*) from tracks" );
            QCOMPARE( result.first(), QString::number( TRACK_COUNT ) );

            result = m_storage->query( "select count(*) from years" );
            QCOMPARE( result.first(), QString( "47" ) );

            result = m_storage->query( "select count(*) from composers" );
            QCOMPARE( result.first(), QString( "31" ) );

            result = m_storage->query( "select count(*) from genres" );
            QCOMPARE( result.first(), QString( "13" ) );

            result = m_storage->query( "select count(*) from albums" );
            QCOMPARE( result.first(), QString( "1000" ) ); // although there are only 893 different album names, there are the full 1000 combinations of album and artist

            result = m_storage->query( "select count(*) from artists" );
            QCOMPARE( result.first(), QString( "123" ) );
        }
    }
}

void
TestScanResultProcessorFull::testIdentifyCompilationInMultipleDirectories()
{
    // Compilations where each is track is from a different artist
    // are often stored as one track per directory, e.g.
    // /artistA/compilation/track1
    // /artistB/compilation/track2
    //
    // this is how Amarok 1 (after using Organize Collection) and iTunes are storing
    // these albums on disc
    // the bad thing is that Amarok 1 (as far as I know) didn't set the id3 tags

    SqlScanResultProcessor scp( m_collection);
    scp.setType( ScanResultProcessor::FullScan );

    for( int artistNr = 0; artistNr < 3; artistNr ++ )
    {
        QString xmlStr = QString("<directory>"
                                 "<path>/%1/compilation/</path>"
                                 "<rpath>../%2/compilation/</rpath>"
                                 "<mtime>1234</mtime>"
                                 " <album>"
                                 "  <name>compilation</name>"
                                 "  <track>"
                                 "   <uniqueid>%3</uniqueid>"
                                 "   <path>/%4/compilation/track</path>"
                                 "   <rpath>../%5/compilation/track</rpath>"
                                 "   <artist>%6</artist>"
                                 "   <album>compilation</album>"
                                 "  </track>"
                                 " </album>"
                                 "</directory>" ).
                                 arg( artistNr ).arg( artistNr ).
                                 arg( artistNr ).arg( artistNr ).
                                 arg( artistNr ).arg( artistNr );

        QXmlStreamReader reader(xmlStr);
        reader.readNext();
        reader.readNext();
        scp.addDirectory( new CollectionScanner::Directory(&reader) );
    }

    scp.commit();

    // -- check the commit
    m_collection->registry()->emptyCache(); // -- everything needs to be re-read from database

    Meta::AlbumPtr albumPtr = m_collection->registry()->getAlbum( "compilation", QString() );
    QVERIFY( albumPtr );
    QCOMPARE( albumPtr->name(), QString("compilation") );
    QCOMPARE( albumPtr->isCompilation(), true );
    QCOMPARE( albumPtr->tracks().count(), 3 );
}

