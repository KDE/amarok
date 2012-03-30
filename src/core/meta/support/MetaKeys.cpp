/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "core/meta/support/MetaKeys.h"

#include "core/meta/Meta.h"

Meta::AlbumKey::AlbumKey()
{
}

Meta::AlbumKey::AlbumKey( const QString &name, const QString &artistName )
{
    m_albumName = name;
    m_artistName = artistName;
}

Meta::AlbumKey::AlbumKey( const Meta::AlbumPtr &album )
{
    m_albumName = album->name();
    if( album->hasAlbumArtist() && album->albumArtist() )
        m_artistName = album->albumArtist()->name();
}

Meta::AlbumKey&
Meta::AlbumKey::operator=( const Meta::AlbumKey &o )
{
    m_albumName = o.m_albumName;
    m_artistName = o.m_artistName;
    return *this;
}

bool
Meta::AlbumKey::operator<( const Meta::AlbumKey &other ) const
{
    // sort first by artist name, then by album name
    if( m_artistName == other.m_artistName )
        return m_albumName < other.m_albumName;
    return m_artistName < other.m_artistName;
}

Meta::TrackKey::TrackKey()
{
}

Meta::TrackKey::TrackKey( const Meta::TrackPtr &track )
{
    m_trackName = track->name();
    m_discNumber = track->discNumber();
    m_trackNumber = track->trackNumber();
    if( track->artist() )
        m_artistName = track->artist()->name();

    if( track->album() )
        m_albumName = track->album()->name();
}

Meta::TrackKey& Meta::TrackKey::operator=( const Meta::TrackKey &o )
{
    m_discNumber = o.m_discNumber;
    m_trackNumber = o.m_trackNumber;
    m_trackName = o.m_trackName;
    m_albumName = o.m_albumName;
    m_artistName = o.m_artistName;
    return *this;
}

