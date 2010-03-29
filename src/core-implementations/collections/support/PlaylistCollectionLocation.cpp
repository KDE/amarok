/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "PlaylistCollectionLocation.h"

#include "core/playlists/Playlist.h"
#include "PlaylistCollection.h"

PlaylistCollectionLocation::PlaylistCollectionLocation( const Collections::PlaylistCollection *collection )
        : CollectionLocation()
        , m_collection( collection )
{
}

QString
PlaylistCollectionLocation::prettyLocation() const
{
    //think of something better
    return m_collection->prettyName();
}

bool
PlaylistCollectionLocation::isWritable() const
{
    return true;
}

bool
PlaylistCollectionLocation::remove( const Meta::TrackPtr &track )
{
    Playlists::PlaylistPtr playlist = m_collection->playlist();
    int index = playlist->tracks().indexOf( track );
    if( index != -1 )
    {
        playlist->removeTrack( index );
    }
    //always succeed as we are not doing anything dangerous
    return true;
}

void
PlaylistCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    Playlists::PlaylistPtr playlist = m_collection->playlist();
    foreach( const Meta::TrackPtr &track, sources.keys() )
    {
        playlist->addTrack( track );
    }
}

void
PlaylistCollectionLocation::removeUrlsFromCollection( const Meta::TrackList &tracks )
{
    foreach( const Meta::TrackPtr &track, tracks )
    {
        remove( track );
    }
}
