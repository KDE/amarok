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

#ifndef MEDIADEVICEHANDLER_WRITECAPABILITY_H
#define MEDIADEVICEHANDLER_WRITECAPABILITY_H

#include "mediadevicecollection_export.h"
#include "../MediaDeviceHandlerCapability.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT WriteCapability : public Handler::Capability
    {
        Q_OBJECT
    public:
        virtual ~WriteCapability();

    /** Returns a list of formats supported by the device, all in lowercase
    *  For example mp3, mpeg, aac.  This is used to avoid copying unsupported
    *  types to a particular device.
    */

    virtual QStringList supportedFormats() = 0; // md:write

    /**
    * Finds the place to copy the track to on the device, which
    * could be a url in the case of Ipods, or a folder in the
    * case of MTP devices.
    * @param srcTrack The source track of the copy
    * @param destTrack The destination track whose path we seek
    */

    virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack ) = 0;

    /** libCopyTrack does the actual file copying.  For Ipods, it uses KIO,
    *  for MTPs this uses a libmtp call
    *  Copy the file associate with srcTrack to destTrack
    *  @param srcTrack The track being copied from
    *  @param destTrack The track being copied to
    *  @return Whether or not the track copy was successful
    */

    virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack ) = 0;

    /** libDeleteTrack does the actual file deleting.  For Ipods, it uses KIO,
    *  for MTPs this uses a libmtp call.  Must emit libRemoveTrackDone when finished.
    *  @param track The track whose file is to be deleted
    *  @return Whether or not the track removal was successful
    */

    virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Creates a new track struct particular to the library of the device
    *  e.g. LIBMTP_new_track_t(), and associates it with the track for
    *  later use, in the same way that setAssociateTrack does it.
    *  @param track The track for which to create a track struct and associate it to
    */

    virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Deletes the track struct associated with this track, freeing
    *  any memory it occupied, and dissociating it from the track
    *  @param track The track whose associated track struct is to be deleted.
    */

    virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Adds the newly created track struct now populated with info into the
    *  database struct of the particular device, e.g. into the itdb for Ipods.
    *  MTP devices automatically add the track into the database upon copying,
    *  so MTP would do nothing.
    *  @param track The track whose associated track struct is to be added \
    into the database.
    */


    virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Remove all traces of the track struct associated with @param track from
    *  the database struct, but do not delete the struct
    *  @param track The track whose associated track struct is to be removed \
    from the database.
    */

    virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Indicates to the subclass that the database has been updated
    *  For ipods, just sets m_dbchanged = true
    */

    virtual void databaseChanged() = 0;

    /**  Each libSet function sets the private track struct associated with @param track
    *    to the second value passed into the function.
    */
    virtual void    libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title ) = 0;
    virtual void    libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album ) = 0;
    virtual void    libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist ) = 0;
    virtual void    libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer ) = 0;
    virtual void    libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre ) = 0;
    virtual void    libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year ) = 0;
    virtual void    libSetLength( Meta::MediaDeviceTrackPtr &track, int length ) = 0;
    virtual void    libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum ) = 0;
    virtual void    libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment ) = 0;
    virtual void    libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum ) = 0;
    virtual void    libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate ) = 0;
    virtual void    libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate ) = 0;
    virtual void    libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm ) = 0;
    virtual void    libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize ) = 0;
    virtual void    libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount ) = 0;
    virtual void    libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed ) = 0;
    virtual void    libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )  = 0;
    virtual void    libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type ) = 0;
    virtual void    libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack ) = 0;

    // TODO: NYI
    //virtual void    libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QPixmap& image ) = 0;

    /** This function is called just before copying tracks begin and allows
    *  a subclass to prepare to copy, e.g. for Ipods it would initialize
    *  the job counter to 0.
    */

    virtual void prepareToCopy() = 0;

    /** This function is called just before deleting tracks begin and allows
    *  a subclass to prepare to delete, e.g. for Ipods it would initialize
    *  the m_tracksdeleting to keep track of urls it is deleting.
    */

    virtual void prepareToDelete() = 0;

    /** Tells subclass that it can update the track, usually because
    *  the track's tags have changed.
    *  @param track The track whose tags should be updated
    */

    virtual void updateTrack( Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) };

    static Type capabilityInterfaceType() { return Handler::Capability::Writable; }

    };
}

#endif
