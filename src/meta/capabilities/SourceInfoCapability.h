/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef SOURCEINFOCAPABILITY_H
#define SOURCEINFOCAPABILITY_H

#include "amarok_export.h"
#include "meta/Capability.h"

class QPixmap;

namespace Meta
{

    /**
    This capability allows getting additional information about the source of a meta item. For now, it is intended for allowing the playlist to display a little emblem to let users know if a track is a Magnatune preview track, a lastfm stream or so on...

        @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
    */
    class AMAROK_EXPORT SourceInfoCapability : public Meta::Capability{
    public:
        Q_OBJECT
        public:
            /**
             * Constructor
             */
            SourceInfoCapability();
            /**
             * Destructor
             */
            virtual ~SourceInfoCapability();

            /**
             * Get the human readable name of the source, for instance "Magnatune.com"
             * @return The name of the source
             */
            virtual QString sourceName() = 0;
            /**
             * Get a brief human readable description or the source
             * @return The source description
             */
            virtual QString sourceDescription() = 0;
            /**
             * Get a small 16x16 pixle emblem that represents the source.
             * @return The source emblem
             */
            virtual QPixmap emblem() = 0;

            /**
             * Get a path to a scalable (svg) version of the source emblem.
             */
            virtual QString scalableEmblem() = 0;

            /**
             * Get the capabilityInterfaceType of this capability
             * @return The capabilityInterfaceType ( always Meta::Capability::SourceInfo; )
             */
            static Type capabilityInterfaceType() { return Meta::Capability::SourceInfo; }

    };

}

#endif
