/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediaDevicePlaylist.h"

#include "CollectionManager.h"
#include "Debug.h"
//#include "meta/stream/Stream.h"
//#include "MediaDeviceStorage.h"
#include "playlistmanager/PlaylistManager.h"
#include "MediaDeviceUserPlaylistProvider.h"

#include <typeinfo>

Meta::MediaDevicePlaylist::MediaDevicePlaylist( const QString & name, const Meta::TrackList
        &tracks )
    : m_tracks( tracks )
    , m_description( QString() )
{
    m_name = name;
    // Tell the handler to save it
}

Meta::MediaDevicePlaylist::~MediaDevicePlaylist()
{
}

Meta::TrackList
Meta::MediaDevicePlaylist::tracks()
{
    return m_tracks;
}

void
Meta::MediaDevicePlaylist::addTrack( Meta::TrackPtr track, int position )
{
    DEBUG_BLOCK
    int insertAt = (position == -1) ? m_tracks.count() : position;
    m_tracks.insert( insertAt, track );
}

void
Meta::MediaDevicePlaylist::removeTrack( int position )
{
    DEBUG_BLOCK
    m_tracks.removeAt( position );
}

void
Meta::MediaDevicePlaylist::setName( const QString & name )
{
    m_name = name;
}

