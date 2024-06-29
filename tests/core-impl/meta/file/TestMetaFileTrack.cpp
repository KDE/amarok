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

#include "TestMetaFileTrack.h"

#include "config-amarok-test.h"
#include "amarokconfig.h"

#include <KLocalizedString>

#include <QTemporaryDir>

#include <QTest>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>


QTEST_GUILESS_MAIN( TestMetaFileTrack )

TestMetaFileTrack::TestMetaFileTrack()
    : m_tmpDir( nullptr )
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void TestMetaFileTrack::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));

    m_tmpDir = new QTemporaryDir();
    QVERIFY( m_tmpDir->isValid() );
    m_origTrackPath = QStringLiteral( AMAROK_TEST_DIR ) + QStringLiteral("/data/audio/Platz 01.mp3");
    QVERIFY( QFile::exists( m_origTrackPath ) );
}

void TestMetaFileTrack::cleanupTestCase()
{
    delete m_tmpDir;
}

void TestMetaFileTrack::init()
{
    static const QString tmpFileNameBase( QStringLiteral("tempfile.mp3") );
    static int i = 0;
    // create new file name for every test: we need to start with clean statistics
    m_tmpFileName = QStringLiteral( "%1%2-%3" ).arg( m_tmpDir->path() ).arg( i++ ).arg( tmpFileNameBase );
    QVERIFY( QFile::copy( m_origTrackPath, m_tmpFileName ) );
    m_track = new MetaFile::Track( QUrl::fromLocalFile(m_tmpFileName) );
    QVERIFY( m_track );
}

void TestMetaFileTrack::testNameAndSetTitle()
{
    // why aren't those called set/getTitle?
    QCOMPARE( m_track->name(), QStringLiteral( "Platz 01" ) );

    m_track->setTitle( QStringLiteral("") );
    //when there is no title, we default to using the filename without extension
    QCOMPARE( m_track->name(), QStringLiteral( "tempfile" ) );

    m_track->setTitle( QStringLiteral("test") );
    QCOMPARE( m_track->name(), QStringLiteral( "test" ) );

    m_track->setTitle( QStringLiteral("Another Test") );
    QCOMPARE( m_track->name(), QStringLiteral( "Another Test" ) );

    m_track->setTitle( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->name(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setTitle( QStringLiteral("Platz 01") );
    QCOMPARE( m_track->name(), QStringLiteral( "Platz 01" ) );
}

void TestMetaFileTrack::testPrettyName()
{
    QCOMPARE( m_track->prettyName(), QStringLiteral( "Platz 01" ) );

    m_track->setTitle( QStringLiteral("") );
    //when there is no title, we default to using the filename without extension
    QCOMPARE( m_track->prettyName(), QStringLiteral( "tempfile" ) );

    m_track->setTitle( QStringLiteral("test") );
    QCOMPARE( m_track->prettyName(), QStringLiteral( "test" ) );

    m_track->setTitle( QStringLiteral("Another Test") );
    QCOMPARE( m_track->prettyName(), QStringLiteral( "Another Test" ) );

    m_track->setTitle( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->prettyName(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setTitle( QStringLiteral("Platz 01") );
    QCOMPARE( m_track->prettyName(), QStringLiteral( "Platz 01" ) );
}

void TestMetaFileTrack::testSortableName()
{
    QCOMPARE( m_track->sortableName(), QStringLiteral( "Platz 01" ) );

    m_track->setTitle( QStringLiteral("test") );
    QCOMPARE( m_track->sortableName(), QStringLiteral( "test" ) );

    m_track->setTitle( QStringLiteral("Another Test") );
    QCOMPARE( m_track->sortableName(), QStringLiteral( "Another Test" ) );

    m_track->setTitle( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->sortableName(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setTitle( QStringLiteral("Platz 01") );
    QCOMPARE( m_track->sortableName(), QStringLiteral( "Platz 01" ) );
}

void TestMetaFileTrack::testPlayableUrl()
{
    const QUrl tempUrl = m_track->playableUrl();
    QCOMPARE( tempUrl.toLocalFile(), m_tmpFileName );
}

void TestMetaFileTrack::testPrettyUrl()
{
    const QUrl tempUrl = QUrl::fromLocalFile(m_track->prettyUrl());
    QCOMPARE( tempUrl.toLocalFile(), m_tmpFileName );
}

void TestMetaFileTrack::testUidUrl()
{
    QUrl tempUrl(m_track->uidUrl());
    QCOMPARE( tempUrl.toLocalFile(), m_tmpFileName );
}

void TestMetaFileTrack::testIsPlayable()
{
    QVERIFY( m_track->isPlayable() );
}

void TestMetaFileTrack::testIsEditable()
{
    QVERIFY( m_track->isEditable() );

    QFile testFile( m_tmpFileName );

    QVERIFY( testFile.setPermissions( {} ) );
    /* When the tests are run as root under Linux, the file is accessible even when it
     * has no permission bits set. Just skip one verify in this case in order not to
     * break whole test. */
    if( !QFileInfo( testFile ).isReadable() )
        QVERIFY( !m_track->isEditable() );

    QVERIFY( testFile.setPermissions( QFile::ReadOwner | QFile::WriteOwner ) );
    QVERIFY( m_track->isEditable() );
}

void TestMetaFileTrack::testSetGetAlbum()
{
    QCOMPARE( m_track->album()->name(), QStringLiteral( "" ) );

    m_track->setAlbum( QStringLiteral("test") );
    QCOMPARE( m_track->album()->name(), QStringLiteral( "test" ) );

    m_track->setAlbum( QStringLiteral("Another Test") );
    QCOMPARE( m_track->album()->name(), QStringLiteral( "Another Test" ) );

    m_track->setAlbum( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->album()->name(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setAlbum( QStringLiteral("") );
    QCOMPARE( m_track->album()->name(), QStringLiteral( "" ) );
}

void TestMetaFileTrack::testSetGetArtist()
{
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "Free Music Charts" ) );

    m_track->setArtist( QStringLiteral("") );
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "" ) );

    m_track->setArtist( QStringLiteral("test") );
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "test" ) );

    m_track->setArtist( QStringLiteral("Another Test") );
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "Another Test" ) );

    m_track->setArtist( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setArtist( QStringLiteral("Free Music Charts") );
    QCOMPARE( m_track->artist()->name(), QStringLiteral( "Free Music Charts" ) );
}

void TestMetaFileTrack::testSetGetGenre()
{
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "Vocal" ) );

    m_track->setGenre( QStringLiteral("rock") );
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "rock" ) );

    m_track->setGenre( QStringLiteral("rock / pop") );
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "rock / pop" ) );

    m_track->setGenre( QStringLiteral("Some Umlauts: äöü") );
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "Some Umlauts: äöü" ) );

    m_track->setGenre( QStringLiteral("") );
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "" ) );

    m_track->setGenre( QStringLiteral("28 Vocal") );
    QCOMPARE( m_track->genre()->name(), QStringLiteral( "28 Vocal" ) );
}

void TestMetaFileTrack::testSetGetComposer()
{
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "" ) );

    m_track->setComposer( QStringLiteral("test") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "test" ) );

    m_track->setComposer( QStringLiteral("Ludwig Van Beethoven") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "Ludwig Van Beethoven" ) );

    m_track->setComposer( QStringLiteral("Georg Friedrich Händel") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "Georg Friedrich Händel" ) );

    m_track->setComposer( QStringLiteral("") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "" ) );
}

void TestMetaFileTrack::testSetGetYear()
{
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "" ) );

    m_track->setComposer( QStringLiteral("test") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "test" ) );

    m_track->setComposer( QStringLiteral("2009") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "2009" ) );

    m_track->setComposer( QStringLiteral("1") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "1" ) );

    m_track->setComposer( QStringLiteral("0") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "0" ) );

    m_track->setComposer( QStringLiteral("-1") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "-1" ) );

    m_track->setComposer( QStringLiteral("") );
    QCOMPARE( m_track->composer()->name(), QStringLiteral( "" ) );
}

void TestMetaFileTrack::testSetGetComment()
{
    QCOMPARE( m_track->comment(), QStringLiteral( "" ) );

    m_track->setComment( QStringLiteral("test") );
    QCOMPARE( m_track->comment(), QStringLiteral( "test" ) );

    m_track->setComment( QStringLiteral("2009") );
    QCOMPARE( m_track->comment(), QStringLiteral( "2009" ) );

    m_track->setComment( QString::fromUtf8( "Some Umlauts: äöü" ) );
    QCOMPARE( m_track->comment(), QString::fromUtf8( "Some Umlauts: äöü" ) );

    m_track->setComment( QStringLiteral("") );
    QCOMPARE( m_track->comment(), QStringLiteral( "" ) );
}

void TestMetaFileTrack::testSetGetScore()
{
    // try with write back enabled...
    AmarokConfig::setWriteBackStatistics( true );

    Meta::StatisticsPtr statistics = m_track->statistics();
    QCOMPARE( statistics->score(), 0.0 );

    /* now the code actually stores the score in track and then it reads it back.
     * the precision it uses is pretty low and it was failing the qFuzzyCompare<double>
     * Just make it use qFuzzyCompare<float>() */

    statistics->setScore( 3 );
    QCOMPARE( float( statistics->score() ), float( 3.0 ) );

    statistics->setScore( 12.55 );
    QCOMPARE( float( statistics->score() ), float( 12.55 ) );

    statistics->setScore( 100 );
    QCOMPARE( float( statistics->score() ), float( 100.0 ) );

    statistics->setScore( 0 );
    QCOMPARE( float( statistics->score() ), float( 0.0 ) );

    // and with writeback disabled
    AmarokConfig::setWriteBackStatistics( false );

    statistics->setScore( 3 );
    QCOMPARE( float( statistics->score() ), float( 0.0 ) );

    statistics->setScore( 12.55 );
    QCOMPARE( float( statistics->score() ), float( 0.0 ) );
}

void TestMetaFileTrack::testSetGetRating()
{
    // try with write back enabled...
    AmarokConfig::setWriteBackStatistics( true );

    Meta::StatisticsPtr statistics = m_track->statistics();
    QCOMPARE( m_track->statistics()->rating(), 0 );

    m_track->statistics()->setRating( 1 );
    QCOMPARE( m_track->statistics()->rating(), 1 );

    m_track->statistics()->setRating( 23 );
    QCOMPARE( m_track->statistics()->rating(), 23 ); // should this be possible?

    m_track->statistics()->setRating( 0 );
    QCOMPARE( m_track->statistics()->rating(), 0 );

    // and with writeback disabled
    AmarokConfig::setWriteBackStatistics( false );

    m_track->statistics()->setRating( 1 );
    QCOMPARE( m_track->statistics()->rating(), 0 );

    m_track->statistics()->setRating( 23 );
    QCOMPARE( m_track->statistics()->rating(), 0 );
}

void TestMetaFileTrack::testSetGetTrackNumber()
{
    QCOMPARE( m_track->trackNumber(), 0 );

    m_track->setTrackNumber( 1 );
    QCOMPARE( m_track->trackNumber(), 1 );

    m_track->setTrackNumber( 23 );
    QCOMPARE( m_track->trackNumber(), 23 );

    m_track->setTrackNumber( -12 );
    QCOMPARE( m_track->trackNumber(), -12 ); // should this be possible?

    m_track->setTrackNumber( 0 );
    QCOMPARE( m_track->trackNumber(), 0 );
}

void TestMetaFileTrack::testSetGetDiscNumber()
{
    QCOMPARE( m_track->discNumber(), 0 );

    m_track->setDiscNumber( 1 );
    QCOMPARE( m_track->discNumber(), 1 );

    m_track->setDiscNumber( 23 );
    QCOMPARE( m_track->discNumber(), 23 );

    m_track->setDiscNumber( -12 );
    QCOMPARE( m_track->discNumber(), -12 ); // should this be possible?

    m_track->setDiscNumber( 0 );
    QCOMPARE( m_track->discNumber(), 0 );
}

void TestMetaFileTrack::testLength()
{
    QCOMPARE( m_track->length(), 12095LL );
}

void TestMetaFileTrack::testFilesize()
{
    QCOMPARE( m_track->filesize(), 389454 );
}

void TestMetaFileTrack::testSampleRate()
{
    QCOMPARE( m_track->sampleRate(), 44100 );
}

void TestMetaFileTrack::testBitrate()
{
    QCOMPARE( m_track->bitrate(), 257 );
}

void TestMetaFileTrack::testSetGetLastPlayed()
{
    QSKIP( "lastPlayed reading/saving not (yet) available in MetaFile::Track", SkipAll );
    QCOMPARE( m_track->statistics()->lastPlayed(), QDateTime() );

    m_track->finishedPlaying( 1.0 );
    QVERIFY( m_track->statistics()->lastPlayed().isValid() );
}

void TestMetaFileTrack::testSetGetFirstPlayed()
{
    QSKIP( "firstPlayed reading/saving not (yet) available in MetaFile::Track", SkipAll );
    QCOMPARE( m_track->statistics()->firstPlayed(), QDateTime() );

    m_track->finishedPlaying( 1.0 );
    QVERIFY( m_track->statistics()->firstPlayed().isValid() );
}

void TestMetaFileTrack::testSetGetPlayCount()
{
    // try with write back enabled...
    AmarokConfig::setWriteBackStatistics( true );

    QCOMPARE( m_track->statistics()->playCount(), 0 );

    m_track->finishedPlaying( 1.0 );
    QCOMPARE( m_track->statistics()->playCount(), 1 );

    m_track->statistics()->setPlayCount( 0 );
    QCOMPARE( m_track->statistics()->playCount(), 0 );

    // and with writeback disabled
    AmarokConfig::setWriteBackStatistics( false );

    m_track->finishedPlaying( 1.0 );
    QCOMPARE( m_track->statistics()->playCount(), 0 );

    m_track->statistics()->setPlayCount( 12 );
    QCOMPARE( m_track->statistics()->playCount(), 0 );
}

void TestMetaFileTrack::testReplayGain()
{
    QCOMPARE( int(m_track->replayGain( Meta::ReplayGain_Track_Gain ) * 1000), -6655 );
    QCOMPARE( int(m_track->replayGain( Meta::ReplayGain_Album_Gain ) * 1000), -6655 );
    QCOMPARE( int(m_track->replayGain( Meta::ReplayGain_Track_Peak ) * 10000), 41263 );
    QCOMPARE( int(m_track->replayGain( Meta::ReplayGain_Album_Peak ) * 10000), 41263 );
}

void TestMetaFileTrack::testType()
{
    QCOMPARE( m_track->type(), QStringLiteral( "mp3" ) );
}

void TestMetaFileTrack::testCreateDate()
{
    QFileInfo fi( m_tmpFileName );
    QDateTime created = fi.birthTime();
    // m_track->createDate() is rounded to full second because it is created from full seconds
    // created therefore also needs to be rounded
    QCOMPARE( m_track->createDate().toSecsSinceEpoch(), created.toSecsSinceEpoch() );
}
