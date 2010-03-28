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

#include "core/playlists/providers/user/UserPlaylistProvider.h"
#include "MediaDevicePlaylist.h"

#include <klocale.h>
#include <kicon.h>

class QAction;

namespace Collections {
    class MediaDeviceCollection;
}

namespace Playlists {

class AMAROK_EXPORT MediaDeviceUserPlaylistProvider : public Playlists::UserPlaylistProvider
{
    Q_OBJECT
    public:
        MediaDeviceUserPlaylistProvider( Collections::MediaDeviceCollection *collection );
        ~MediaDeviceUserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual QString prettyName() const { return i18n("Media Device playlists"); };
        virtual KIcon icon() const { return KIcon( "multimedia-player" ); }

        /* Playlists::UserPlaylistProvider functions */
        virtual Playlists::PlaylistList playlists();

        virtual bool canSavePlaylists() { return true; };
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name );

        //      virtual bool supportsEmptyGroups() { return true; }

        //virtual QList<QAction *> playlistActions( Meta::PlaylistList list );

        virtual bool isWritable() { return true; }

        virtual void rename( Playlists::PlaylistPtr playlist, const QString &newName );

        virtual void deletePlaylists( Playlists::PlaylistList playlistlist );

        /// MediaDevice-specific Functions

        void addPlaylist( Playlists::MediaDevicePlaylistPtr &playlist );
        void removePlaylist( Playlists::MediaDevicePlaylistPtr &playlist );

        public slots:
            void sendUpdated() { emit updated(); }

        signals:
            void playlistSaved( const MediaDevicePlaylistPtr &playlist, const QString& name );
            void playlistRenamed( const MediaDevicePlaylistPtr &playlist );
            void playlistsDeleted( const MediaDevicePlaylistList &playlistlist );

        private slots:
        //void slotDelete();
        //void slotRename();
        //void slotRemove();

 private:

    MediaDevicePlaylistList m_playlists;

    QAction *m_renameAction;
    Collections::MediaDeviceCollection *m_collection;
};

} //namespace Playlists

#endif
