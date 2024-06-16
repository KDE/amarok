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

#include "KConfigSyncRelStore.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "playlistmanager/SyncedPlaylist.h"

#include <KConfigGroup>

#include <QString>

using namespace Playlists;

KConfigSyncRelStore::KConfigSyncRelStore()
{
    DEBUG_BLOCK

    for( const QString &key : syncedPlaylistsConfig().keyList() )
    {
        QUrl masterUrl( key );

        m_syncMasterMap.insert( masterUrl, SyncedPlaylistPtr() );

        for( const QString &value : syncedPlaylistsConfig().readEntry( key ).split( QLatin1Char(',') ) )
        {
            m_syncSlaveMap.insert( QUrl( value ), masterUrl );
        }
    }
}

KConfigSyncRelStore::~KConfigSyncRelStore()
{
}

bool
KConfigSyncRelStore::hasToSync( Playlists::PlaylistPtr master, Playlists::PlaylistPtr slave ) const
{
    return m_syncSlaveMap.values( slave->uidUrl() ).contains( master->uidUrl() );
}

SyncedPlaylistPtr
KConfigSyncRelStore::asSyncedPlaylist( const PlaylistPtr playlist )
{
    DEBUG_BLOCK

    debug() << QStringLiteral("UIDurl: %1").arg( playlist->uidUrl().url() );

    SyncedPlaylistPtr syncedPlaylist;

    if( m_syncMasterMap.keys().contains( playlist->uidUrl() ) )
    {
        syncedPlaylist = m_syncMasterMap.value( playlist->uidUrl() );

        if( syncedPlaylist )
            syncedPlaylist->addPlaylist( playlist );
        else
        {
            syncedPlaylist = createSyncedPlaylist( playlist );
            m_syncMasterMap.insert( playlist->uidUrl(), syncedPlaylist );
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
KConfigSyncRelStore::syncedPlaylistsConfig() const
{
    return Amarok::config( QStringLiteral("Synchronized Playlists") );
}

void
KConfigSyncRelStore::addSync( const PlaylistPtr master, const PlaylistPtr slave )
{
    QUrl masterUrl( master->uidUrl() );

    if ( m_syncMasterMap.contains( masterUrl ) )
        m_syncSlaveMap.insert( slave->uidUrl(), masterUrl );
    else
    {
        m_syncMasterMap.insert( masterUrl, SyncedPlaylistPtr() );
        m_syncSlaveMap.insert( slave->uidUrl(), masterUrl );
    }

    QList<QString> slaveUrlStringList;

    for( const QUrl &slaveUrl : m_syncSlaveMap.keys() )
    {
        if( m_syncSlaveMap.value( slaveUrl ) == masterUrl )
            slaveUrlStringList.append( slaveUrl.url() );
    }

    syncedPlaylistsConfig().writeEntry( masterUrl.url(), slaveUrlStringList );
}
