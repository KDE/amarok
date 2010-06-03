/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include <src/playlistmanager/SyncRelationStorage.h>

class KConfigGroup;

class KConfigSyncRelStore : public SyncRelationStorage
{
public:
    KConfigSyncRelStore();

    ~KConfigSyncRelStore();

    virtual bool shouldBeSynced( Playlists::PlaylistPtr playlist );
    virtual SyncedPlaylistPtr asSyncedPlaylist( Playlists::PlaylistPtr playlist );
private:
    KConfigGroup syncedPlaylistsConfig() const;

    QMap<KUrl,SyncedPlaylistPtr> m_syncMasterMap;
    QMap<KUrl,KUrl> m_syncSlaveMap;
};

#endif // KCONFIGSYNCRELSTORE_H
