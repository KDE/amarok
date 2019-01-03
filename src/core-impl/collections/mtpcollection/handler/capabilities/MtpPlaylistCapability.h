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

#ifndef MTPPLAYLISTCAPABILITY_H
#define MTPPLAYLISTCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "PlaylistCapability.h"

namespace Meta {
    class MtpHandler;
}

namespace Handler
{

class MtpPlaylistCapability : public PlaylistCapability
{
    Q_OBJECT

    public:
        explicit MtpPlaylistCapability( Meta::MtpHandler *handler );

        void prepareToParsePlaylists() override;
        bool isEndOfParsePlaylistsList() override;
        void prepareToParseNextPlaylist() override;
        void nextPlaylistToParse() override;
        bool shouldNotParseNextPlaylist() override;
        void prepareToParsePlaylistTracks() override;
        bool isEndOfParsePlaylist() override;
        void prepareToParseNextPlaylistTrack() override;
        void nextPlaylistTrackToParse() override;
        
        void savePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name ) override;
        void deletePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist ) override;
        void renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist ) override;

        Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct() override;
        QString libGetPlaylistName() override;

    private:
        Meta::MtpHandler *m_handler;
};

}

#endif
