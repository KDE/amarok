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
#include "core/playlists/PlaylistProvider.h"

using namespace Playlists;

PlaylistObserver::~PlaylistObserver()
{
    foreach( PlaylistPtr playlist, m_playlistSubscriptions )
    {
        playlist->unsubscribe( this );
    }
}

void
PlaylistObserver::subscribeTo( PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.insert( playlist );
        playlist->subscribe( this );
    }
}

void
PlaylistObserver::unsubscribeFrom( PlaylistPtr playlist )
{
    if( playlist )
    {
        m_playlistSubscriptions.remove( playlist );
        playlist->unsubscribe( this );
    }
}

Playlist::~Playlist()
{
}

void
Playlist::triggerTrackLoad()
{
}

QActionList
Playlist::actions()
{
    if( provider() )
        return provider()->playlistActions( PlaylistPtr( this ) );

    return QActionList();
}

QActionList
Playlist::trackActions( int trackIndex )
{
    if( provider() )
        return provider()->trackActions( PlaylistPtr( this ), trackIndex );
    return QActionList();
}

void
Playlist::notifyObserversMetadataChanged()
{
    foreach( PlaylistObserver *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( PlaylistPtr( this ) );
    }
}

void
Playlist::notifyObserversTrackAdded( const Meta::TrackPtr &track, int position )
{
    Q_ASSERT( position >= 0 ); // notice bug 293295 early
    foreach( PlaylistObserver *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->trackAdded( PlaylistPtr( this ), track, position );
    }
}

void
Playlist::notifyObserversTrackRemoved( int position )
{
    foreach( PlaylistObserver *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->trackRemoved( PlaylistPtr( this ), position );
    }
}

void
Playlist::subscribe( PlaylistObserver* observer )
{
    if( observer )
        m_observers.insert( observer );
}

void
Playlist::unsubscribe( PlaylistObserver* observer )
{
    m_observers.remove( observer );
}
