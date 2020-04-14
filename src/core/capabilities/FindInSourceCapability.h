/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef FINDINSOURCECAPABILITY_H
#define FINDINSOURCECAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"

namespace Capabilities {

/**
This capability exposes a method that shows this track (or the closest possible parent, such as album) in the source where it was added from.

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class AMAROKCORE_EXPORT FindInSourceCapability : public Capabilities::Capability
{
    Q_OBJECT
    Q_FLAGS( TargetTag TargetTags )

public: 
    enum TargetTag
    {
        Artist   = 0x01,
        Album    = 0x02,
        Composer = 0x04,
        Genre    = 0x08,
        Track    = 0x10,
        Year     = 0x20
    };

    ~FindInSourceCapability() override;

    virtual void findInSource( QFlags<TargetTag> tag = Album ) = 0;

    /**
     * Get the capabilityInterfaceType of this capability
     * @return The capabilityInterfaceType ( always Capabilities::Capability::FindInSource; )
     */
    static Type capabilityInterfaceType() { return Capabilities::Capability::FindInSource; }

    Q_DECLARE_FLAGS( TargetTags, TargetTag )
};

} // namespace Capabilities

Q_DECLARE_OPERATORS_FOR_FLAGS( Capabilities::FindInSourceCapability::TargetTags )

#endif // FINDINSOURCECAPABILITY_H
