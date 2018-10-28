/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_COLLECTIONSCANCAPABILITY_H
#define AMAROK_COLLECTIONSCANCAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"

namespace Capabilities
{
    /**
     * This capability allows to initiate a scan on a collection.
     * Currently only a few collections have this capablitity and even then it's unclear
     * Which collections uses the collection folders.
     *
     * @author Ralf Engels <ralf-engels@gmx.de>
     */

    class AMAROK_CORE_EXPORT CollectionScanCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:

            /**
             * Constructor
             */
            CollectionScanCapability();

            /**
             * Destructor
             */
            virtual ~CollectionScanCapability();

            /** Begin a full scan on the collection.
              */
            virtual void startFullScan() = 0;

            /** Begin an incremental scan on the collection.
              @p directory An optional specification of which directory to scan. If empty the scanner will check all the collections directories set in the Amarok settings
              */
            virtual void startIncrementalScan( const QString &directory = QString() ) = 0;

            /** Stop a scan on this collection.
              */
            virtual void stopScan() = 0;

            /**
             * Get the capabilityInterfaceType of this capability
             * @return The capabilityInterfaceType ( always Capabilities::Capability::CollectionScan; )
             */
            static Type capabilityInterfaceType() { return Capabilities::Capability::CollectionScan; }
    };
}

#endif
