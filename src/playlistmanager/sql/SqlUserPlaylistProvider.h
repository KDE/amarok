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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_SQLUSERPLAYLISTPROVIDER_H

#include "playlistmanager/UserPlaylistProvider.h"
#include "meta/SqlPlaylist.h"
#include "SqlPlaylistGroup.h"

#include <klocale.h>

class PopupDropperAction;

class AMAROK_EXPORT SqlUserPlaylistProvider : public UserPlaylistProvider
{
    Q_OBJECT
    public:
        SqlUserPlaylistProvider();
        ~SqlUserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual QString prettyName() const { return i18n("Local Playlists stored in the database"); };

        /* UserPlaylistProvider functions */
        virtual Meta::PlaylistList playlists();

        virtual bool canSavePlaylists() { return true; };
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name );

        virtual bool supportsEmptyGroups() { return true; }

        QList<PopupDropperAction *> playlistActions( Meta::PlaylistPtr playlist );
        QList<PopupDropperAction *> trackActions( Meta::PlaylistPtr playlist,
                                                  int trackIndex );

        Meta::SqlPlaylistGroupPtr group( const QString &name );
        bool import( const QString& fromLocation );

        static Meta::SqlPlaylistList toSqlPlaylists( Meta::PlaylistList playlists );

    signals:
        void updated();

    private slots:
        void slotDelete();
        void slotRename();
        void slotRemove();

    private:
        void reloadFromDb();
        Meta::SqlPlaylistGroupPtr m_root;

        void createTables();
        void deleteTables();
        void checkTables();
        void loadFromDb();

        Meta::SqlPlaylistList selectedPlaylists() const
            { return m_selectedPlaylists; };
        Meta::SqlPlaylistList m_selectedPlaylists;
        PopupDropperAction *m_renameAction;
        PopupDropperAction *m_deleteAction;
        PopupDropperAction *m_removeTrackAction;
};

#endif
