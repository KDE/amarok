/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukAlbum.h"

#include "core/meta/Meta.h"

using namespace Meta;

NepomukAlbum::NepomukAlbum( const QString &albumName, const ArtistPtr &artistPtr )
    : Meta::Album()
    , m_name( albumName )
    , m_artist( artistPtr )
{
    // TODO, check if album has unique artist
    m_isCompilation = false;
}

TrackList
NepomukAlbum::tracks()
{
    return TrackList();
}

bool
NepomukAlbum::isCompilation() const
{
    return m_isCompilation;
}


bool
NepomukAlbum::hasAlbumArtist() const
{
    return m_artist;
}

ArtistPtr
NepomukAlbum::albumArtist() const
{
    return m_artist;
}

QString
NepomukAlbum::name() const
{
    return m_name;
}
