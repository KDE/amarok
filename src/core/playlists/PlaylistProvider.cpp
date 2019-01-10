/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "core/meta/Meta.h"
#include "core/playlists/PlaylistProvider.h"

using namespace Playlists;

PlaylistProvider::PlaylistProvider( QObject *parent)
    : QObject( parent )
{
}

QActionList
PlaylistProvider::providerActions()
{
    return QActionList();
}

QActionList
PlaylistProvider::playlistActions( const PlaylistList & )
{
    return QActionList();

}

QActionList
PlaylistProvider::trackActions( const QMultiHash<PlaylistPtr, int> & )
{
    return QActionList();
}

bool
PlaylistProvider::isWritable()
{
    return false;
}

PlaylistPtr
PlaylistProvider::addPlaylist(PlaylistPtr)
{
    return PlaylistPtr();
}

void
PlaylistProvider::renamePlaylist(PlaylistPtr, const QString & )
{
}

bool
PlaylistProvider::deletePlaylists( const PlaylistList & )
{
    return false;
}

Meta::TrackPtr
PlaylistProvider::addTrack( const Meta::TrackPtr &)
{
    return Meta::TrackPtr();
}
