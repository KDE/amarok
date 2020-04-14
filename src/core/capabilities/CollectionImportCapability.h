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

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"

#include <QIODevice>

namespace Capabilities
{
    /**
     * This capability allows the collection to import it's content form a file.
     * Currently this is only used by the SqlCollection and it's scanner
     *
     * @author Ralf Engels <ralf-engels@gmx.de>
     */

    class AMAROKCORE_EXPORT CollectionImportCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:

            CollectionImportCapability();
            ~CollectionImportCapability() override;

            /** Starts importing the given file into the collection.
                @param input is an already opened input device. The importer will take ownership.
                @param listener An object that will listen on import signals.
                Those signals are:
                  trackAdded( Meta::TrackPtr )
                  trackDiscarded( QString )
                  trackMatchFound( Meta::TrackPtr, QString )
                  trackMatchMultiple( Meta::TrackList, QString )
                  importError( QString )
                  done( ThreadWeaver::Job* )
                  showMessage( QString )
                @return A QObject that can be used to connect several status signals from.
                */
            virtual void import( QIODevice *input, QObject *listener ) = 0;

            /** Get the capabilityInterfaceType of this capability
                @return The capabilityInterfaceType ( always Capabilities::Capability::CollectionImport; )
             */
            static Type capabilityInterfaceType() { return Capabilities::Capability::CollectionImport; }
    };
}

#endif
