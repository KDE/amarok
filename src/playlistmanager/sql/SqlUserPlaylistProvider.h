/*
 *  Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H

#include "playlistmanager/UserPlaylistProvider.h"
#include "meta/SqlPlaylist.h"
#include "SqlPlaylistGroup.h"

#include <klocale.h>

class AMAROK_EXPORT SqlUserPlaylistProvider : public UserPlaylistProvider
{
    Q_OBJECT
    public:
        AMAROK_EXPORT SqlUserPlaylistProvider();
        AMAROK_EXPORT ~SqlUserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual QString prettyName() const { return i18n("Local Playlists stored in the database"); };

        /* UserPlaylistProvider functions */
        virtual Meta::PlaylistList playlists();
        virtual Meta::PlaylistGroupList groups();

        virtual bool canSavePlaylists() { return true; };
        virtual bool save( const Meta::TrackList &tracks );

    signals:
        void updated();

    private:
//         Meta::SqlPlaylistGroupPtr m_root;
        Meta::SqlPlaylistList m_playlists;
        Meta::SqlPlaylistGroupList m_groups;

        void createTables();
        void deleteTables();
        void checkTables();
        void loadFromDb();
};

#endif
