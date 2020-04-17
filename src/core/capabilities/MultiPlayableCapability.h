/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
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

#ifndef AMAROK_MULTIPLAYABLECAPABILITY_H
#define AMAROK_MULTIPLAYABLECAPABILITY_H

#include "core/capabilities/Capability.h"

#include <QUrl>

namespace Capabilities
{
    class AMAROKCORE_EXPORT MultiPlayableCapability : public Capability
    {
        Q_OBJECT

        public:
            ~MultiPlayableCapability() override;

            static Type capabilityInterfaceType()
                { return Capabilities::Capability::MultiPlayable; }

            virtual void fetchFirst() = 0;
            virtual void fetchNext() = 0;

        Q_SIGNALS:
            void playableUrlFetched( const QUrl &url );
    };
}

#endif // AMAROK_MULTIPLAYABLECAPABILITY_H
