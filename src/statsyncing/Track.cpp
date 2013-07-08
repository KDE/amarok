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
#include "core/meta/Meta.h"

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
    if( fieldMask & Meta::valTitle && name().toLower() != other.name().toLower() )
        return false;
    if( fieldMask & Meta::valAlbum && album().toLower() != other.album().toLower() )
        return false;
    if( fieldMask & Meta::valArtist && artist().toLower() != other.artist().toLower() )
        return false;
    if( fieldMask & Meta::valComposer && composer().toLower() != other.composer().toLower() )
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
    if( fieldMask & Meta::valArtist && artist().toLower() != other.artist().toLower() )
        return artist().toLower() < other.artist().toLower();
    if( fieldMask & Meta::valYear && year() != other.year() )
        return year() < other.year();
    if( fieldMask & Meta::valAlbum && album().toLower() != other.album().toLower() )
        return album().toLower() < other.album().toLower();
    if( fieldMask & Meta::valDiscNr && discNumber() != other.discNumber() )
        return discNumber() < other.discNumber();
    if( fieldMask & Meta::valTrackNr && trackNumber() != other.trackNumber() )
        return trackNumber() < other.trackNumber();
    if( fieldMask & Meta::valComposer && composer().toLower() != other.composer().toLower() )
        return composer().toLower() < other.composer().toLower();
    if( fieldMask & Meta::valTitle && name().toLower() != other.name().toLower() )
        return name().toLower() < other.name().toLower();
    return false;
}

void
Track::setRating( int rating )
{
    Q_UNUSED( rating )
}

void
Track::setFirstPlayed( const QDateTime &firstPlayed )
{
    Q_UNUSED( firstPlayed )
}

void
Track::setLastPlayed( const QDateTime &lastPlayed )
{
    Q_UNUSED( lastPlayed )
}

int
Track::recentPlayCount() const
{
    return 0;
}

void
Track::setPlayCount( int playCount )
{
    Q_UNUSED( playCount )
}

void
Track::setLabels(const QSet<QString> &labels)
{
    Q_UNUSED( labels )
}

Meta::TrackPtr
Track::metaTrack() const
{
    return Meta::TrackPtr();
}

void
Track::commit()
{
}
