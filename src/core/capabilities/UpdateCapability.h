/****************************************************************************************
 * Copyright (c) 2007 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef AMAROK_UPDATECAPABILITY_H
#define AMAROK_UPDATECAPABILITY_H

#include "amarok_export.h"
#include "core/capabilities/Capability.h"

namespace Meta
{
    /**
      * Do not use this capability. Ever. If you really need to emit Collection::updated()
      * from outside the collection (which is wrong) use Collection::collectionUpdated(),
      * e.g. track->collection()->collectionUpdated().
      *
      * The proper solution is to add code to the collection that allows it figure out on its
      * own whether it has been changed.
      */
    class AMAROK_EXPORT UpdateCapability : public Meta::Capability
    {
        Q_OBJECT
        public:
            virtual ~UpdateCapability();

            static Type capabilityInterfaceType() { return Meta::Capability::Updatable; }
        // simply should emit track's collection's updated signal
            virtual void collectionUpdated() const = 0;

    };
}

#endif
