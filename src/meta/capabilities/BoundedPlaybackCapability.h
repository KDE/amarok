/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef METABOUNDEDPLAYBACKCAPABILITY_H
#define METABOUNDEDPLAYBACKCAPABILITY_H

#include "meta/Capability.h"

namespace Meta {

/**
A capability for tracks that represents a given, bounded, interval of a url, for instance a single track in a long podcast.

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class BoundedPlaybackCapability : public Capability
{
    Q_OBJECT
public:
    BoundedPlaybackCapability();
    ~BoundedPlaybackCapability();

    virtual qint64 startPosition() = 0;
    virtual qint64 endPosition() = 0;
    
    /**
     * Get the capabilityInterfaceType of this capability
     * @return The capabilityInterfaceType ( always Meta::Capability::BoundedPlayback; )
     */
    static Type capabilityInterfaceType() { return Capability::BoundedPlayback; }

};

}

#endif
