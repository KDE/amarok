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

#ifndef MEDIADEVICEHANDLER_READCAPABILITY_H
#define MEDIADEVICEHANDLER_READCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "ReadCapabilityBase.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{

class MEDIADEVICECOLLECTION_EXPORT ReadCapability : public Handler::ReadCapabilityBase
{
    Q_OBJECT

    public:
        virtual ~ReadCapability();

        /* Parsing of Tracks on Device */

        /**
         * Methods that wrap get/set of information using given library (e.g. libgpod)
         * Subclasses of MediaDeviceHandler must keep a pointer to the track struct
         * associated to the track parameter to get the information from the struct in libGet*,
         * and to set the struct's information to the passed metadata in libSet*
         */
        virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual qint64  libGetLength( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual qreal   libGetBpm( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual uint    libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track )  = 0;
        virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track ) = 0;
        virtual KUrl libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track ) = 0;
};
}

#endif
