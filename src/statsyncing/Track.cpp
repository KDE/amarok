/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "Track.h"

#include "MetaValues.h"

using namespace StatSyncing;

Track::Track()
    : QSharedData()
{
}

Track::~Track()
{
}

QString
Track::composer() const
{
    return QString();
}

int
Track::year() const
{
    return 0;
}

int
Track::trackNumber() const
{
    return 0;
}

int
Track::discNumber() const
{
    return 0;
}

bool Track::equals( const Track &other, qint64 fieldMask ) const
{
    if( fieldMask & Meta::valTitle && name() != other.name() )
        return false;
    if( fieldMask & Meta::valAlbum && album() != other.album() )
        return false;
    if( fieldMask & Meta::valArtist && artist() != other.artist() )
        return false;
    if( fieldMask & Meta::valComposer && composer() != other.composer() )
        return false;
    if( fieldMask & Meta::valYear && year() != other.year() )
        return false;
    if( fieldMask & Meta::valTrackNr && trackNumber() != other.trackNumber() )
        return false;
    if( fieldMask & Meta::valDiscNr && discNumber() != other.discNumber() )
        return false;
    return true;
}

bool
Track::lessThan( const Track &other, qint64 fieldMask ) const
{
    // artist > year > album > discNumber > trackNumber > composer > title
    if( fieldMask & Meta::valArtist && artist() != other.artist() )
        return artist() < other.artist();
    if( fieldMask & Meta::valYear && year() != other.year() )
        return year() < other.year();
    if( fieldMask & Meta::valAlbum && album() != other.album() )
        return album() < other.album();
    if( fieldMask & Meta::valDiscNr && discNumber() != other.discNumber() )
        return discNumber() < other.discNumber();
    if( fieldMask & Meta::valTrackNr && trackNumber() != other.trackNumber() )
        return trackNumber() < other.trackNumber();
    if( fieldMask & Meta::valComposer && composer() != other.composer() )
        return composer() < other.composer();
    if( fieldMask & Meta::valTitle && name() != other.name() )
        return name() < other.name();
    return false;
}

int
Track::recentPlayCount() const
{
    return 0;
}
