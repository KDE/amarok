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

#include "Amarok.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "MediaDeviceCollection.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/XSPFPlaylist.h"
#include "PlaylistFileSupport.h"
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

MediaDeviceUserPlaylistProvider::MediaDeviceUserPlaylistProvider( MediaDeviceCollection *collection )
    : UserPlaylistProvider()
    , m_renameAction( 0 )
    , m_collection( collection )
{
    DEBUG_BLOCK
//    checkTables();
//    m_root = Meta::MediaDevicePlaylistGroupPtr( new Meta::MediaDevicePlaylistGroup( "",
//            Meta::MediaDevicePlaylistGroupPtr() ) );
//    The::playlistManager()->addProvider( this, category() );
}

MediaDeviceUserPlaylistProvider::~MediaDeviceUserPlaylistProvider()
{
    DEBUG_BLOCK
//     foreach( Meta::MediaDevicePlaylistPtr playlist, m_playlists )
//     {
//         playlist->saveToDb( true );
//     }
    m_playlists.clear();
//    emit updated();
//    The::playlistManager()->removeProvider( this );

}

Meta::PlaylistList
MediaDeviceUserPlaylistProvider::playlists()
{
    DEBUG_BLOCK
    Meta::PlaylistList playlists;

    foreach( Meta::MediaDevicePlaylistPtr mediadevicePlaylist, m_playlists )
    {
        playlists << Meta::PlaylistPtr::staticCast( mediadevicePlaylist );
    }

    return playlists;
}
#if 0
void
SqlUserPlaylistProvider::slotDelete()
{
    DEBUG_BLOCK

    //TODO FIXME Confirmation of delete
    foreach( Meta::PlaylistPtr playlist, The::userPlaylistModel()->selectedPlaylists() )
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
    Meta::MediaDevicePlaylistPtr playlist = selectedPlaylists().first();
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
SqlUserPlaylistProvider::slotRemove()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    foreach( Meta::PlaylistPtr playlist, playlistMap.keys() )
        foreach( Meta::TrackPtr track, playlistMap.values( playlist ) )
            playlist->removeTrack( playlist->tracks().indexOf( track ) );

    //clear the data
    action->setData( QVariant() );
}
#endif
Meta::PlaylistPtr
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

Meta::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to device with name" << name;
    // NOTE: the playlist constructor tells the handler to make the playlist, save to db etc.
    Meta::MediaDevicePlaylistPtr pl = Meta::MediaDevicePlaylistPtr( new Meta::MediaDevicePlaylist( name, tracks ) );
    //pl = 0;

    emit playlistSaved( pl, name ); // inform handler of new playlist

    addPlaylist( pl );

    return Meta::PlaylistPtr::dynamicCast( pl );
}

void
MediaDeviceUserPlaylistProvider::rename( Meta::PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    Meta::MediaDevicePlaylistPtr pl = Meta::MediaDevicePlaylistPtr::staticCast( playlist );
    if( pl )
    {
        debug() << "Setting name of playlist";
        pl->setName( newName );

        emit playlistRenamed( pl );
    }
}

void
MediaDeviceUserPlaylistProvider::deletePlaylists( Meta::PlaylistList playlistlist )
{
    Meta::MediaDevicePlaylistList pllist;
    foreach( Meta::PlaylistPtr playlist, playlistlist )
    {
        Meta::MediaDevicePlaylistPtr pl = Meta::MediaDevicePlaylistPtr::staticCast( playlist );

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
MediaDeviceUserPlaylistProvider::addPlaylist( Meta::MediaDevicePlaylistPtr &playlist )
{
    m_playlists << playlist;
    emit updated();
}

void
MediaDeviceUserPlaylistProvider::removePlaylist( Meta::MediaDevicePlaylistPtr &playlist )
{
    m_playlists.removeOne( playlist );
    emit updated();
}

#include "MediaDeviceUserPlaylistProvider.moc"
