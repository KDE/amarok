/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef AMAROK_COLLECTIONCAPABILITY_H
#define AMAROK_COLLECTIONCAPABILITY_H

#include "shared/amarok_export.h"
#include "core/capabilities/Capability.h"
#include "core/meta/Meta.h"


#include <QAction>
#include <QList>
#include <QObject>

namespace Capabilities
{

    class AMAROK_EXPORT CollectionCapability : public Capabilities::Capability
    {
        Q_OBJECT

        public:
            virtual ~CollectionCapability();

            static Type capabilityInterfaceType() { return Capabilities::Capability::Collection; }
            virtual QList<QAction*> collectionActions() = 0;
    };
}

#endif
