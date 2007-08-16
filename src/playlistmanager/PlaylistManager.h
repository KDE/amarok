/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

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
#ifndef AMAROK_PLAYLISTMANAGER_H
#define AMAROK_PLAYLISTMANAGER_H

#include "amarok.h"
#include "amarok_export.h"
#include "plugin/plugin.h"
#include "meta/Playlist.h"

#include <QMultiMap>

class PlaylistManager;
class PlaylistProvider;

/**
 * Facility for managing PlaylistProviders registered by other
 * parts of the application, plugins and scripts.
 */
class PlaylistManager : public QObject
{
    enum PlaylistType
    {
        UserPlaylist = 1,
        PodcastChannel,
        PodcastPlaylist,
        Dynamic,
        SmartPlaylist,
        Lastfm
    }

    Q_OBJECT
    public:
        PlaylistManager();
        ~PlaylistManager();

        /**
         * returns playlists of a certain type from all registered PlaylistProviders
         */
        Meta::PlaylistList PlaylistsOfType( int playlistType )
                { return m_map.values( playlistType ); }

        /**
         * Add a PlaylistProvider that contains Playlists of a type defined
         * in the PlaylistType enum.
         */
        void addProvider( int playlistType, PodcastProvider * provider );

        /** Add a PlaylistProvider that of a custom type.
         * This is supposed to be used by plugins and scripts.
         * Make sure custom types don't conflict with the default types enum.
         */
        void addCustomProvider( int customPlaylistType, PodcastProvider * provider );

    private:
        QMultiMap<int, Meta::PlaylistPtr> m_map;

}

class AMAROK_EXPORT PlaylistProvider : public QObject : public Amarok::Plugin
{
    Q_OBJECT
    public:
        virtual ~PlaylistManager() = 0;

        virtual QString prettyName() const = 0;
        virtual int ofType() const = 0;
        virtual QString typeName() = 0;

}

#endif