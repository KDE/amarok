/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef TIMECODEBOUNDEDPLAYBACKCAPABILITY_H
#define TIMECODEBOUNDEDPLAYBACKCAPABILITY_H

#include "core-impl/meta/timecode/TimecodeMeta.h"
#include "core/capabilities/BoundedPlaybackCapability.h"

namespace Capabilities {

class AMAROK_EXPORT TimecodeBoundedPlaybackCapability : public BoundedPlaybackCapability
{
Q_OBJECT
public:
    TimecodeBoundedPlaybackCapability( Meta::TimecodeTrack * track )
        : m_track( track )
    {}

    virtual qint64 startPosition();
    virtual qint64 endPosition();

private:
    Meta::TimecodeTrack * m_track;

};

}
#endif
