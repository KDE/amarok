/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H

#include "playlistmanager/UserPlaylistProvider.h"
#include "MediaDevicePlaylist.h"

#include <klocale.h>
#include <kicon.h>

class QAction;

class AMAROK_EXPORT MediaDeviceUserPlaylistProvider : public UserPlaylistProvider
{
    Q_OBJECT
    public:
        MediaDeviceUserPlaylistProvider();
        ~MediaDeviceUserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual QString prettyName() const { return i18n("Media Device playlists"); };
        virtual KIcon icon() const { return KIcon( "multimedia-player" ); }

        /* UserPlaylistProvider functions */
        virtual Meta::PlaylistList playlists();

        virtual bool canSavePlaylists() { return true; };
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name );

        //      virtual bool supportsEmptyGroups() { return true; }

        //virtual QList<QAction *> playlistActions( Meta::PlaylistList list );

        virtual bool isWritable() { return true; }

        virtual void rename( Meta::PlaylistPtr playlist, const QString &newName );

        virtual void deletePlaylists( Meta::PlaylistList playlistlist );

        /// MediaDevice-specific Functions

        void addPlaylist( Meta::MediaDevicePlaylistPtr &playlist );
        void removePlaylist( Meta::MediaDevicePlaylistPtr &playlist );

        public slots:
            void sendUpdated() { emit updated(); }

        signals:
            void playlistSaved( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name );
            void playlistRenamed( const Meta::MediaDevicePlaylistPtr &playlist );
            void playlistsDeleted( const Meta::MediaDevicePlaylistList &playlistlist );

        private slots:
        //void slotDelete();
        //void slotRename();
        //void slotRemove();

 private:

    Meta::MediaDevicePlaylistList m_playlists;

    QAction *m_renameAction;
};

#endif
