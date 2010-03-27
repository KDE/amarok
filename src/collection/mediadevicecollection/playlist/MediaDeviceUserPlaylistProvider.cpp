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

#include "core/support/Amarok.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "MediaDeviceCollection.h"
#include "core-implementations/playlists/file/m3u/M3UPlaylist.h"
#include "core-implementations/playlists/file/pls/PLSPlaylist.h"
#include "core-implementations/playlists/file/xspf/XSPFPlaylist.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"
#include "PlaylistManager.h"
//#include "MediaDeviceStorage.h"
#include "SvgHandler.h"
#include "UserPlaylistModel.h"

#include <KIcon>
#include <KInputDialog>
#include <KUrl>

#include <QAction>
#include <QMap>

static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key("AMAROK_USERPLAYLIST");

namespace Playlists {

MediaDeviceUserPlaylistProvider::MediaDeviceUserPlaylistProvider( MediaDeviceCollection *collection )
    : Playlists::UserPlaylistProvider()
    , m_renameAction( 0 )
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
//     foreach( Playlists::MediaDevicePlaylistPtr playlist, m_playlists )
//     {
//         playlist->saveToDb( true );
//     }
    m_playlists.clear();
//    emit updated();
//    The::playlistManager()->removeProvider( this );

}

Playlists::PlaylistList
MediaDeviceUserPlaylistProvider::playlists()
{
    DEBUG_BLOCK
    Playlists::PlaylistList playlists;

    foreach( Playlists::MediaDevicePlaylistPtr mediadevicePlaylist, m_playlists )
    {
        playlists << Playlists::PlaylistPtr::staticCast( mediadevicePlaylist );
    }

    return playlists;
}
#if 0
void
SqlPlaylists::UserPlaylistProvider::slotDelete()
{
    DEBUG_BLOCK

    //TODO FIXME Confirmation of delete
    foreach( Playlists::PlaylistPtr playlist, The::userPlaylistModel()->selectedPlaylists() )
    {
        Meta::SqlPlaylistPtr sqlPlaylist =
                Meta::SqlPlaylistPtr::dynamicCast( playlist );
        if( sqlPlaylist )
        {
            debug() << "deleting " << sqlPlaylist->name();
            sqlPlaylist->removeFromDb();
        }
    }
    reloadFromDb();
}
#endif

#if 0
void
MediaDeviceUserPlaylistProvider::slotRename()
{
    DEBUG_BLOCK
    //only one playlist can be selected at this point
    Playlists::MediaDevicePlaylistPtr playlist = selectedPlaylists().first();
    if( playlist.isNull() )
        return;

    bool ok;
    const QString newName = KInputDialog::getText( i18n("Change playlist"),
                i18n("Enter new name for playlist:"), playlist->name(),
                                                   &ok );
    if ( ok )
    {
        playlist->setName( newName.trimmed() );
        emit( updated() );
    }
}
#endif
#if 0
void
SqlPlaylists::UserPlaylistProvider::slotRemove()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    foreach( Playlists::PlaylistPtr playlist, playlistMap.keys() )
        foreach( Meta::TrackPtr track, playlistMap.values( playlist ) )
            playlist->removeTrack( playlist->tracks().indexOf( track ) );

    //clear the data
    action->setData( QVariant() );
}
#endif
Playlists::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    // This provider can only save it's own tracks for now, filter out all the others.
    Meta::TrackList filteredTracks;
    foreach( const Meta::TrackPtr track, tracks )
        if( track->collection() == m_collection )
            filteredTracks << track;

    return save( filteredTracks,
                 QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm" ) );
}

Playlists::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to device with name" << name;
    // NOTE: the playlist constructor tells the handler to make the playlist, save to db etc.
    Playlists::MediaDevicePlaylistPtr pl = Playlists::MediaDevicePlaylistPtr( new Playlists::MediaDevicePlaylist( name, tracks ) );
    //pl = 0;

    emit playlistSaved( pl, name ); // inform handler of new playlist

    addPlaylist( pl );

    return Playlists::PlaylistPtr::dynamicCast( pl );
}

void
MediaDeviceUserPlaylistProvider::rename( Playlists::PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    Playlists::MediaDevicePlaylistPtr pl = Playlists::MediaDevicePlaylistPtr::staticCast( playlist );
    if( pl )
    {
        debug() << "Setting name of playlist";
        pl->setName( newName );

        emit playlistRenamed( pl );
    }
}

void
MediaDeviceUserPlaylistProvider::deletePlaylists( Playlists::PlaylistList playlistlist )
{
    Playlists::MediaDevicePlaylistList pllist;
    foreach( Playlists::PlaylistPtr playlist, playlistlist )
    {
        Playlists::MediaDevicePlaylistPtr pl = Playlists::MediaDevicePlaylistPtr::staticCast( playlist );

        if( pl )
        {
            debug() << "Deleting playlist: " << pl->name();
            removePlaylist( pl );
            pllist << pl;
        }
    }

    emit playlistsDeleted( pllist );
}

void
MediaDeviceUserPlaylistProvider::addPlaylist( Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_playlists << playlist;
    emit updated();
}

void
MediaDeviceUserPlaylistProvider::removePlaylist( Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_playlists.removeOne( playlist );
    emit updated();
}

} //namespace Playlists

#include "MediaDeviceUserPlaylistProvider.moc"
