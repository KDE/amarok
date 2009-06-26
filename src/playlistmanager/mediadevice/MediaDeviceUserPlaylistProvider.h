/*
 *  Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
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
#ifndef AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H
#define AMAROK_COLLECTION_MEDIADEVICEUSERPLAYLISTPROVIDER_H

#include "playlistmanager/UserPlaylistProvider.h"
#include "meta/MediaDevicePlaylist.h"
//#include "MediaDevicePlaylistGroup.h"

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

        QList<PopupDropperAction *> playlistActions( Meta::PlaylistList list );

	/// MediaDevice-specific Functions

	void addPlaylist( Meta::MediaDevicePlaylistPtr &playlist )
	    {
		m_playlists << playlist;
		emit updated();
	    }

//        Meta::MediaDevicePlaylistGroupPtr group( const QString &name );
//        bool import( const QString& fromLocation );

//        static Meta::MediaDevicePlaylistList toMediaDevicePlaylists( Meta::PlaylistList playlists );

    signals:
//        void updated();

    private slots:
//        void slotDelete();
//        void slotRename();

    private:
//        void reloadFromDb();
//        Meta::MediaDevicePlaylistGroupPtr m_root;

//        Meta::MediaDevicePlaylistList selectedPlaylists() const
//            { return m_selectedPlaylists; };
//        Meta::MediaDevicePlaylistList m_selectedPlaylists;
//        PopupDropperAction *m_renameAction;
    Meta::MediaDevicePlaylistList m_playlists;
};

#endif
