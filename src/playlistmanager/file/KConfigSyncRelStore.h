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
#ifndef KCONFIGSYNCRELSTORE_H
#define KCONFIGSYNCRELSTORE_H

#include "playlistmanager/SyncRelationStorage.h"

#include <QMap>

class KConfigGroup;

class KConfigSyncRelStore : public SyncRelationStorage
{
public:
    KConfigSyncRelStore();

    virtual ~KConfigSyncRelStore();

    virtual void addSync( const Playlists::PlaylistPtr master, const Playlists::PlaylistPtr slave );
    virtual bool hasToSync( Playlists::PlaylistPtr master, Playlists::PlaylistPtr slave ) const;
    virtual SyncedPlaylistPtr asSyncedPlaylist( const Playlists::PlaylistPtr playlist );

private:
    KConfigGroup syncedPlaylistsConfig() const;

    QMap<QUrl,SyncedPlaylistPtr> m_syncMasterMap;
    QMap<QUrl,QUrl> m_syncSlaveMap;
};

#endif // KCONFIGSYNCRELSTORE_H
