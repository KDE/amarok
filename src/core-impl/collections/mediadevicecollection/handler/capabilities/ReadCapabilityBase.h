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

#ifndef MEDIADEVICEHANDLER_READCAPABILITYBASE_H
#define MEDIADEVICEHANDLER_READCAPABILITYBASE_H

#include "mediadevicecollection_export.h"
#include "../MediaDeviceHandlerCapability.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{

class MEDIADEVICECOLLECTION_EXPORT ReadCapabilityBase : public Handler::Capability
{
    Q_OBJECT

    public:
        virtual ~ReadCapabilityBase();

        /* Parsing of Tracks on Device */

        /**
         * Initializes iteration over some list of track structs
         * e.g. with libgpod, this initializes a GList to the beginning of
         * the list of tracks
         */

        virtual void prepareToParseTracks() = 0;

        /**
         * Runs a test to see if we have reached the end of
         * the list of tracks to be parsed on the device, e.g. in libgpod
         * this tests if cur != NULL, i.e. if(cur)
         */
        virtual bool isEndOfParseTracksList() = 0;

        /**
         * Moves the iterator to the next track on the list of
         *  track structs, e.g. with libgpod, cur = cur->next where cur
         *  is a GList*
         */
        virtual void prepareToParseNextTrack() = 0;

        /**
         * This method attempts to access the special struct of the
         * next track, so that information can then be parsed from it.
         * For libgpod, this is m_currtrack = (Itdb_Track*) cur->data
         */
        virtual void nextTrackToParse() = 0;

        /**
         * This method must create a two-way association of the current Meta::Track
         * to the special struct provided by the library to read/write information.
         * For example, for libgpod one would associate Itdb_Track*.  It makes
         * the most sense to use a QHash since it is fastest lookup and order
         * does not matter.
         * @param track The track to two-way associate with a library track struct
         */

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track ) = 0;

        /**
         * Methods that wrap get/set of information using given library (e.g. libgpod)
         * Subclasses of MediaDeviceHandler must keep a pointer to the track struct
         * associated to the track parameter to get the information from the struct in libGet*,
         * and to set the struct's information to the passed metadata in libSet*
         */

        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track ) = 0;

        /**
         * Methods related to device space usage, in bytes
         **/
        virtual float usedCapacity() const { return 0.0; }
        virtual float totalCapacity() const { return 0.0; }

        static Type capabilityInterfaceType() { return Handler::Capability::Readable; }
};
}

#endif
