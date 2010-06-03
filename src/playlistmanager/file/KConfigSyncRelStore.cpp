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
#include "KConfigSyncRelStore.h"

#include <src/core/support/Amarok.h>
#include <src/core/support/Debug.h>

#include <KConfigGroup>

using namespace Playlists;

KConfigSyncRelStore::KConfigSyncRelStore()
{
    DEBUG_BLOCK

    foreach( QString key, syncedPlaylistsConfig().keyList() )
    {
        debug() << "master: " << key;
        KUrl masterUrl( key );
        m_syncMasterMap.insert( masterUrl, SyncedPlaylistPtr() );
        foreach( QString value, syncedPlaylistsConfig().readEntry( key ).split( ',' ) )
        {
            m_syncSlaveMap.insert( KUrl( value ), masterUrl );
            debug() << "\tslave" << value;
        }
    }

}

KConfigSyncRelStore::~KConfigSyncRelStore()
{
}

bool
KConfigSyncRelStore::shouldBeSynced( PlaylistPtr playlist )
{
    DEBUG_BLOCK
    debug() << playlist->uidUrl().url();
    return m_syncMasterMap.keys().contains( playlist->uidUrl() )
           || m_syncSlaveMap.keys().contains( playlist->uidUrl() );
}

SyncedPlaylistPtr
KConfigSyncRelStore::asSyncedPlaylist( PlaylistPtr playlist )
{
    DEBUG_BLOCK
    debug() << playlist->uidUrl().url();
    SyncedPlaylistPtr syncedPlaylist;
    if( m_syncMasterMap.keys().contains( playlist->uidUrl() ) )
    {
        syncedPlaylist = m_syncMasterMap.value( playlist->uidUrl() );
        if( !syncedPlaylist )
        {
            syncedPlaylist = SyncedPlaylistPtr( new SyncedPlaylist( playlist ) );;
            m_syncMasterMap.insert( playlist->uidUrl(), syncedPlaylist );
            return syncedPlaylist;
        }
    }
    else if( m_syncSlaveMap.keys().contains( playlist->uidUrl() ) )
    {
        syncedPlaylist = m_syncMasterMap.value( m_syncSlaveMap.value( playlist->uidUrl() ) );
        if( syncedPlaylist )
            syncedPlaylist->addPlaylist( playlist );
    }

    return syncedPlaylist;
}

inline KConfigGroup
KConfigSyncRelStore::syncedPlaylistsConfig() const {
    return Amarok::config( "Synchronized Playlists" );
}
