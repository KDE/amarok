/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "ServiceDynamicCollection.h"

ServiceDynamicCollection::ServiceDynamicCollection( const QString &id, const QString &prettyName )
    : ServiceCollection()
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}


ServiceDynamicCollection::~ServiceDynamicCollection()
{
}




void ServiceDynamicCollection::addTrack(QString key, Meta::TrackPtr trackPtr)
{
    MemoryCollection::addTrack( key, trackPtr );
    const Meta::ServiceTrack * track = static_cast< const Meta::ServiceTrack * >( trackPtr.data() );
    
    if ( track->id() != 0 )
        m_trackIdMap.insert( track->id(), trackPtr );
}

void ServiceDynamicCollection::addArtist(QString key, Meta::ArtistPtr artistPtr)
{
    MemoryCollection::addArtist( key, artistPtr );
    const Meta::ServiceArtist * artist = static_cast< const Meta::ServiceArtist* >( artistPtr.data() );
    
    if ( artist->id() != 0 )
        m_artistIdMap.insert( artist->id(), artistPtr );
}

void ServiceDynamicCollection::addAlbum(QString key, Meta::AlbumPtr albumPtr)
{
    MemoryCollection::addAlbum( key, albumPtr );
    const Meta::ServiceAlbum * album = static_cast< const Meta::ServiceAlbum* >( albumPtr.data() );
    
    if ( album->id() != 0 )
        m_albumIdMap.insert( album->id(), albumPtr );
}

void ServiceDynamicCollection::addGenre(QString key, Meta::GenrePtr genrePtr)
{
    MemoryCollection::addGenre( key, genrePtr );
    const Meta::ServiceGenre * genre = static_cast< const Meta::ServiceGenre * >( genrePtr.data() );
    
    if ( genre->id() != 0 )
        m_genreIdMap.insert( genre->id(), genrePtr );
}

Meta::TrackPtr ServiceDynamicCollection::trackById(int id)
{
    return m_trackIdMap.value( id );
}

Meta::AlbumPtr ServiceDynamicCollection::albumById(int id)
{
    return m_albumIdMap.value( id );
}

Meta::ArtistPtr ServiceDynamicCollection::artistById(int id)
{
    return m_artistIdMap.value( id );
}

Meta::GenrePtr ServiceDynamicCollection::genreById(int id)
{
    return m_genreIdMap.value( id );
}

QString ServiceDynamicCollection::collectionId() const
{
    return m_collectionId;
}

QString ServiceDynamicCollection::prettyName() const
{
    return m_prettyName;
}
