/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
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
#include "core-implementations/meta/file/File.h"

#include <KTempDir>

#include <QtTest/QTest>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestMetaFileTrack )

TestMetaFileTrack::TestMetaFileTrack()
    : m_track( 0 )
    , m_tmpDir( 0 )
    , m_tmpFileName( "tempfile.mp3" )
{}

void TestMetaFileTrack::initTestCase()
{
    m_tmpDir = new KTempDir();
    QVERIFY( m_tmpDir->exists() );

    const QString path = QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + "/data/audio/Platz 01.mp3" );
    QVERIFY( QFile::exists( path ) );

    const QString tmpFile = m_tmpDir->name() + m_tmpFileName;
    QVERIFY( QFile::copy( path, tmpFile ) );

    m_track = new MetaFile::Track( tmpFile );
    QVERIFY( m_track );
}

void TestMetaFileTrack::cleanupTestCase()
{
    delete m_tmpDir;
    delete m_track;
}

void TestMetaFileTrack::testNameAndSetTitle()
{ 
    // why aren't those called set/getTitle?
    QCOMPARE( m_track->name(), QString( "Platz 01" ) );

    m_track->setTitle( "" );
    //when there is no title, we default to using the filename
    QCOMPARE( m_track->name(), QString( m_tmpFileName ) );

    m_track->setTitle( "test" );
    QCOMPARE( m_track->name(), QString( "test" ) );

    m_track->setTitle( "Another Test" );
    QCOMPARE( m_track->name(), QString( "Another Test" ) );

    m_track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( m_track->name(), QString( "Some Umlauts: äöü" ) );

    m_track->setTitle( "Platz 01" );
    QCOMPARE( m_track->name(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testPrettyName()
{
    QCOMPARE( m_track->prettyName(), QString( "Platz 01" ) );

    m_track->setTitle( "" );
    //when there is no title, we default to using the filename
    QCOMPARE( m_track->prettyName(), QString( m_tmpFileName ) );

    m_track->setTitle( "test" );
    QCOMPARE( m_track->prettyName(), QString( "test" ) );

    m_track->setTitle( "Another Test" );
    QCOMPARE( m_track->prettyName(), QString( "Another Test" ) );

    m_track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( m_track->prettyName(), QString( "Some Umlauts: äöü" ) );

    m_track->setTitle( "Platz 01" );
    QCOMPARE( m_track->prettyName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testFullPrettyName()
{
    QCOMPARE( m_track->fullPrettyName(), QString( "Platz 01" ) );

    m_track->setTitle( "" );
    //when there is no title, we default to using the filename
    QCOMPARE( m_track->fullPrettyName(), QString( m_tmpFileName ) );

    m_track->setTitle( "test" );
    QCOMPARE( m_track->fullPrettyName(), QString( "test" ) );

    m_track->setTitle( "Another Test" );
    QCOMPARE( m_track->fullPrettyName(), QString( "Another Test" ) );

    m_track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( m_track->fullPrettyName(), QString( "Some Umlauts: äöü" ) );

    m_track->setTitle( "Platz 01" );
    QCOMPARE( m_track->fullPrettyName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testSortableName()
{
    QCOMPARE( m_track->sortableName(), QString( "Platz 01" ) );

    m_track->setTitle( "test" );
    QCOMPARE( m_track->sortableName(), QString( "test" ) );

    m_track->setTitle( "Another Test" );
    QCOMPARE( m_track->sortableName(), QString( "Another Test" ) );

    m_track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( m_track->sortableName(), QString( "Some Umlauts: äöü" ) );

    m_track->setTitle( "Platz 01" );
    QCOMPARE( m_track->sortableName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testPlayableUrl()
{
    const KUrl tempUrl = m_track->playableUrl();
    QCOMPARE( tempUrl.toLocalFile(), m_tmpDir->name() + m_tmpFileName );
}

void TestMetaFileTrack::testPrettyUrl()
{
    KUrl tempUrl;
    tempUrl = m_track->prettyUrl();
    QCOMPARE( tempUrl.toLocalFile(), m_tmpDir->name() + m_tmpFileName );
}

void TestMetaFileTrack::testUidUrl()
{
    KUrl tempUrl;
    tempUrl = m_track->uidUrl();
    QCOMPARE( tempUrl.toLocalFile(), m_tmpDir->name() + m_tmpFileName );
}

void TestMetaFileTrack::testIsPlayable()
{
    QVERIFY( m_track->isPlayable() );
}

void TestMetaFileTrack::testIsEditable()
{
    QVERIFY( m_track->isEditable() );

    QFile testFile( m_tmpDir->name() + m_tmpFileName );

    testFile.setPermissions( 0x0000 );
    QVERIFY( !m_track->isEditable() );

    testFile.setPermissions( QFile::ReadOwner | QFile::WriteOwner );
    QVERIFY( m_track->isEditable() );
}

void TestMetaFileTrack::testSetGetAlbum()
{
    QCOMPARE( m_track->album().data()->name(), QString( "" ) );

    m_track->setAlbum( "test" );
    QCOMPARE( m_track->album().data()->name(), QString( "test" ) );

    m_track->setAlbum( "Another Test" );
    QCOMPARE( m_track->album().data()->name(), QString( "Another Test" ) );

    m_track->setAlbum( "Some Umlauts: äöü" );
    QCOMPARE( m_track->album().data()->name(), QString( "Some Umlauts: äöü" ) );

    m_track->setAlbum( "" );
    QCOMPARE( m_track->album().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetArtist()
{
    QCOMPARE( m_track->artist().data()->name(), QString( "Free Music Charts" ) );

    m_track->setArtist( "" );
    QCOMPARE( m_track->artist().data()->name(), QString( "" ) );

    m_track->setArtist( "test" );
    QCOMPARE( m_track->artist().data()->name(), QString( "test" ) );

    m_track->setArtist( "Another Test" );
    QCOMPARE( m_track->artist().data()->name(), QString( "Another Test" ) );

    m_track->setArtist( "Some Umlauts: äöü" );
    QCOMPARE( m_track->artist().data()->name(), QString( "Some Umlauts: äöü" ) );

    m_track->setArtist( "Free Music Charts" );
    QCOMPARE( m_track->artist().data()->name(), QString( "Free Music Charts" ) );
}

void TestMetaFileTrack::testSetGetGenre()
{
    QCOMPARE( m_track->genre().data()->name(), QString( "Vocal" ) );

    m_track->setGenre( "rock" );
    QCOMPARE( m_track->genre().data()->name(), QString( "rock" ) );

    m_track->setGenre( "rock / pop" );
    QCOMPARE( m_track->genre().data()->name(), QString( "rock / pop" ) );

    m_track->setGenre( "Some Umlauts: äöü" );
    QCOMPARE( m_track->genre().data()->name(), QString( "Some Umlauts: äöü" ) );

    m_track->setGenre( "" );
    QCOMPARE( m_track->genre().data()->name(), QString( "" ) );

    m_track->setGenre( "28 Vocal" );
    QCOMPARE( m_track->genre().data()->name(), QString( "28 Vocal" ) );
}

void TestMetaFileTrack::testSetGetComposer()
{
    QCOMPARE( m_track->composer().data()->name(), QString( "" ) );

    m_track->setComposer( "test" );
    QCOMPARE( m_track->composer().data()->name(), QString( "test" ) );

    m_track->setComposer( "Ludwig Van Beethoven" );
    QCOMPARE( m_track->composer().data()->name(), QString( "Ludwig Van Beethoven" ) );

    m_track->setComposer( "Georg Friedrich Händel" );
    QCOMPARE( m_track->composer().data()->name(), QString( "Georg Friedrich Händel" ) );

    m_track->setComposer( "" );
    QCOMPARE( m_track->composer().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetYear()
{
    QCOMPARE( m_track->composer().data()->name(), QString( "" ) );

    m_track->setComposer( "test" );
    QCOMPARE( m_track->composer().data()->name(), QString( "test" ) );

    m_track->setComposer( "2009" );
    QCOMPARE( m_track->composer().data()->name(), QString( "2009" ) );

    m_track->setComposer( "1" );
    QCOMPARE( m_track->composer().data()->name(), QString( "1" ) );

    m_track->setComposer( "0" );
    QCOMPARE( m_track->composer().data()->name(), QString( "0" ) );

    m_track->setComposer( "-1" );
    QCOMPARE( m_track->composer().data()->name(), QString( "-1" ) );

    m_track->setComposer( "" );
    QCOMPARE( m_track->composer().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetComment()
{
    QCOMPARE( m_track->comment(), QString( "" ) );

    m_track->setComment( "test" );
    QCOMPARE( m_track->comment(), QString( "test" ) );

    m_track->setComment( "2009" );
    QCOMPARE( m_track->comment(), QString( "2009" ) );

    m_track->setComment( "Some Umlauts: äöü" );
    QCOMPARE( m_track->comment(), QString( "Some Umlauts: äöü" ) );

    m_track->setComment( "" );
    QCOMPARE( m_track->comment(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetScore()
{
    QCOMPARE( m_track->score(), 0.0 );

    m_track->setScore( 1 );
    QCOMPARE( m_track->score(), 1.0 );

    m_track->setScore( 23.42 );
    QCOMPARE( m_track->score(), 23.42 );

    m_track->setScore( -12 );
    QCOMPARE( m_track->score(), -12.0 ); // should this be possible?

    m_track->setScore( 0 );
    QCOMPARE( m_track->score(), 0.0 );
}

void TestMetaFileTrack::testSetGetRating()
{
    QCOMPARE( m_track->rating(), 0 );

    m_track->setRating( 1 );
    QCOMPARE( m_track->rating(), 1 );

    m_track->setRating( 23 );
    QCOMPARE( m_track->rating(), 23 ); // should this be possible?

    m_track->setRating( -12 );
    QCOMPARE( m_track->rating(), -12 ); // should this be possible?

    m_track->setRating( 0 );
    QCOMPARE( m_track->rating(), 0 );
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
    QCOMPARE( m_track->length(), 12000LL );
}

void TestMetaFileTrack::testFilesize()
{
    QCOMPARE( m_track->filesize(), 389530 );
}

void TestMetaFileTrack::testSampleRate()
{
    QCOMPARE( m_track->sampleRate(), 44100 );
}

void TestMetaFileTrack::testBitrate()
{
    QCOMPARE( m_track->bitrate(), 256 );
}

void TestMetaFileTrack::testSetGetLastPlayed()
{
    QCOMPARE( m_track->lastPlayed(), 4294967295U ); // portability?

    m_track->setLastPlayed( 0 );
    QCOMPARE( m_track->lastPlayed(), 0U );

    m_track->setLastPlayed( 1 );
    QCOMPARE( m_track->lastPlayed(), 1U );

    m_track->setLastPlayed( 23 );
    QCOMPARE( m_track->lastPlayed(), 23U );

    m_track->setLastPlayed( 4294967295U );
    QCOMPARE( m_track->lastPlayed(), 4294967295U );
}

void TestMetaFileTrack::testSetGetFirstPlayed()
{
    QCOMPARE( m_track->firstPlayed(), 4294967295U );

    m_track->setFirstPlayed( 0 );
    QCOMPARE( m_track->firstPlayed(), 0U );

    m_track->setFirstPlayed( 1 );
    QCOMPARE( m_track->firstPlayed(), 1U );

    m_track->setFirstPlayed( 23 );
    QCOMPARE( m_track->firstPlayed(), 23U );

    m_track->setFirstPlayed( 4294967295U );
    QCOMPARE( m_track->firstPlayed(), 4294967295U );
}

void TestMetaFileTrack::testSetGetPlayCount()
{
    QCOMPARE( m_track->playCount(), 0 );

    m_track->setPlayCount( 1 );
    QCOMPARE( m_track->playCount(), 1 );

    m_track->setPlayCount( 23 );
    QCOMPARE( m_track->playCount(), 23 );

    m_track->setPlayCount( -12 );
    QCOMPARE( m_track->playCount(), -12 ); // should this be possible?

    m_track->setPlayCount( 0 );
    QCOMPARE( m_track->playCount(), 0 );
}

void TestMetaFileTrack::testReplayGain()
{
    // HACK: double fails even with fuzzy compare :(
    QCOMPARE( static_cast<float>( m_track->replayGain( Meta::Track::TrackReplayGain ) ), -6.655f );
    QCOMPARE( static_cast<float>( m_track->replayGain( Meta::Track::AlbumReplayGain ) ), -6.655f );
}

void TestMetaFileTrack::testReplayPeakGain()
{
    QCOMPARE( static_cast<float>( m_track->replayPeakGain( Meta::Track::TrackReplayGain ) ), 4.1263f );
    QCOMPARE( static_cast<float>( m_track->replayPeakGain( Meta::Track::AlbumReplayGain ) ), 4.1263f );
}

void TestMetaFileTrack::testType()
{
    QCOMPARE( m_track->type(), QString( "mp3" ) );
}

void TestMetaFileTrack::testCreateDate()
{
    QFileInfo fi( m_tmpDir->name() + m_tmpFileName );
    QDateTime created = fi.created();
    QCOMPARE( m_track->createDate(), created );
}
