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

#include <gmock/gmock.h>

#include "meta/Meta.h"

namespace Meta
{
class MockTrack : public Meta::Track
{
    public:
    MOCK_CONST_METHOD0( name, QString() );
    MOCK_CONST_METHOD0( prettyName, QString() );
    MOCK_CONST_METHOD0( playableUrl, KUrl() );
    MOCK_CONST_METHOD0( prettyUrl, QString() );
    MOCK_CONST_METHOD0( uidUrl, QString() );
    MOCK_CONST_METHOD0( isPlayable, bool() );
    MOCK_CONST_METHOD0( album, Meta::AlbumPtr() );
    MOCK_CONST_METHOD0( artist, Meta::ArtistPtr() );
    MOCK_CONST_METHOD0( composer, Meta::ComposerPtr() );
    MOCK_CONST_METHOD0( genre, Meta::GenrePtr() );
    MOCK_CONST_METHOD0( year, Meta::YearPtr() );
    MOCK_CONST_METHOD0( bpm, float() );
    MOCK_CONST_METHOD0( comment, QString() );
    MOCK_CONST_METHOD0( score, double() );
    MOCK_METHOD1( setScore, void(double score) );
    MOCK_CONST_METHOD0( rating, int() );
    MOCK_METHOD1( setRating, void(int rating) );
    MOCK_CONST_METHOD0( length, qint64() );
    MOCK_CONST_METHOD0( filesize, int() );
    MOCK_CONST_METHOD0( sampleRate, int() );
    MOCK_CONST_METHOD0( bitrate, int() );
    MOCK_CONST_METHOD0( createDate, QDateTime() );
    MOCK_CONST_METHOD0( trackNumber, int() );
    MOCK_CONST_METHOD0( discNumber, int() );
    MOCK_CONST_METHOD0( lastPlayed, uint() );
    MOCK_CONST_METHOD0( firstPlayed, uint() );
    MOCK_CONST_METHOD0( playCount, int() );
    MOCK_CONST_METHOD1( replayGain, qreal(Meta::Track::ReplayGainMode mode) );
    MOCK_CONST_METHOD1( replayPeakGain, qreal(Meta::Track::ReplayGainMode mode) );
    MOCK_CONST_METHOD0( type, QString() );
    MOCK_METHOD0( prepareToPlay, void() );
    MOCK_METHOD1( finishedPlaying, void( double playedFraction ) );
    MOCK_CONST_METHOD0( inCollection, bool() );
    MOCK_CONST_METHOD0( collection, Amarok::Collection*() );
    MOCK_CONST_METHOD0( cachedLyrics, QString() );
};
}

#endif
