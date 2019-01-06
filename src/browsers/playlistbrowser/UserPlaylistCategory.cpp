/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "UserPlaylistCategory.h"

#include "UserPlaylistModel.h"

#include <QStandardPaths>

#include <KLocalizedString>


using namespace PlaylistBrowserNS;

QString UserPlaylistCategory::s_configGroup( QStringLiteral("Saved Playlists View") );

UserPlaylistCategory::UserPlaylistCategory( QWidget * parent )
    : PlaylistBrowserCategory( Playlists::UserPlaylist,
                               QStringLiteral("user playlists"), s_configGroup,
                               The::userPlaylistModel(), parent )
{
    setPrettyName( i18n( "Saved Playlists" ) );
    setShortDescription( i18n( "User generated and imported playlists" ) );
    setIcon( QIcon::fromTheme( QStringLiteral("amarok_playlist") ) );

    setLongDescription( i18n( "Create, edit, organize and load playlists. "
        "Amarok automatically adds any playlists found when scanning your collection, "
        "and any playlists that you save are also shown here." ) );

    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_user_playlists.png") ) );
}

UserPlaylistCategory::~UserPlaylistCategory()
{
}
