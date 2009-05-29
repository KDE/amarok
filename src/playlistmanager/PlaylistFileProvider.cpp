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
#include "PlaylistFileSupport.h"
#include "EditablePlaylistCapability.h"
#include "Amarok.h"
#include "Debug.h"

#include <QString>

#include <KLocale>
#include <KUrl>

PlaylistFileProvider::PlaylistFileProvider()
 : PlaylistProvider()
{
    //load the playlists defined in the config
    QStringList keys = Amarok::config( "Loaded Playlist Files" ).keyList();
    debug() << "keys " << keys;

    //ConfigEntry: name, file
    foreach( const QString &key, keys )
    {
        QStringList configEntry = Amarok::config( "Loaded Playlist Files" ).readXdgListEntry( key );
        Meta::PlaylistPtr playlist = Meta::loadPlaylist( KUrl( configEntry[1] ).path() );
        //TODO: make this work
        if( playlist->is<Meta::EditablePlaylistCapability>() )
        {
            QString title = configEntry[0];
            playlist->create<Meta::EditablePlaylistCapability>()->setTitle( title );
        }
        m_playlists << playlist;
    }
    if( m_playlists.isEmpty() )
    {
        //TODO: find playlist files in the configured collection directories and home folder
    }
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //Write loaded playlists to config file
    debug() << m_playlists.size()  << " Playlists loaded";
    int i = 0;
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        QStringList configEntry;
        KUrl url = playlist->retrievableUrl();
        debug() << "storing: " << playlist->name() << " : " << url.url();

        configEntry << playlist->name();
        configEntry << url.url();

        Amarok::config( "Loaded Playlist Files" ).writeXdgListEntry(
                        QString("Playlist %1").arg( ++i ), configEntry );
    }
    Amarok::config( "Loaded Playlist Files" ).sync();
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

