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

#include "TrackDelegate.h"

using namespace StatSyncing;

TrackDelegate::TrackDelegate()
    : QSharedData()
{
}

TrackDelegate::~TrackDelegate()
{
}

bool
TrackDelegate::operator==( const StatSyncing::TrackDelegate &other ) const
{
    if( name() != other.name() )
        return false;
    if( album() != other.album() )
        return false;
    if( artist() != other.artist() )
        return false;
    if( composer() != other.composer() && !composer().isEmpty() && !other.composer().isEmpty() )
        return false;
    if( year() != other.year() && year() > 0 && other.year() > 0 )
        return false;
    if( trackNumber() != other.trackNumber() && trackNumber() > 0 && other.trackNumber() > 0 )
        return false;
    if( discNumber() != other.discNumber() && discNumber() > 0 && other.discNumber() > 0 )
        return false;
    return true;
}

bool
TrackDelegate::operator<( const StatSyncing::TrackDelegate &other ) const
{
    // precedence: artist > album > name
    if( artist() != other.artist() )
        return artist() < other.artist();
    if( album() != other.album() )
        return album() < other.album();
    if( name() != other.name() )
        return name() < other.name();
    return false;
}
