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

#include "browsers/playlistbrowser/APGCategory.h"
#include "browsers/playlistbrowser/DynamicCategory.h"
#include "browsers/playlistbrowser/UserPlaylistCategory.h"
#include "core/playlists/Playlist.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "playlistmanager/PlaylistManager.h"

#include <QStandardPaths>
#include <QString>

#include <KLocalizedString>

PlaylistBrowserNS::PlaylistBrowser::PlaylistBrowser( const QString &name, QWidget *parent )
    : BrowserCategoryList( name, parent )
{
    setContentsMargins(0,0,0,0);

    addCategory( new DynamicCategory( nullptr ) );
    addCategory( new UserPlaylistCategory( nullptr ) );
    addCategory( new APGCategory( nullptr ) );

    setLongDescription( i18n( "The playlist browser contains your list of imported and saved playlists. It is also where you can specify powerful dynamic playlists and manage your podcast subscriptions and episodes." ) );

    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_playlists.png") ) );
}

PlaylistBrowserNS::PlaylistBrowser::~PlaylistBrowser()
{
}

