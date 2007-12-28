/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PlaylistFileProvider.h"
#include "amarok.h"
#include "debug.h"

#include <QString>

#include <KConfigGroup>
#include <KLocale>
#include <KUrl>

PlaylistFileProvider::PlaylistFileProvider()
 : PlaylistProvider()
{
    //load the playlists defined in the config
    KConfigGroup userPlaylistConfig = Amarok::config( "Loaded Playlist Files" );
    QStringList keys = userPlaylistConfig.keyList();
    debug() << "keys " << keys;

    //ConfigEntry: name, type, key_for_type
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    //Write loaded playlists to
    KConfigGroup userPlaylistConfig = Amarok::config( "Loaded Playlist Files" );
}

QString
PlaylistFileProvider::prettyName() const
{
    return i18n("Playlist Files");
}

Meta::PlaylistList
PlaylistFileProvider::playlists()
{
    return m_playlists;
}
