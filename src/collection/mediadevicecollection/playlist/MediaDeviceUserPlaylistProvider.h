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

class PopupDropperAction;

class AMAROK_EXPORT MediaDeviceUserPlaylistProvider : public UserPlaylistProvider
{
    Q_OBJECT
    public:
        MediaDeviceUserPlaylistProvider();
        ~MediaDeviceUserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual QString prettyName() const { return i18n("Media Device playlists"); };

        /* UserPlaylistProvider functions */
        virtual Meta::PlaylistList playlists();

        virtual bool canSavePlaylists() { return true; };
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name );

        //      virtual bool supportsEmptyGroups() { return true; }

        //virtual QList<PopupDropperAction *> playlistActions( Meta::PlaylistList list );

        /// MediaDevice-specific Functions

        void addPlaylist( Meta::MediaDevicePlaylistPtr &playlist );

        public slots:
            void sendUpdated() { emit updated(); }

        signals:
            void playlistSaved( const Meta::TrackList &tracks, const QString& name );

 private:

    Meta::MediaDevicePlaylistList m_playlists;
};

#endif
