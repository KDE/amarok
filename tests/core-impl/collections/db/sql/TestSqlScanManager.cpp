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

// #include "core/support/Debug.h"

#include "amarokconfig.h"
#include "core-impl/meta/file/TagLibUtils.h"
// #include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "core-impl/collections/db/ScanManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/db/sql/SqlCollectionFactory.h"
#include "core-impl/collections/db/sql/SqlRegistry.h"
#include "core-impl/collections/db/sql/mysqlecollection/MySqlEmbeddedStorage.h"
// #include "core/collections/support/SqlStorage.h"

#include "config-amarok-test.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlScanManager )

TestSqlScanManager::TestSqlScanManager()
    : QObject()
{
}

void
TestSqlScanManager::initTestCase()
{
    // that is the original mp3 file that we use to generate the "real" tracks
    m_sourcePath = QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + "/data/audio/Platz 01.mp3" );
    QVERIFY( QFile::exists( m_sourcePath ) );

    m_tmpDatabaseDir = new KTempDir();
    QVERIFY( m_tmpDatabaseDir->exists() );
    m_storage = new MySqlEmbeddedStorage( m_tmpDatabaseDir->name() );
    m_collection = Collections::SqlCollectionFactory().createSqlCollection( "testId", "testscanmanager", m_storage );
    m_scanManager = m_collection->scanManager();

    AmarokConfig::setScanRecursively( true );
    AmarokConfig::setMonitorChanges( false );

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
    delete m_collection;
    //m_storage is deleted by SqlCollection
    delete m_tmpDatabaseDir;
}

void
TestSqlScanManager::init()
{
    m_tmpCollectionDir = new KTempDir();
    QVERIFY( m_tmpCollectionDir->exists() );

    QStringList collectionFolders;
    collectionFolders << m_tmpCollectionDir->name();
    m_collection->mountPointManager()->setCollectionFolders( collectionFolders );
}

void
TestSqlScanManager::cleanup()
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

    delete m_tmpCollectionDir;
}

void
TestSqlScanManager::waitScannerFinished()
{
    QTest::qWait( 200 );
    for( int i=0; i<20 && m_scanManager->isRunning(); i++)
    {
        QTest::qWait( 200 );
    }
}


void
TestSqlScanManager::createTrack( const Meta::FieldHash &values )
{
    // -- copy the file from our original
    QVERIFY( values.contains( Meta::valUrl ) );
    const QString targetPath = m_tmpCollectionDir->name() + values.value( Meta::valUrl ).toString();
    QVERIFY( QDir( m_tmpCollectionDir->name() ).mkpath( QFileInfo( values.value( Meta::valUrl ).toString() ).path() ) );
    QVERIFY( QFile::copy( m_sourcePath, targetPath ) );

    // -- set all the values that we need
    Meta::Field::writeFields( targetPath, values );
}

void
TestSqlScanManager::createSingleTrack()
{
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("794b1bd040d5dd9b5b45c1494d84cc82") );
    values.insert( Meta::valUrl, QVariant("Various Artists/Big Screen Adventures/28 - Theme From Armageddon.mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Theme From Armageddon") );
    values.insert( Meta::valArtist, QVariant("Soundtrack & Theme Orchestra") );
    values.insert( Meta::valAlbumArtist, QVariant("Various Artists") );
    values.insert( Meta::valAlbum, QVariant("Big Screen Adventures") );
    values.insert( Meta::valComposer, QVariant("Unknown Composer") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 210541237") );
    values.insert( Meta::valGenre, QVariant("Broadway & Vocalists") );
    values.insert( Meta::valYear, QVariant(2009) );
    values.insert( Meta::valTrackNr, QVariant(28) );
    // values.insert( Meta::valBitrate, QVariant(216) );
    values.insert( Meta::valBitrate, QVariant(128) );
    values.insert( Meta::valLength, QVariant(184000) );
    values.insert( Meta::valSamplerate, QVariant(44100) );
    values.insert( Meta::valFilesize, QVariant(5094892) );
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
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 01 - Michael Jackson - Track01.mp3") );
    values.insert( Meta::valTitle, QVariant("Wanna Be Startin' Somethin'") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("1dc708934a3e4c51b46577da9b3ab11") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 02 - Michael Jackson - Track02.mp3") );
    values.insert( Meta::valTitle, QVariant("Baby Be Mine") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(2) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("15a6b1bf79747fdc8e9c6b6f06203017") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 03 - Michael Jackson - Track03.mp3") );
    values.insert( Meta::valTitle, QVariant("The Girl Is Mine") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(3) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("4aba4c8b1d1893c03c112cc3c01221e9") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 04 - Michael Jackson - Track04.mp3") );
    values.insert( Meta::valTitle, QVariant("Thriller") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(4) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("cb44d2a3d8053829b04672723bf0bd6e") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 05 - Michael Jackson - Track05.mp3") );
    values.insert( Meta::valTitle, QVariant("Beat It") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(5) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("eba1858eeeb3c6d97fe3385200114d86") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 06 - Michael Jackson - Track06.mp3") );
    values.insert( Meta::valTitle, QVariant("Billy Jean") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(6) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("4623850290998486b0f7b39a2719904e") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 07 - Michael Jackson - Track07.mp3") );
    values.insert( Meta::valTitle, QVariant("Human Nature") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(7) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6d9a7de13af1e16bb13a6208e44b046d") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 08 - Michael Jackson - Track08.mp3") );
    values.insert( Meta::valTitle, QVariant("P.Y.T. (Pretty Young Thing)") );
    values.insert( Meta::valArtist, QVariant("Michael Jackson") );
    values.insert( Meta::valAlbum, QVariant("Thriller") );
    values.insert( Meta::valYear, QVariant(1982) );
    values.insert( Meta::valTrackNr, QVariant(8) );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("91cf9a7c0d255399f9f6babfacae432b") );
    values.insert( Meta::valUrl, QVariant("Thriller/Thriller - 09 - Michael Jackson - Track09.mp3") );
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
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 01 - Kenny Loggins - Danger Zone.mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Danger Zone") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("3e3970f52b0eda3f2a8c1b3a8c8d39ea") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 02 - Cheap Trick - Mighty Wings.mp3") );
    values.insert( Meta::valTitle, QVariant("Mighty Wings") );
    values.insert( Meta::valArtist, QVariant("Cheap Trick") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6ea0bbd97ad8068df58ad75a81f271f7") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 03 - Kenny Loggins - Playing With The Boys.mp3") );
    values.insert( Meta::valTitle, QVariant("Playing With The Boys") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("f3ac2e15288361d779a0ae813a2018ba") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 04 - Teena Marie - Lead Me On.mp3") );
    values.insert( Meta::valTitle, QVariant("Lead Me On") );
    values.insert( Meta::valArtist, QVariant("Teena Marie") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("ffe2bb3e6e2f698983c95e40937545ff") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 05 - Berlin - Take My Breath Away (Love Theme From _Top Gun_).mp3") );
    values.insert( Meta::valTitle, QVariant("Take My Breath Away (Love Theme From &quot;Top Gun&quot;)") );
    values.insert( Meta::valArtist, QVariant("Berlin") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("c871dba16f92483898bcd6a1ed1bc14f") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 06 - Miami Sound Machine - Hot Summer Nights.mp3") );
    values.insert( Meta::valTitle, QVariant("Hot Summer Nights") );
    values.insert( Meta::valArtist, QVariant("Miami Sound Machine") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("80d157c36ed334192ed8df4c01bf0d4e") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 07 - Loverboy - Heaven In Your Eyes.mp3") );
    values.insert( Meta::valTitle, QVariant("Heaven In Your Eyes") );
    values.insert( Meta::valArtist, QVariant("Loverboy") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("1fe5897cdea860348c3a5eb40d47c382") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 08 - Larry Greene - Through The Fire.mp3") );
    values.insert( Meta::valTitle, QVariant("Through The Fire") );
    values.insert( Meta::valArtist, QVariant("Larry Greene") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("e0eacff604bfe38b5c275b45aa4f5323") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 09 - Marietta - Destination Unknown.mp3") );
    values.insert( Meta::valTitle, QVariant("Destination Unknown") );
    values.insert( Meta::valArtist, QVariant("Marietta") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("9f1b00dab2df7537b6c5b2be9f08b220") );
    values.insert( Meta::valUrl, QVariant("Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 10 - Harold Faltermeyer &amp; Steve Stevens - Top Gun Anthem.mp3") );
    values.insert( Meta::valTitle, QVariant("Top Gun Anthem") );
    values.insert( Meta::valArtist, QVariant("Harold Faltermeyer &amp; Steve Stevens") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );
}

void
TestSqlScanManager::createCompilationTrack()
{
    Meta::FieldHash values;

    values.insert( Meta::valUniqueId, QVariant("c6c29f50279ab9523a0f44928bc1e96b") );
    values.insert( Meta::valUrl, QVariant("Amazon MP3/The Sum Of All Fears (O.S.T.)/The Sum of All Fears/01 - If We Could Remember (O.S.T. LP Version).mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("If We Could Remember (O.S.T. LP Version)") );
    values.insert( Meta::valArtist, QVariant("The Sum Of All Fears (O.S.T.)/Yolanda Adams") );
    values.insert( Meta::valAlbumArtist, QVariant("The Sum Of All Fears (O.S.T.)") );
    values.insert( Meta::valAlbum, QVariant("The Sum of All Fears") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 203452096") );
    values.insert( Meta::valGenre, QVariant("Soundtracks") );
    values.insert( Meta::valYear, QVariant("2002") );
    values.insert( Meta::valTrackNr, QVariant("1") );
    values.insert( Meta::valBitrate, QVariant("256") );
    values.insert( Meta::valLength, QVariant("210000") );
    values.insert( Meta::valSamplerate, QVariant("44100") );
    values.insert( Meta::valFilesize, QVariant("6787812") );
    values.insert( Meta::valComposer, QVariant("Jerry Goldsmith") );
    values.insert( Meta::valScore, QVariant("0.875") );
    values.insert( Meta::valPlaycount, QVariant("6") );

    values.insert( Meta::valUniqueId, QVariant("2188afd457cd75a363905f411966b9a0") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/01 - Second Chapter.mp3") );
    values.insert( Meta::valFiletype, QVariant(1) );
    values.insert( Meta::valTitle, QVariant("Second Chapter") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985325") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    values.insert( Meta::valBitrate, QVariant(240) );
    values.insert( Meta::valLength, QVariant(135000) );
    values.insert( Meta::valSamplerate, QVariant("44100") );
    values.insert( Meta::valFilesize, QVariant("4103669") );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.54") );
    values.insert( Meta::valPlaycount, QVariant("2") );

    values.insert( Meta::valUniqueId, QVariant("637bee4fd456d2ff9eafe65c71ba192e") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/02 - The Eyes Of Truth.mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("The Eyes Of Truth") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985326") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant("2004") );
    values.insert( Meta::valTrackNr, QVariant("2") );
    values.insert( Meta::valBitrate, QVariant("256") );
    values.insert( Meta::valLength, QVariant("434000") );
    values.insert( Meta::valSamplerate, QVariant("44100") );
    values.insert( Meta::valFilesize, QVariant("13945872") );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.928572") );
    values.insert( Meta::valPlaycount, QVariant("1286469632") );

    values.insert( Meta::valUniqueId, QVariant("b4206da4bc0335d76c2bbc5d4c1b164c") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/03 - Return To Innocence.mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Return To Innocence") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985327") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant("2004") );
    values.insert( Meta::valTrackNr, QVariant("3") );
    values.insert( Meta::valBitrate, QVariant("248") );
    values.insert( Meta::valLength, QVariant("255000") );
    values.insert( Meta::valSamplerate, QVariant("44100") );
    values.insert( Meta::valFilesize, QVariant("7975210") );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant("0.75") );
    values.insert( Meta::valPlaycount, QVariant("1286469888") );

    values.insert( Meta::valUniqueId, QVariant("eb0061602f52d67140fd465dc275fbf2") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/04 - I Love You...I'Ll Kill You.mp3") );
    values.insert( Meta::valFiletype, 1 );
    values.insert( Meta::valTitle, QVariant("I Love You...I'Ll Kill You") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985328") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(4) );
    values.insert( Meta::valBitrate, QVariant(256) );
    values.insert( Meta::valLength, QVariant(532000) );
    values.insert( Meta::valSamplerate, QVariant(44100) );
    values.insert( Meta::valFilesize, QVariant(17088657) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant(0.5) );
    values.insert( Meta::valPlaycount, QVariant(1286470656) );

    values.insert( Meta::valUniqueId, QVariant("94dabc09509379646458f62bee7e41ed") );
    values.insert( Meta::valUrl, QVariant("The Cross Of Changes/05 - Silent Warrior.mp3") );
    values.insert( Meta::valFiletype, 1 );
    values.insert( Meta::valTitle, QVariant("Silent Warrior") );
    values.insert( Meta::valArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbumArtist, QVariant("Enigma") );
    values.insert( Meta::valAlbum, QVariant("The Cross Of Changes") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 201985329") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2004) );
    values.insert( Meta::valTrackNr, QVariant(5) );
    values.insert( Meta::valBitrate, QVariant(235) );
    values.insert( Meta::valLength, QVariant(369000) );
    values.insert( Meta::valSamplerate, QVariant(44100) );
    values.insert( Meta::valFilesize, QVariant(10919058) );
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
    values.insert( Meta::valBitrate, QVariant(243) );
    values.insert( Meta::valLength, QVariant(167000) );
    values.insert( Meta::valSamplerate, QVariant(44100) );
    values.insert( Meta::valFilesize, QVariant(5128990) );
    values.insert( Meta::valComposer, QVariant("Curly M.C.") );
    values.insert( Meta::valScore, QVariant(0.5) );
    values.insert( Meta::valPlaycount, QVariant(2) );

    values.insert( Meta::valUniqueId, QVariant("7957bc25521c1dc91351d497321c27a6") );
    values.insert( Meta::valUrl, QVariant("Amazon MP3/Ashford &amp; Simpson/Solid/01 - Solid.mp3") );
    values.insert( Meta::valTitle, QVariant("Solid") );
    values.insert( Meta::valArtist, QVariant("Ashford &amp; Simpson") );
    values.insert( Meta::valAlbumArtist, QVariant("Ashford &amp; Simpson") );
    values.insert( Meta::valAlbum, QVariant("Solid") );
    values.insert( Meta::valComment, QVariant("Amazon.com Song ID: 202265871") );
    values.insert( Meta::valGenre, QVariant("Pop") );
    values.insert( Meta::valYear, QVariant(2007) );
    values.insert( Meta::valTrackNr, QVariant(1) );
    values.insert( Meta::valBitrate, QVariant(228) );
    values.insert( Meta::valLength, QVariant(312000) );
    values.insert( Meta::valSamplerate, QVariant(44100) );
    values.insert( Meta::valFilesize, QVariant(8969178) );
    values.insert( Meta::valComposer, QVariant("Valerie Simpson") );
    values.insert( Meta::valRating, QVariant(0.898438) );
    values.insert( Meta::valScore, QVariant(0.875) );
    values.insert( Meta::valPlaycount, QVariant(12) );
}

void
TestSqlScanManager::testScanSingle()
{
    createSingleTrack();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    Meta::TrackPtr track = m_collection->registry()->getTrack( 1 );
    QVERIFY( track );
    QCOMPARE( track->name(), QString("Theme From Armageddon") );
    QVERIFY( track->artist() );
    QCOMPARE( track->artist()->name(), QString("Soundtrack & Theme Orchestra") );
    QVERIFY( track->album() );
    QCOMPARE( track->album()->name(), QString("Big Screen Adventures") );
    QVERIFY( track->album()->isCompilation() );
    QCOMPARE( track->composer()->name(), QString("Unknown Composer") );
    QCOMPARE( track->comment(), QString("Amazon.com Song ID: 210541237") );
    QCOMPARE( track->year()->year(), 2009 );
    QCOMPARE( track->type(), QString("mp3") );
    QCOMPARE( track->trackNumber(), 28 );
    QCOMPARE( track->bitrate(), 216 );
    QCOMPARE( track->length(), qint64(184000) );
    QCOMPARE( track->sampleRate(), 44100 );
    QCOMPARE( track->filesize(), 5094892 );
    qFuzzyCompare( track->score(), 0.875 );
    QCOMPARE( track->playCount(), 5 );
}

void
TestSqlScanManager::testScanDirectory()
{
    createAlbum();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    QCOMPARE( album->name(), QString("Thriller") );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );
}

void
TestSqlScanManager::testCompilation()
{
    createCompilation();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    QCOMPARE( album->name(), QString("Top Gun - (Original Motion Picture Soundtrack)") );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );
}


void
TestSqlScanManager::testRestartScanner()
{
    // the scanner crashes at a special file:
#ifndef QT_NO_DEBUG
#else
    QSKIP( "Collection scanner only crashes in debug build." );
#endif
}

void
TestSqlScanManager::testBlock()
{
    createSingleTrack();
    Meta::TrackPtr track;

    m_scanManager->blockScan(); // block the incremental scanning
    m_scanManager->requestFullScan();

    QTest::qWait( 1000 );
    track = m_collection->registry()->getTrack( 1 );
    QVERIFY( !track );
    QVERIFY( !m_scanManager->isRunning() );

    m_scanManager->unblockScan(); // block the incremental scanning
    // now the actual behaviour is not defined.
    // currently the scanner will not continue after being unblocked.
}

void
TestSqlScanManager::testAddDirectory()
{
    createAlbum();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    createCompilation();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "Top Gun - (Original Motion Picture Soundtrack)", QString() );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );
}

void
TestSqlScanManager::testRemoveDir()
{
    Meta::AlbumPtr album;

    createAlbum();
    createCompilation();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    album = m_collection->registry()->getAlbum( "Top Gun - (Original Motion Picture Soundtrack)", QString() );
    QCOMPARE( album->tracks().count(), 10 );
    QVERIFY( album->isCompilation() );

    // -- remove one album
    album = m_collection->registry()->getAlbum( "Top Gun - (Original Motion Picture Soundtrack)", QString() );
    foreach( Meta::TrackPtr t, album->tracks() )
        QVERIFY( QFile::remove( t->playableUrl().path() ) );
    QVERIFY( QDir( m_tmpCollectionDir->name() ).rmdir( QFileInfo( album->tracks().first()->playableUrl().path() ).path() ) );

    m_scanManager->requestFullScan();
    waitScannerFinished();

    // this one is still here
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );

    // this one is gone
    album = m_collection->registry()->getAlbum( "Top Gun - (Original Motion Picture Soundtrack)", QString() );
    QCOMPARE( album->tracks().count(), 0 );


    // -- remove the second album
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    foreach( Meta::TrackPtr t, album->tracks() )
        QVERIFY( QFile::remove( t->playableUrl().path() ) );
    QVERIFY( QDir( m_tmpCollectionDir->name() ).rmdir( QFileInfo( album->tracks().first()->playableUrl().path() ).path() ) );

    m_scanManager->requestFullScan();
    waitScannerFinished();

    // this both are gone
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 0 );
    album = m_collection->registry()->getAlbum( "Top Gun - (Original Motion Picture Soundtrack)", QString() );
    QCOMPARE( album->tracks().count(), 0 );

}

void
TestSqlScanManager::testRemoveTrack()
{
    Meta::AlbumPtr album;

    createAlbum();
    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    album = m_collection->registry()->getAlbum( "Thriller", "Michael Jackson" );
    QCOMPARE( album->tracks().count(), 9 );
    QVERIFY( !album->isCompilation() );


    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- remove one track
    QVERIFY( QFile::remove( album->tracks().first()->playableUrl().path() ) );

    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check that the track is really gone
    QCOMPARE( album->tracks().count(), 8 );
}

void
TestSqlScanManager::testMerges()
{
    // songs from same album but different directory
    // check that images are merged
    // check that old image is not overwritten
}

void
TestSqlScanManager::testLargeInsert()
{
    /*
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

            SqlScanResultProcessor scp( m_collection, ScanResultProcessor::FullScan );

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
    */
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
    values.insert( Meta::valUrl, QVariant("Kenny Loggins/Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 01 - Kenny Loggins - Danger Zone.mp3") );
    values.insert( Meta::valFiletype, QVariant("1") );
    values.insert( Meta::valTitle, QVariant("Danger Zone") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    values.insert( Meta::valTrackNr, QVariant("1") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("3e3970f52b0eda3f2a8c1b3a8c8d39ea") );
    values.insert( Meta::valUrl, QVariant("Cheap Trick/Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 02 - Cheap Trick - Mighty Wings.mp3") );
    values.insert( Meta::valTitle, QVariant("Mighty Wings") );
    values.insert( Meta::valArtist, QVariant("Cheap Trick") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("6ea0bbd97ad8068df58ad75a81f271f7") );
    values.insert( Meta::valUrl, QVariant("Kenny Loggins/Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 03 - Kenny Loggins - Playing With The Boys.mp3") );
    values.insert( Meta::valTitle, QVariant("Playing With The Boys") );
    values.insert( Meta::valArtist, QVariant("Kenny Loggins") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    values.clear();
    values.insert( Meta::valUniqueId, QVariant("f3ac2e15288361d779a0ae813a2018ba") );
    values.insert( Meta::valUrl, QVariant("Teena Marie/Top Gun - (Original Motion Picture Soundtrack)/Top Gun - (Original Motion Picture Soundtrack) - 04 - Teena Marie - Lead Me On.mp3") );
    values.insert( Meta::valTitle, QVariant("Lead Me On") );
    values.insert( Meta::valArtist, QVariant("Teena Marie") );
    values.insert( Meta::valAlbum, QVariant("Top Gun - (Original Motion Picture Soundtrack)") );
    createTrack( values );

    m_scanManager->requestFullScan();
    waitScannerFinished();

    // -- check the commit
    Meta::AlbumPtr album = m_collection->registry()->getAlbum( 1 );
    QCOMPARE( album->name(), QString("Top Gun - (Original Motion Picture Soundtrack)") );
    QCOMPARE( album->tracks().count(), 4 );
    QVERIFY( album->isCompilation() );
}

