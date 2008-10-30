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

#ifndef LASTFM_SCROBBLE_H
#define LASTFM_SCROBBLE_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/types/Track.h>


struct LASTFM_SCROBBLE_DLLEXPORT Scrobble : Track
{
    Scrobble()
    {}
    
    Scrobble( const Track& that ) : Track( that )
    {}
    
    QByteArray sourceString() const;
    
    QByteArray ratingCharacter() const
    {
        return d->extras["rating"].toAscii();
    }
    
    bool isNotLovedSkippedOrBanned() const
    {
        return ratingCharacter() == "";
    }

    enum Invalidity
    {
        TooShort,
        ArtistNameMissing,
        TrackNameMissing,
        ArtistInvalid,

        NoTimestamp,
        FromTheFuture,
        FromTheDistantPast
    };
    
    /** @returns true if the server is unlikely to reject this scrobble */
    bool isValid( Invalidity* = 0 ) const;
};

#endif
