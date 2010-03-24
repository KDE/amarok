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

#include "core/meta/impl/file/File.h"

#include <KStandardDirs>

#include <QtTest/QTest>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestMetaFileTrack )

TestMetaFileTrack::TestMetaFileTrack()
{}

void TestMetaFileTrack::initTestCase()
{
    QFile::remove( QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
    const QString relPath = "amarok/testdata/audio/Platz 01.mp3";
    const QString path = KStandardDirs::locate( "data", QDir::toNativeSeparators( relPath ) );
    QVERIFY( QFile::copy( path, QDir::tempPath() + QDir::separator() + "tempfile.mp3" ) );
    track = new MetaFile::Track( QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
}


void TestMetaFileTrack::testNameAndSetTitle()
{
    // why aren't those called set/getTitle?
    QCOMPARE( track->name(), QString( "Platz 01" ) );

    track->setTitle( "" );
    QCOMPARE( track->name(), QString( "" ) );

    track->setTitle( "test" );
    QCOMPARE( track->name(), QString( "test" ) );

    track->setTitle( "Another Test" );
    QCOMPARE( track->name(), QString( "Another Test" ) );

    track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( track->name(), QString( "Some Umlauts: äöü" ) );

    track->setTitle( "Platz 01" );
    QCOMPARE( track->name(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testPrettyName()
{
    QCOMPARE( track->prettyName(), QString( "Platz 01" ) );

    track->setTitle( "" );
    QCOMPARE( track->prettyName(), QString( "" ) );

    track->setTitle( "test" );
    QCOMPARE( track->prettyName(), QString( "test" ) );

    track->setTitle( "Another Test" );
    QCOMPARE( track->prettyName(), QString( "Another Test" ) );

    track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( track->prettyName(), QString( "Some Umlauts: äöü" ) );

    track->setTitle( "Platz 01" );
    QCOMPARE( track->prettyName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testFullPrettyName()
{
    QCOMPARE( track->fullPrettyName(), QString( "Platz 01" ) );

    track->setTitle( "" );
    QCOMPARE( track->fullPrettyName(), QString( "" ) );

    track->setTitle( "test" );
    QCOMPARE( track->fullPrettyName(), QString( "test" ) );

    track->setTitle( "Another Test" );
    QCOMPARE( track->fullPrettyName(), QString( "Another Test" ) );

    track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( track->fullPrettyName(), QString( "Some Umlauts: äöü" ) );

    track->setTitle( "Platz 01" );
    QCOMPARE( track->fullPrettyName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testSortableName()
{
    QCOMPARE( track->sortableName(), QString( "Platz 01" ) );

    track->setTitle( "test" );
    QCOMPARE( track->sortableName(), QString( "test" ) );

    track->setTitle( "Another Test" );
    QCOMPARE( track->sortableName(), QString( "Another Test" ) );

    track->setTitle( "Some Umlauts: äöü" );
    QCOMPARE( track->sortableName(), QString( "Some Umlauts: äöü" ) );

    track->setTitle( "Platz 01" );
    QCOMPARE( track->sortableName(), QString( "Platz 01" ) );
}

void TestMetaFileTrack::testPlayableUrl()
{
    KUrl tempUrl;
    tempUrl = track->playableUrl();
    QCOMPARE( tempUrl.toLocalFile(), QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
}

void TestMetaFileTrack::testPrettyUrl()
{
    KUrl tempUrl;
    tempUrl = track->prettyUrl();
    QCOMPARE( tempUrl.toLocalFile(), QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
}

void TestMetaFileTrack::testUidUrl()
{
    KUrl tempUrl;
    tempUrl = track->uidUrl();
    QCOMPARE( tempUrl.toLocalFile(), QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
}

void TestMetaFileTrack::testIsPlayable()
{
    QVERIFY( track->isPlayable() );
}

void TestMetaFileTrack::testIsEditable()
{
    QVERIFY( track->isEditable() );

    QFile testFile( QDir::tempPath() + QDir::separator() + "tempfile.mp3" );

    testFile.setPermissions( 0x0000 );
    QVERIFY( !track->isEditable() );

    testFile.setPermissions( QFile::ReadOwner | QFile::WriteOwner );
    QVERIFY( track->isEditable() );
}

void TestMetaFileTrack::testSetGetAlbum()
{
    QCOMPARE( track->album().data()->name(), QString( "" ) );

    track->setAlbum( "test" );
    QCOMPARE( track->album().data()->name(), QString( "test" ) );

    track->setAlbum( "Another Test" );
    QCOMPARE( track->album().data()->name(), QString( "Another Test" ) );

    track->setAlbum( "Some Umlauts: äöü" );
    QCOMPARE( track->album().data()->name(), QString( "Some Umlauts: äöü" ) );

    track->setAlbum( "" );
    QCOMPARE( track->album().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetArtist()
{
    QCOMPARE( track->artist().data()->name(), QString( "Free Music Charts" ) );

    track->setArtist( "" );
    QCOMPARE( track->artist().data()->name(), QString( "" ) );

    track->setArtist( "test" );
    QCOMPARE( track->artist().data()->name(), QString( "test" ) );

    track->setArtist( "Another Test" );
    QCOMPARE( track->artist().data()->name(), QString( "Another Test" ) );

    track->setArtist( "Some Umlauts: äöü" );
    QCOMPARE( track->artist().data()->name(), QString( "Some Umlauts: äöü" ) );

    track->setArtist( "Free Music Charts" );
    QCOMPARE( track->artist().data()->name(), QString( "Free Music Charts" ) );
}

void TestMetaFileTrack::testSetGetGenre()
{
    QCOMPARE( track->genre().data()->name(), QString( "28 Vocal" ) );

    track->setGenre( "rock" );
    QCOMPARE( track->genre().data()->name(), QString( "rock" ) );

    track->setGenre( "rock / pop" );
    QCOMPARE( track->genre().data()->name(), QString( "rock / pop" ) );

    track->setGenre( "Some Umlauts: äöü" );
    QCOMPARE( track->genre().data()->name(), QString( "Some Umlauts: äöü" ) );

    track->setGenre( "" );
    QCOMPARE( track->genre().data()->name(), QString( "" ) );

    track->setGenre( "28 Vocal" );
    QCOMPARE( track->genre().data()->name(), QString( "28 Vocal" ) );
}

void TestMetaFileTrack::testSetGetComposer()
{
    QCOMPARE( track->composer().data()->name(), QString( "" ) );

    track->setComposer( "test" );
    QCOMPARE( track->composer().data()->name(), QString( "test" ) );

    track->setComposer( "Ludwig Van Beethoven" );
    QCOMPARE( track->composer().data()->name(), QString( "Ludwig Van Beethoven" ) );

    track->setComposer( "Georg Friedrich Händel" );
    QCOMPARE( track->composer().data()->name(), QString( "Georg Friedrich Händel" ) );

    track->setComposer( "" );
    QCOMPARE( track->composer().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetYear()
{
    QCOMPARE( track->composer().data()->name(), QString( "" ) );

    track->setComposer( "test" );
    QCOMPARE( track->composer().data()->name(), QString( "test" ) );

    track->setComposer( "2009" );
    QCOMPARE( track->composer().data()->name(), QString( "2009" ) );

    track->setComposer( "1" );
    QCOMPARE( track->composer().data()->name(), QString( "1" ) );

    track->setComposer( "0" );
    QCOMPARE( track->composer().data()->name(), QString( "0" ) );

    track->setComposer( "-1" );
    QCOMPARE( track->composer().data()->name(), QString( "-1" ) );

    track->setComposer( "" );
    QCOMPARE( track->composer().data()->name(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetComment()
{
    QCOMPARE( track->comment(), QString( "" ) );

    track->setComment( "test" );
    QCOMPARE( track->comment(), QString( "test" ) );

    track->setComment( "2009" );
    QCOMPARE( track->comment(), QString( "2009" ) );

    track->setComment( "Some Umlauts: äöü" );
    QCOMPARE( track->comment(), QString( "Some Umlauts: äöü" ) );

    track->setComment( "" );
    QCOMPARE( track->comment(), QString( "" ) );
}

void TestMetaFileTrack::testSetGetScore()
{
    QCOMPARE( track->score(), 0.0 );

    track->setScore( 1 );
    QCOMPARE( track->score(), 1.0 );

    track->setScore( 23.42 );
    QCOMPARE( track->score(), 23.42 );

    track->setScore( -12 );
    QCOMPARE( track->score(), -12.0 ); // should this be possible?

    track->setScore( 0 );
    QCOMPARE( track->score(), 0.0 );
}

void TestMetaFileTrack::testSetGetRating()
{
    QCOMPARE( track->rating(), 0 );

    track->setRating( 1 );
    QCOMPARE( track->rating(), 1 );

    track->setRating( 23 );
    QCOMPARE( track->rating(), 23 ); // should this be possible?

    track->setRating( -12 );
    QCOMPARE( track->rating(), -12 ); // should this be possible?

    track->setRating( 0 );
    QCOMPARE( track->rating(), 0 );
}

void TestMetaFileTrack::testSetGetTrackNumber()
{
    QCOMPARE( track->trackNumber(), 0 );

    track->setTrackNumber( 1 );
    QCOMPARE( track->trackNumber(), 1 );

    track->setTrackNumber( 23 );
    QCOMPARE( track->trackNumber(), 23 );

    track->setTrackNumber( -12 );
    QCOMPARE( track->trackNumber(), -12 ); // should this be possible?

    track->setTrackNumber( 0 );
    QCOMPARE( track->trackNumber(), 0 );
}

void TestMetaFileTrack::testSetGetDiscNumber()
{
    QCOMPARE( track->discNumber(), 0 );

    track->setDiscNumber( 1 );
    QCOMPARE( track->discNumber(), 1 );

    track->setDiscNumber( 23 );
    QCOMPARE( track->discNumber(), 23 );

    track->setDiscNumber( -12 );
    QCOMPARE( track->discNumber(), -12 ); // should this be possible?

    track->setDiscNumber( 0 );
    QCOMPARE( track->discNumber(), 0 );
}

void TestMetaFileTrack::testLength()
{
    QCOMPARE( track->length(), 12000LL );
}

void TestMetaFileTrack::testFilesize()
{
    QCOMPARE( track->filesize(), 389530 );
}

void TestMetaFileTrack::testSampleRate()
{
    QCOMPARE( track->sampleRate(), 44100 );
}

void TestMetaFileTrack::testBitrate()
{
    QCOMPARE( track->bitrate(), 258 );
}

void TestMetaFileTrack::testSetGetLastPlayed()
{
    QCOMPARE( track->lastPlayed(), 4294967295U ); // portability?

    track->setLastPlayed( 0 );
    QCOMPARE( track->lastPlayed(), 0U );

    track->setLastPlayed( 1 );
    QCOMPARE( track->lastPlayed(), 1U );

    track->setLastPlayed( 23 );
    QCOMPARE( track->lastPlayed(), 23U );

    track->setLastPlayed( 4294967295U );
    QCOMPARE( track->lastPlayed(), 4294967295U );
}

void TestMetaFileTrack::testSetGetFirstPlayed()
{
    QCOMPARE( track->firstPlayed(), 4294967295U );

    track->setFirstPlayed( 0 );
    QCOMPARE( track->firstPlayed(), 0U );

    track->setFirstPlayed( 1 );
    QCOMPARE( track->firstPlayed(), 1U );

    track->setFirstPlayed( 23 );
    QCOMPARE( track->firstPlayed(), 23U );

    track->setFirstPlayed( 4294967295U );
    QCOMPARE( track->firstPlayed(), 4294967295U );
}

void TestMetaFileTrack::testSetGetPlayCount()
{
    QCOMPARE( track->playCount(), 0 );

    track->setPlayCount( 1 );
    QCOMPARE( track->playCount(), 1 );

    track->setPlayCount( 23 );
    QCOMPARE( track->playCount(), 23 );

    track->setPlayCount( -12 );
    QCOMPARE( track->playCount(), -12 ); // should this be possible?

    track->setPlayCount( 0 );
    QCOMPARE( track->playCount(), 0 );
}

void TestMetaFileTrack::testReplayGain()
{
    // HACK: double fails even with fuzzy compare :(
    QCOMPARE( static_cast<float>( track->replayGain( Meta::Track::TrackReplayGain ) ), -6.655f );
    QCOMPARE( static_cast<float>( track->replayGain( Meta::Track::AlbumReplayGain ) ), -6.655f );
}

void TestMetaFileTrack::testReplayPeakGain()
{
    QCOMPARE( static_cast<float>( track->replayPeakGain( Meta::Track::TrackReplayGain ) ), 4.1263f );
    QCOMPARE( static_cast<float>( track->replayPeakGain( Meta::Track::AlbumReplayGain ) ), 4.1263f );
}

void TestMetaFileTrack::testType()
{
    QCOMPARE( track->type(), QString( "mp3" ) );
}

void TestMetaFileTrack::testCreateDate()
{
    QFileInfo fi( QDir::tempPath() + QDir::separator() + "tempfile.mp3" );
    QDateTime created = fi.created();
    QCOMPARE( track->createDate(), created );
}
