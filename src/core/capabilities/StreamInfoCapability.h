/****************************************************************************************
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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
 
#ifndef STREAMINFOCAPABILITY_H
#define STREAMINFOCAPABILITY_H

#include "shared/amarok_export.h"
#include "core/capabilities/Capability.h"

#include <QString>
namespace Capabilities
{

    /**
     * This capability is designed to provide additional information
     * about streaming metadata. For meta types that provide multiple
     * tracks within the same stream (lastfm, shoutcast, etc )
     * this capability is designed to return stream metadata,
     * where the properties in the Meta::Track class refers to the track
     * being played within the stream.

        @author Dan Meltzer <parallelgrapefruit@gmail.com>
    */
    class AMAROK_EXPORT StreamInfoCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:
            StreamInfoCapability() {};
            virtual ~StreamInfoCapability() {};

            /**
             * The human readable name of this stream
             * @return A string representing the name of this stream.
             */
            virtual QString streamName() const = 0;
            /**
             * The source this stream belongs to.
             * @return The name of the owning source.
             */
            virtual QString streamSource() const { return QString(); }
            static Type capabilityInterfaceType() { return Capabilities::Capability::StreamInfo; }

    };

}

#endif
