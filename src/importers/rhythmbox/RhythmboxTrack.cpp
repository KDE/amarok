/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "RhythmboxTrack.h"

using namespace StatSyncing;

RhythmboxTrack::RhythmboxTrack( const QString &location, const Meta::FieldHash &metadata )
    : SimpleWritableTrack( metadata )
    , m_location( location )
{
}

RhythmboxTrack::~RhythmboxTrack()
{
}

int
RhythmboxTrack::rating() const
{
    return SimpleWritableTrack::rating() * 2;
}

void
RhythmboxTrack::setLastPlayed( const QDateTime &lastPlayed )
{
    SimpleWritableTrack::setLastPlayed( lastPlayed );
    emit trackUpdated( m_location, Meta::valLastPlayed, lastPlayed.toTime_t() );
}

void
RhythmboxTrack::setPlayCount( int playCount )
{
    SimpleWritableTrack::setPlayCount( playCount );
    emit trackUpdated( m_location, Meta::valPlaycount, playCount );
}

void
RhythmboxTrack::setRating( int rating )
{
    rating = (rating + 1)/2;
    SimpleWritableTrack::setRating( rating );
    emit trackUpdated( m_location, Meta::valRating, rating );
}

void
RhythmboxTrack::doCommit( const QSet<qint64> &fields )
{
    Q_UNUSED( fields );
    emit commitCalled();
}
