/****************************************************************************************
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

#ifndef AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H

#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "SqlPlaylist.h"
#include "SqlPlaylistGroup.h"

#include <QIcon>

#include <KLocalizedString>

class QAction;

namespace Playlists {

class AMAROK_EXPORT SqlUserPlaylistProvider : public UserPlaylistProvider
{
    Q_OBJECT
    public:
        /**
         * SqlUserPlaylistProvider constructor
         * @param debug used for unit testing; enabling means skipping
         * confirmation dialogs when deleting or renaming playlists.
         */
        explicit SqlUserPlaylistProvider( bool debug = false );
        ~SqlUserPlaylistProvider();

        /* PlaylistProvider functions */
        QString prettyName() const override { return i18n( "Amarok Database" ); }
        virtual QString description() const { return i18n( "Local playlists stored in the database" ); }
        QIcon icon() const override { return QIcon::fromTheme( "server-database" ); }

        int playlistCount() const override;
        Playlists::PlaylistList playlists() override;

        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks );
        Playlists::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name ) override;

        /* UserPlaylistProvider functions */
        bool isWritable() override;
        bool deletePlaylists( const Playlists::PlaylistList &playlistlist ) override;
        void renamePlaylist(Playlists::PlaylistPtr playlist, const QString &newName ) override;

        Playlists::SqlPlaylistGroupPtr group( const QString &name );

        static Playlists::SqlPlaylistList toSqlPlaylists( Playlists::PlaylistList playlists );

    private:
        void reloadFromDb();
        Playlists::SqlPlaylistGroupPtr m_root;

        void createTables();
        void deleteTables();
        void checkTables();
        /**
         * removes COLUMN "description" from "playlists"
         */
        void upgradeVersion2to3();
        void loadFromDb();

        bool deleteSqlPlaylists( Playlists::SqlPlaylistList playlistlist );

        Playlists::SqlPlaylistList selectedPlaylists() const
            { return m_selectedPlaylists; }
        Playlists::SqlPlaylistList m_selectedPlaylists;

        const bool m_debug;
};

} //namespace Playlists

#endif
