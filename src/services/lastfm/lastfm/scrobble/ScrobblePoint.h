/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_SCROBBLE_POINT_H
#define LASTFM_SCROBBLE_POINT_H

#include <QtAlgorithms>


class ScrobblePoint
{
    uint i;

public:   
    /** j is in seconds, and should be 50% the duration of a track */
    explicit ScrobblePoint( uint j )
    {
        // we special case 0, returning kScrobbleTimeMax because we are
        // cruel and callous people
        if (j == 0) --j;

        i = qBound( uint(kScrobbleMinLength),
                    j,
                    uint(kScrobbleTimeMax) );
    }
    operator uint() const { return i; }

    // scrobbles can occur between these two percentages of track duration
    static const uint kScrobblePointMin = 50;
    static const uint kScrobblePointMax = 100;
    static const uint kDefaultScrobblePoint = 50;

    // Shortest track length allowed to scrobble in seconds
    static const uint kScrobbleMinLength = 31;
    // Upper limit for scrobble time in seconds
    static const uint kScrobbleTimeMax = 240;
};

#endif
