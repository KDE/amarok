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

#ifndef MEDIADEVICEHANDLER_CUSTOMREADCAPABILITY_H
#define MEDIADEVICEHANDLER_CUSTOMREADCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "ReadCapabilityBase.h"
#include "../../MediaDeviceMeta.h"

namespace Handler
{

class MEDIADEVICECOLLECTION_EXPORT CustomReadCapability : public Handler::ReadCapabilityBase
{
    Q_OBJECT

    public:
        virtual ~CustomReadCapability();

        /* Parsing of Tracks on Device */

        /**
         * Get a track from which to copy metadata for new media device track
         */

        virtual Meta::TrackPtr sourceTrack() = 0;

};
}

#endif
