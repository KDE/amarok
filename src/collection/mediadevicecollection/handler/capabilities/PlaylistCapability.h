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

#ifndef MEDIADEVICEHANDLER_PLAYLISTCAPABILITY_H
#define MEDIADEVICEHANDLER_PLAYLISTCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "../MediaDeviceHandlerCapability.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT PlaylistCapability : public Handler::Capability
    {
        Q_OBJECT
    public:
        virtual ~PlaylistCapability();

    /// Parsing of Tracks in Playlists on Device
    /// NOTE: not required by devices with no playlists, just reimplement empty functions

    /** This method initializes iteration over some list of playlist structs
    *  e.g. with libgpod, this initializes a GList to the beginning of
    *  the list of playlists
    */

    virtual void prepareToParsePlaylists() = 0;

    /** This method runs a test to see if we have reached the end of
    *  the list of playlists to be parsed on the device, e.g. in libgpod
    *  this tests if cur != NULL, i.e. if(cur)
    */

    virtual bool isEndOfParsePlaylistsList() = 0;

    /** This method moves the iterator to the next playlist on the list of
    *  playlist structs, e.g. with libgpod, cur = cur->next where cur
    *  is a GList*
    */

    virtual void prepareToParseNextPlaylist() = 0;

    /** This method attempts to access the special struct of the
    *  next playlist, so that information can then be parsed from it.
    *  For libgpod, this is m_currplaylist = ( Itdb_Playlist * ) cur->data
    */

    virtual void nextPlaylistToParse() = 0;

    /** This method checks if the playlist should be parsed, or skipped.
    *  Certain playlists, like the master playlist on the iPod, do not
    *  need to be or should not be parsed.
    *  @return true if should not parse, false otherwise.
    */

    virtual bool shouldNotParseNextPlaylist() = 0;

    /** This method initializes iteration over some list of track structs
    *  that correspond to a playlist struct
    *  e.g. with libgpod, this initializes a GList to the beginning of
    *  the list of tracks
    */

    virtual void prepareToParsePlaylistTracks() = 0;

    /** This method runs a test to see if we have reached the end of
    *  the list of tracks in the playlist to be parsed on the device, e.g. in libgpod
    *  this tests if cur != NULL, i.e. if(cur)
    */

    virtual bool isEndOfParsePlaylist() = 0;

    /** This method moves the iterator to the next track on the playlist of
    *  track structs, e.g. with libgpod, cur = cur->next where cur
    *  is a GList*
    */

    virtual void prepareToParseNextPlaylistTrack() = 0;

    /** This method attempts to access the special struct of the
    *  next track on the playlist, so that information can then be parsed from it.
    *  For libgpod, this is m_currtrack = (Itdb_Track*) cur->data
    */

    virtual void nextPlaylistTrackToParse() = 0;

    /** Returns a MediaDeviceTrackPtr that is associated with the currently parsed track struct.
    *  @return A MediaDeviceTrackPtr to currently parsed track struct
    */

    virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct() = 0;

    /** Returns a string containing the playlist name of the currently parsed playlist struct, if available.
    *  @return A string with the name of the currently parsed playlist
    */

    virtual QString libGetPlaylistName() = 0;

    /** Saves a playlist of tracks, with a name.
    *  @param tracks the tracks that make up the playlist to be made
    *  @param name the name of the playlist
    */

    // TODO: make pure virtual, reimplement in every plcapability
    virtual void savePlaylist( const Meta::TrackList &tracks, const QString& name ) { Q_UNUSED(tracks) Q_UNUSED( name) }

    static Type capabilityInterfaceType() { return Handler::Capability::Playlist; }

    };
}

#endif
