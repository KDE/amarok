/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "MediaDeviceUserPlaylistProvider.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "Debug.h"
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

#include <QMap>

static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key("AMAROK_USERPLAYLIST");

MediaDeviceUserPlaylistProvider::MediaDeviceUserPlaylistProvider()
    : UserPlaylistProvider()
//    , m_renameAction( 0 )
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

Meta::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    return save( tracks,
          QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") );

}

Meta::PlaylistPtr
MediaDeviceUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    Q_UNUSED( tracks )
    Q_UNUSED( name )
    DEBUG_BLOCK
//    debug() << "saving " << tracks.count() << " tracks to db with name" << name;
        Meta::MediaDevicePlaylistPtr mediadevicePlaylist;
    mediadevicePlaylist = 0;
//    reloadFromDb();
//    emit updated();

    return Meta::PlaylistPtr::dynamicCast( mediadevicePlaylist ); //assumes insertion in db was successful!
}
#if 0
QList<PopupDropperAction *>
MediaDeviceUserPlaylistProvider::playlistActions( Meta::PlaylistList list )
{
    Q_UNUSED( list )
    QList<PopupDropperAction *> actions;

    if ( m_renameAction == 0 )
    {
        m_renameAction =  new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "edit", KIcon( "media-track-edit-amarok" ), i18n( "&Rename" ), this );
        connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRename() ) );
    }
    actions << m_renameAction;


    return actions;
}
#endif
void
MediaDeviceUserPlaylistProvider::addPlaylist( Meta::MediaDevicePlaylistPtr &playlist )
{
    m_playlists << playlist;
    emit updated();
}

#include "MediaDeviceUserPlaylistProvider.moc"
