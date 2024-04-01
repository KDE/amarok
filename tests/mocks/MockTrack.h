/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef META_MOCKTRACK_H
#define META_MOCKTRACK_H

#undef kWarning  // WORKAROUND: Prevent symbols clash with KDE's kWarning macro
#include <gmock/gmock.h>

#include "core/meta/Meta.h"

using ::testing::Return;

namespace Meta
{
class MockTrack : public Meta::Track
{
    public:
    MockTrack() : Meta::Track()
    {
        ON_CALL( *this, name() ).WillByDefault( Return( "" ) );
        ON_CALL( *this, notPlayableReason() ).WillByDefault( Return( QString() ) );
        ON_CALL( *this, artist() ).WillByDefault( Return( Meta::ArtistPtr() ) );
        ON_CALL( *this, album() ).WillByDefault( Return( Meta::AlbumPtr() ) );
        ON_CALL( *this, genre() ).WillByDefault( Return( Meta::GenrePtr() ) );
        ON_CALL( *this, year() ).WillByDefault( Return( Meta::YearPtr() ) );
        ON_CALL( *this, composer() ).WillByDefault( Return( Meta::ComposerPtr() ) );
    }

    MOCK_METHOD( QString, name, (), (const, override) );
    MOCK_METHOD( QString, prettyName, (), (const, override) );
    MOCK_METHOD( QUrl, playableUrl, (), (const, override) );
    MOCK_METHOD( QString, prettyUrl, (), (const, override) );
    MOCK_METHOD( QString, uidUrl, (), (const, override) );
    MOCK_METHOD( QString, notPlayableReason, (), (const, override) );
    MOCK_METHOD( Meta::AlbumPtr, album, (), (const, override) );
    MOCK_METHOD( Meta::ArtistPtr, artist, (), (const, override) );
    MOCK_METHOD( Meta::ComposerPtr, composer, (), (const, override) );
    MOCK_METHOD( Meta::GenrePtr, genre, (), (const, override) );
    MOCK_METHOD( Meta::YearPtr, year, (), (const, override) );
    MOCK_METHOD( qreal, bpm, (), (const, override) );
    MOCK_METHOD( QString, comment, (), (const, override) );
    MOCK_METHOD( double, score, (), (const) );
    MOCK_METHOD1( setScore, void(double score) );
    MOCK_METHOD( int, rating, (), (const) );
    MOCK_METHOD1( setRating, void(int rating) );
    MOCK_METHOD( qint64, length, (), (const, override) );
    MOCK_METHOD( int, filesize, (), (const, override) );
    MOCK_METHOD( int, sampleRate, (), (const, override) );
    MOCK_METHOD( int, bitrate, (), (const, override) );
    MOCK_METHOD( QDateTime, createDate, (), (const, override) );
    MOCK_METHOD( int, trackNumber, (), (const, override) );
    MOCK_METHOD( int, discNumber, (), (const, override) );
    MOCK_CONST_METHOD0( lastPlayed, QDateTime() );
    MOCK_CONST_METHOD0( firstPlayed, QDateTime() );
    MOCK_CONST_METHOD0( playCount, int() );
    MOCK_METHOD( qreal, replayGain, (Meta::ReplayGainTag mode), (const, override) );
    MOCK_METHOD( QString, type, (), (const, override) );
    MOCK_METHOD( void, prepareToPlay, (), (override) );
    MOCK_METHOD( void, finishedPlaying, ( double playedFraction ), (override) );
    MOCK_METHOD( bool, inCollection, (), (const, override) );
    MOCK_METHOD( Collections::Collection*, collection, (), (const, override) );
    MOCK_METHOD( QString, cachedLyrics, (), (const, override) );
};
}

#endif
