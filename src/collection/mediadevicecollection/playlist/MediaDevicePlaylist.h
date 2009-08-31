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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef METAMEDIADEVICEPLAYLIST_H
#define METAMEDIADEVICEPLAYLIST_H

#include "Playlist.h"

#include "Debug.h"

namespace Meta
{

    class MediaDevicePlaylist;

    typedef KSharedPtr<MediaDevicePlaylist> MediaDevicePlaylistPtr;
    typedef QList<MediaDevicePlaylistPtr> MediaDevicePlaylistList;

    class MediaDevicePlaylist : public Playlist
    {
        public:
	MediaDevicePlaylist( const QString &name, const TrackList &tracks );
	~MediaDevicePlaylist();

	// Playlist Functions
	virtual QString name() const { return m_name; }
            virtual QString prettyName() const { return m_name; }
            virtual QString description() const { return QString(); }

            /**override showing just the filename */
            virtual void setName( const QString &name );

            /** returns all tracks in this playlist */
            virtual TrackList tracks();
            virtual void addTrack( Meta::TrackPtr track, int position = -1 );

            virtual void removeTrack( int position );

	    bool hasCapabilityInterface( Meta::Capability::Type type ) const { Q_UNUSED( type ); return false; }
	    Capability* createCapabilityInterface( Capability::Type type ) { Q_UNUSED( type ); return 0; }

    private:
        Meta::TrackList m_tracks;
        QString m_description;

    };

}

Q_DECLARE_METATYPE( Meta::MediaDevicePlaylistPtr )
Q_DECLARE_METATYPE( Meta::MediaDevicePlaylistList )

#endif
