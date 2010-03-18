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

#include "PlaylistBrowser.h"

#include "Amarok.h"
#include "Debug.h"
#include "DynamicCategory.h"
#include "Playlist.h"
#include "UserPlaylistCategory.h"
#include "PlaylistManager.h"

#include <klocale.h>
#include <KStandardDirs>

#include <QList>
#include <QString>

PlaylistBrowserNS::PlaylistBrowser::PlaylistBrowser( const char *name, QWidget *parent )
 : BrowserCategoryList( parent, name )
{
    DEBUG_BLOCK

    setObjectName( name );

    setMargin( 0 );
    setContentsMargins(0,0,0,0);

    BrowserCategoryList::addCategory( new DynamicCategory( 0 ) );
    BrowserCategoryList::addCategory( new UserPlaylistCategory( 0 ) );

    connect( The::playlistManager(), SIGNAL( categoryAdded( int ) ), SLOT( addCategory( int ) ) );

    setLongDescription( i18n( "The playlist browser contains your list of imported and saved playlists. It is also where you can specify powerful dynamic playlists and manage your podcast subscriptions and episodes." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_playlists.png" ) );
}

PlaylistBrowserNS::PlaylistBrowser::~PlaylistBrowser()
{
}

//SLOT
void
PlaylistBrowserNS::PlaylistBrowser::addCategory( int )
{
    DEBUG_BLOCK
}

#include "PlaylistBrowser.moc"
