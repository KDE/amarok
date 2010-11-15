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

#ifndef AMAROK_COLLECTIONIMPORTCAPABILITY_H
#define AMAROK_COLLECTIONIMPORTCAPABILITY_H

#include "shared/amarok_export.h"
#include "core/capabilities/Capability.h"

#include <QList>

namespace Capabilities
{
    /**
     * This capability allows the collection to import it's content form a file.
     * Currently this is only used by the SqlCollection and it's scanner
     *
     * @author Ralf Engels <ralf-engels@gmx.de>
     */

    class AMAROK_CORE_EXPORT CollectionImportCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:

            CollectionImportCapability();
            virtual ~CollectionImportCapability();

            /** Starts importing the given file into the collection.
             * @return A QObject that can be used to connect several status signals from.
             */
            virtual QObject *import( const QString &importFilePath ) = 0;

            /**
             * Get the capabilityInterfaceType of this capability
             * @return The capabilityInterfaceType ( always Capabilities::Capability::CollectionImport; )
             */
            static Type capabilityInterfaceType() { return Capabilities::Capability::CollectionImport; }
    };
}

#endif
