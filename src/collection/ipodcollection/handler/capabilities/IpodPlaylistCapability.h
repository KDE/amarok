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

#ifndef IPODHANDLER_PLAYLISTCAPABILITY_H
#define IPODHANDLER_PLAYLISTCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "PlaylistCapability.h"

namespace Meta {
    class IpodHandler;
}

namespace Handler
{

class IpodPlaylistCapability : public PlaylistCapability
{
    Q_OBJECT
    public:
    IpodPlaylistCapability( Meta::IpodHandler *handler );

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

    virtual void savePlaylist( const Meta::TrackList &tracks, const QString& name );
    virtual void renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );

    virtual void setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );

    private:
        Meta::IpodHandler *m_handler;
};

}

#endif
