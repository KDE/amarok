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

#include "core/playlists/Playlist.h"


Playlists::PlaylistObserver::~PlaylistObserver()
{
    foreach( Playlists::PlaylistPtr playlist, m_playlistSubscriptions )
    {
        playlist->unsubscribe( this );
    }
}

void
Playlists::PlaylistObserver::subscribeTo( Playlists::PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.insert( playlist );
        playlist->subscribe( this );
    }
}

void
Playlists::PlaylistObserver::unsubscribeFrom( Playlists::PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.remove( playlist );
        playlist->unsubscribe( this );
    }
}
