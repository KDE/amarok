/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef METACAPABILITY_H
#define METACAPABILITY_H

#include "core/capabilities/Capability.h"
#include "core/amarokcore_export.h"

class AMAROKCORE_EXPORT MetaCapability
{
    public:
        virtual ~MetaCapability() {}

        /**
         * Return true if this entity has capability @c CapIface, false otherwise.
         */
        template <class CapIface> bool has() const
        {
            return hasCapabilityInterface( CapIface::capabilityInterfaceType() );
        }

        /**
         * Creates a specialized interface which represents a capability of this
         * Meta::Base object. The caller of this method is responsible for deleting
         * created capability!
         *
         * @returns a pointer to the capability interface if it exists, 0 otherwise
         */
        template <class CapIface> CapIface *create()
        {
            Capabilities::Capability::Type type = CapIface::capabilityInterfaceType();
            Capabilities::Capability *iface = createCapabilityInterface( type );
            return qobject_cast<CapIface *>( iface );
        }

        /**
         * Subclasses should override this method to denote they provide particular
         * capability type. Must match @see createCapabilityInterface()
         *
         * This method should be considered protected (but is not because of practical
         * reasons), you should normally call @see has()
         */
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        /**
         * Subclasses should override this method to create particular capability.
         * Memory-management of the returned pointer is the responsibility of the
         * caller of this method. Must match @see hasCapabilityInterface()
         *
         * This method should be considered protected (but is not because of practical
         * reasons), you should normally call @see create()
         */
        virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type );
};

#endif // METACAPABILITY_H
