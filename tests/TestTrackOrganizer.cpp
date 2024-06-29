/***************************************************************************
 *   Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>              *
 *   Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>*
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

#include "TestTrackOrganizer.h"

#include "dialogs/TrackOrganizer.h"
#include "core/support/Debug.h"

#include "CollectionTestImpl.h"

#include "mocks/MockTrack.h"
#include "mocks/MockAlbum.h"
#include "mocks/MockArtist.h"

#include <KLocalizedString>

#include <QMap>
#include <QTest>

#include <gmock/gmock.h>

using ::testing::Return;
using ::testing::AnyNumber;

QTEST_GUILESS_MAIN( TestTrackOrganizer )

namespace Collections {

class MyCollectionTestImpl : public CollectionTestImpl
{
public:
    MyCollectionTestImpl( const QString &id ) : CollectionTestImpl( id ) {}

};

} //namespace Collections

TestTrackOrganizer::TestTrackOrganizer()
{
  KLocalizedString::setApplicationDomain("amarok-test");
}

void TestTrackOrganizer::init()
{
  mColl = new Collections::MyCollectionTestImpl(QStringLiteral("A"));
}

void TestTrackOrganizer::cleanup()
{
  delete mColl;
  delete mTrackOrganizer;
}

void TestTrackOrganizer::testBasic()
{

  mTracks = makeTracks( 10 );
  mTrackOrganizer = new TrackOrganizer( mTracks, this );
  QString folder = QStringLiteral("/home/user/Music");
  mTrackOrganizer->setFormatString( QStringLiteral("%collectionroot%/%artist%/%album%/%track%-%title%.%filetype%") );
  mTrackOrganizer->setFolderPrefix( folder );
  QMap <Meta::TrackPtr, QString > dests = mTrackOrganizer->getDestinations();
  QVERIFY( dests.size() == 10 );

  for( auto const &track : mTracks )
  {
    QVERIFY( dests.contains( track ) );
    QString format = QStringLiteral("%1/%2/%3/%4-%5.%6");
    QString trackNum = QStringLiteral("%1").arg( track->trackNumber(), 2, 10, QLatin1Char('0') );
    QString result = format.arg( folder, track->artist()->prettyName(), track->album()->prettyName(), trackNum, track->prettyName(), QStringLiteral("mp3"));
    QCOMPARE( dests.value( track ), result );
  }
}

Meta::TrackPtr TestTrackOrganizer::makeMockTrack( const QString &trackName, const QString &artistName, const QString &albumName, int trackNumber )
{
    Meta::MockTrack *track = new Meta::MockTrack();
    ::testing::Mock::AllowLeak( track );
    Meta::TrackPtr trackPtr( track );
    EXPECT_CALL( *track, name() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName ) );
    EXPECT_CALL( *track, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName ) );
    EXPECT_CALL( *track, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName + QLatin1Char('_') + artistName + QLatin1Char('_') + albumName ) );
    EXPECT_CALL( *track, playableUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( QUrl( QLatin1Char('/') + track->uidUrl() ) ) );
    EXPECT_CALL( *track, trackNumber() ).Times( AnyNumber() ).WillRepeatedly( Return( trackNumber ) );
    EXPECT_CALL( *track, type() ).Times( AnyNumber() ).WillRepeatedly( Return( QStringLiteral("mp3") ) );
    EXPECT_CALL( *track, composer() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::ComposerPtr() ) );
    EXPECT_CALL( *track, year() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::YearPtr() ) );
    EXPECT_CALL( *track, genre() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::GenrePtr() ) );
    EXPECT_CALL( *track, discNumber() ).Times( AnyNumber() ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( *track, rating() ).Times( AnyNumber() ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( *track, filesize() ).Times( AnyNumber() ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( *track, length() ).Times( AnyNumber() ).WillRepeatedly( Return( 0 ) );
    EXPECT_CALL( *track, comment() ).Times( AnyNumber() ).WillRepeatedly( Return( QStringLiteral("") ) );

    mColl->mc->addTrack( trackPtr );

    Meta::AlbumPtr albumPtr = mColl->mc->albumMap().value( albumName, QString() /* no album artist */ );
    Meta::MockAlbum *album;
    Meta::TrackList albumTracks;
    if( albumPtr )
    {
        album = dynamic_cast<Meta::MockAlbum*>( albumPtr.data() );
        if( !album )
        {
            qFatal("expected a Meta::MockAlbum");
//             QFAIL( "expected a Meta::MockAlbum" );
            return trackPtr;
        }
        albumTracks = albumPtr->tracks();
    }
    else
    {
        album = new Meta::MockAlbum();
        ::testing::Mock::AllowLeak( album );
        albumPtr = Meta::AlbumPtr( album );
        EXPECT_CALL( *album, name() ).Times( AnyNumber() ).WillRepeatedly( Return( albumName ) );
        EXPECT_CALL( *album, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( albumName ) );
        EXPECT_CALL( *album, hasAlbumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( false ) );
        EXPECT_CALL( *album, albumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::ArtistPtr() ) );
        EXPECT_CALL( *album, isCompilation() ).Times( AnyNumber() ).WillRepeatedly( Return( false ) );
        mColl->mc->addAlbum( albumPtr );
    }
    albumTracks << trackPtr;
    EXPECT_CALL( *album, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( albumTracks ) );

    EXPECT_CALL( *track, album() ).Times( AnyNumber() ).WillRepeatedly( Return( albumPtr ) );

    Meta::ArtistPtr artistPtr = mColl->mc->artistMap().value( artistName );
    Meta::MockArtist *artist;
    Meta::TrackList artistTracks;
    if( artistPtr )
    {
        artist = dynamic_cast<Meta::MockArtist*>( artistPtr.data() );
        if( !artist )
        {
            qFatal("expected a Meta::MockArtist");
//             QFAIL( "expected a Meta::MockArtist" );
            return trackPtr;
        }
        artistTracks = artistPtr->tracks();
    }
    else
    {
        artist = new Meta::MockArtist();
        ::testing::Mock::AllowLeak( artist );
        artistPtr = Meta::ArtistPtr( artist );
        EXPECT_CALL( *artist, name() ).Times( AnyNumber() ).WillRepeatedly( Return( artistName ) );
	EXPECT_CALL( *artist, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( artistName ) );
        mColl->mc->addArtist( artistPtr );
    }
    artistTracks << trackPtr;
    EXPECT_CALL( *artist, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( artistTracks ) );
    EXPECT_CALL( *track, artist() ).Times( AnyNumber() ).WillRepeatedly( Return( artistPtr ) );
    return trackPtr;
}

Meta::TrackList TestTrackOrganizer::makeTracks( int numTracks )
{
  Meta::TrackList tracks;
  for( int i = 1; i <= numTracks; ++i )
  {
    QString title = QStringLiteral("Title") + QString::number(i);
    Meta::TrackPtr t = makeMockTrack( title, QStringLiteral("Artist1"), QStringLiteral("Album1"), i );
    if( t )
      tracks << t;
  }
  return tracks;
}
