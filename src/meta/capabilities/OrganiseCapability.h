/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_ORGANISECAPABILITY_H
#define AMAROK_ORGANISECAPABILITY_H

#include "amarok_export.h"
#include "capabilities/Capability.h"

namespace Meta
{
    class AMAROK_EXPORT OrganiseCapability : public Meta::Capability
    {
        Q_OBJECT
        public:
            virtual ~OrganiseCapability();

            static Type capabilityInterfaceType() { return Meta::Capability::Organisable; }

            /**
             * delete this track from the collection
             */
            virtual void deleteTrack() = 0;
            //virtual void organiseTrack() = 0;
    };
}

#endif
