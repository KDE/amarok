/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "MediaDeviceUserPlaylistProvider.h"

#include "SvgHandler.h"
#include "browsers/playlistbrowser/UserPlaylistModel.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"
#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"
#include "playlistmanager/PlaylistManager.h"

#include <QIcon>
#include <QInputDialog>
#include <QUrl>

#include <QMap>

// static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key(QStringLiteral("AMAROK_USERPLAYLIST"));

namespace Playlists {

MediaDeviceUserPlaylistProvider::MediaDeviceUserPlaylistProvider( Collections::MediaDeviceCollection *collection )
    : Playlists::UserPlaylistProvider()
    , m_collection( collection )
{
    DEBUG_BLOCK
//    checkTables();
//    m_root = Playlists::MediaDevicePlaylistGroupPtr( new Playlists::MediaDevicePlaylistGroup( "",
//            Playlists::MediaDevicePlaylistGroupPtr() ) );
//    The::playlistManager()->addProvider( this, category() );
}

MediaDeviceUserPlaylistProvider::~MediaDeviceUserPlaylistProvider()
{
    DEBUG_BLOCK
//     for( Playlists::MediaDevicePlaylistPtr playlist : m_playlists )
//     {
//         playlist->saveToDb( true );
//     }
    m_playlists.clear();
//    Q_EMIT updated();
//    The::playlistManager()->removeProvider( this );

}

Playlists::PlaylistList
MediaDeviceUserPlaylistProvider::playlists()
{
    DEBUG_BLOCK
    Playlists::PlaylistList playlists;

    for( Playlists::MediaDevicePlaylistPtr mediadevicePlaylist : m_playlists )
    {
        playlists << Playlists::PlaylistPtr::staticCast( mediadevicePlaylist );
    }

    return playlists;
}

Playlists::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    // This provider can only save it's own tracks for now, filter out all the others.
    Meta::TrackList filteredTracks;
    for( const Meta::TrackPtr &track : tracks )
        if( track->collection() == m_collection )
            filteredTracks << track;

    return save( filteredTracks,
                 QDateTime::currentDateTime().toString( QStringLiteral("ddd MMMM d yy hh-mm") ) );
}

Playlists::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to device with name" << name;
    // NOTE: the playlist constructor tells the handler to make the playlist, save to db etc.
    Playlists::MediaDevicePlaylistPtr pl = Playlists::MediaDevicePlaylistPtr( new Playlists::MediaDevicePlaylist( name, tracks ) );
    //pl = 0;

    Q_EMIT playlistSaved( pl, name ); // inform handler of new playlist

    addMediaDevicePlaylist( pl );

    return Playlists::PlaylistPtr::dynamicCast( pl );
}

void
MediaDeviceUserPlaylistProvider::renamePlaylist(PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    Playlists::MediaDevicePlaylistPtr pl = Playlists::MediaDevicePlaylistPtr::staticCast( playlist );
    if( pl )
    {
        debug() << "Setting name of playlist";
        pl->setName( newName );

        Q_EMIT playlistRenamed( pl );
    }
}

bool
MediaDeviceUserPlaylistProvider::deletePlaylists( const Playlists::PlaylistList &playlistlist )
{
    Playlists::MediaDevicePlaylistList pllist;
    for( Playlists::PlaylistPtr playlist : playlistlist )
    {
        Playlists::MediaDevicePlaylistPtr pl =
                Playlists::MediaDevicePlaylistPtr::staticCast( playlist );

        if( pl )
        {
            debug() << "Deleting playlist: " << pl->name();
            removePlaylist( pl );
            pllist << pl;
        }
    }

    Q_EMIT playlistsDeleted( pllist );

    return true;
}

void
MediaDeviceUserPlaylistProvider::addMediaDevicePlaylist( Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_playlists << playlist;
    Q_EMIT updated();
}

void
MediaDeviceUserPlaylistProvider::removePlaylist( Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_playlists.removeOne( playlist );
    Q_EMIT updated();
}

} //namespace Playlists

