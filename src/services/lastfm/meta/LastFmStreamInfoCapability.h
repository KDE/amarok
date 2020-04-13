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

#ifndef AMAROK_LASTFMSTREAMINFOCAPABILITY_H
#define AMAROK_LASTFMSTREAMINFOCAPABILITY_H

#include "core/capabilities/StreamInfoCapability.h"

namespace LastFm
{
    class Track;
}
class LastFmStreamInfoCapability : public Capabilities::StreamInfoCapability
{
    Q_OBJECT
    public:
        explicit LastFmStreamInfoCapability( LastFm::Track *track );
        ~LastFmStreamInfoCapability();

        QString streamName() const override;
        QString streamSource() const override;

    private:
        LastFm::Track *m_sourceTrack;

};

#endif
