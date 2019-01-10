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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H

#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "MediaDevicePlaylist.h"

#include <QIcon>

#include <KLocalizedString>

namespace Collections {
    class MediaDeviceCollection;
}

namespace Playlists {

class AMAROK_EXPORT MediaDeviceUserPlaylistProvider : public Playlists::UserPlaylistProvider
{
    Q_OBJECT
    public:
        explicit MediaDeviceUserPlaylistProvider( Collections::MediaDeviceCollection *collection );
        ~MediaDeviceUserPlaylistProvider();

        /* PlaylistProvider functions */
        QString prettyName() const override { return i18n( "Media Device playlists" ); }
        QIcon icon() const override { return QIcon::fromTheme( "multimedia-player" ); }

        /* Playlists::UserPlaylistProvider functions */
        Playlists::PlaylistList playlists() override;

        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks );
        Playlists::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name ) override;

        bool isWritable() override { return true; }
        void renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName ) override;
        bool deletePlaylists( const Playlists::PlaylistList &playlistlist ) override;

        /// MediaDevice-specific Functions

        void addMediaDevicePlaylist( Playlists::MediaDevicePlaylistPtr &playlist );
        void removePlaylist( Playlists::MediaDevicePlaylistPtr &playlist );

        public Q_SLOTS:
            void sendUpdated() { Q_EMIT updated(); }

        Q_SIGNALS:
            void playlistSaved( const Playlists::MediaDevicePlaylistPtr &playlist, const QString &name );
            void playlistRenamed( const Playlists::MediaDevicePlaylistPtr &playlist );
            void playlistsDeleted( const Playlists::MediaDevicePlaylistList &playlistlist );

 private:
    MediaDevicePlaylistList m_playlists;

    Collections::MediaDeviceCollection *m_collection;
};

} //namespace Playlists

#endif
