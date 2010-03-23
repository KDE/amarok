/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_LASTFMCAPABILITY_H
#define AMAROK_LASTFMCAPABILITY_H

#include "amarok_export.h"
#include "core/capabilities/Capability.h"

namespace Meta
{
    class AMAROK_EXPORT LastFmCapability : public Meta::Capability
    {
        Q_OBJECT
        public:
            virtual ~LastFmCapability();

            virtual void love() = 0;
            virtual void ban() = 0;
            virtual void skip() = 0;

            static Type capabilityInterfaceType() { return Meta::Capability::LastFm; }
    };
}

#endif
