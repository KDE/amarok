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

#include "Playlist.h"


Meta::PlaylistObserver::~PlaylistObserver()
{
    foreach( Meta::PlaylistPtr playlist, m_playlistSubscriptions )
    {
        playlist->unsubscribe( this );
    }
}

void
Meta::PlaylistObserver::subscribeTo( Meta::PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.insert( playlist );
        playlist->subscribe( this );
    }
}

void
Meta::PlaylistObserver::unsubscribeFrom( Meta::PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.remove( playlist );
        playlist->unsubscribe( this );
    }
}
