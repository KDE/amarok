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
RhythmboxTrack::setRating( int rating )
{
    SimpleWritableTrack::setRating( (rating + 1)/2 );
}

void
RhythmboxTrack::doCommit( const qint64 fields )
{
    Q_UNUSED( fields );
    Q_EMIT commitCalled( m_location, m_statistics );
}
