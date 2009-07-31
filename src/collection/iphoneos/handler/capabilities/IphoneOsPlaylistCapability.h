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

#ifndef IPHONEOSHANDLER_PLAYLISTCAPABILITY_H
#define IPHONEOSHANDLER_PLAYLISTCAPABILITY_H

#include <mediadevicecollection_export.h>
#include <handler/capabilities/PlaylistCapability.h>

namespace Meta {
    class IphoneOsHandler;
}

namespace Handler
{

class IphoneOsPlaylistCapability : public PlaylistCapability
{
    Q_OBJECT

    public:
        IphoneOsPlaylistCapability( Meta::IphoneOsHandler *handler );

        virtual void prepareToParsePlaylists();
        virtual bool isEndOfParsePlaylistsList();
        virtual void prepareToParseNextPlaylist();
        virtual void nextPlaylistToParse();
        virtual bool shouldNotParseNextPlaylist();
        virtual void prepareToParsePlaylistTracks();
        virtual bool isEndOfParsePlaylist();
        virtual void prepareToParseNextPlaylistTrack();
        virtual void nextPlaylistTrackToParse();

        virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct();
        virtual QString libGetPlaylistName();
        
        virtual void savePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name );
        virtual void deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        virtual void renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );

    private:
        Meta::IphoneOsHandler *m_handler;

        QStringList m_playlistIds;
        int m_currentPlaylistIndex;
        QString m_currentPlaylist;
        QString m_playlistName;
        QStringList m_pids;
        int m_currentPidIndex;
        QString m_currentPid;
};

}

#endif
