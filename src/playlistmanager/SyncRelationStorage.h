/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#ifndef SYNCRELATIONSTORAGE_H
#define SYNCRELATIONSTORAGE_H

#include "SyncedPlaylist.h"
#include "SyncedPodcast.h"

class SyncRelationStorage
{
public:
    SyncRelationStorage();
    virtual ~SyncRelationStorage() {}

    virtual void addSync( const Playlists::PlaylistPtr master, const Playlists::PlaylistPtr slave ) = 0;
    virtual bool hasToSync( Playlists::PlaylistPtr master, Playlists::PlaylistPtr slave ) const = 0;
    virtual SyncedPlaylistPtr asSyncedPlaylist( const Playlists::PlaylistPtr playlist ) = 0;

protected:
    SyncedPlaylistPtr createSyncedPlaylist( const Playlists::PlaylistPtr playlist );
};

#endif // SYNCRELATIONSTORAGE_H
