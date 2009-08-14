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

#ifndef MEDIADEVICEHANDLER_WRITECAPABILITYBASE_H
#define MEDIADEVICEHANDLER_WRITECAPABILITYBASE_H

#include "mediadevicecollection_export.h"
#include "../MediaDeviceHandlerCapability.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{

class MEDIADEVICECOLLECTION_EXPORT WriteCapabilityBase : public Handler::Capability
{
    Q_OBJECT

    public:
        virtual ~WriteCapabilityBase();

        /**
         * Returns a list of formats supported by the device, all in lowercase
         * For example mp3, mpeg, aac.  This is used to avoid copying unsupported
         * types to a particular device.
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

        /**
         * libCopyTrack does the actual file copying.  For Ipods, it uses KIO,
         * for MTPs this uses a libmtp call
         * Copy the file associate with srcTrack to destTrack
         * @param srcTrack The track being copied from
         * @param destTrack The track being copied to
         * @return Whether or not the track copy was successful
         */
        virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack ) = 0;

        /**
         * libDeleteTrack does the actual file deleting.  For Ipods, it uses KIO,
         * for MTPs this uses a libmtp call.  Must emit libRemoveTrackDone when finished.
         * @param track The track whose file is to be deleted
         * @return Whether or not the track removal was successful
         */
        virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track ) = 0;

        virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack ) = 0;

        /**
         * This function is called just before copying tracks begin and allows
         * a subclass to prepare to copy, e.g. for Ipods it would initialize
         * the job counter to 0.
         */
        virtual void prepareToCopy() = 0;

        /**
         * This function is called just before deleting tracks begin and allows
         * a subclass to prepare to delete, e.g. for Ipods it would initialize
         * the m_tracksdeleting to keep track of urls it is deleting.
         */
        virtual void prepareToDelete() = 0;

        /**
         * Tells subclass that it can update the track, usually because
         * the track's tags have changed.
         * @param track The track whose tags should be updated
         */
        virtual void updateTrack( Meta::MediaDeviceTrackPtr &track )
        {
            Q_UNUSED( track )
        }

        static Type capabilityInterfaceType() { return Handler::Capability::Writable; }
};
}

#endif
