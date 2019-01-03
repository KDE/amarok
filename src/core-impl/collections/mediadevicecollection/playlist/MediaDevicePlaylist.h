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

#ifndef METAMEDIADEVICEPLAYLIST_H
#define METAMEDIADEVICEPLAYLIST_H

#include "core/playlists/Playlist.h"

#include "core/support/Debug.h"

namespace Playlists
{

    class MediaDevicePlaylist;

    typedef AmarokSharedPointer<MediaDevicePlaylist> MediaDevicePlaylistPtr;
    typedef QList<MediaDevicePlaylistPtr> MediaDevicePlaylistList;

    class MediaDevicePlaylist : public Playlist
    {
        public:
            MediaDevicePlaylist( const QString &name, const Meta::TrackList &tracks );
            ~MediaDevicePlaylist();

            // Playlist Functions
            QString name() const override { return m_name; }
            QUrl uidUrl() const override { return QUrl(); }

            /**override showing just the filename */
            void setName( const QString &name ) override;

            int trackCount() const override;
            Meta::TrackList tracks() override;
            void addTrack( Meta::TrackPtr track, int position = -1 ) override;

            void removeTrack( int position ) override;

    private:
        Meta::TrackList m_tracks;
        QString m_name;

    };

}

Q_DECLARE_METATYPE( Playlists::MediaDevicePlaylistPtr )
Q_DECLARE_METATYPE( Playlists::MediaDevicePlaylistList )

#endif
