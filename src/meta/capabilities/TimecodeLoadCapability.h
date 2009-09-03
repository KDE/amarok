/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef TIMECODELOADCAPABILITY_H
#define TIMECODELOADCAPABILITY_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "meta/Capability.h"
#include "amarokurls/AmarokUrl.h"

#include <KSharedPtr>

#include <QList>

namespace Meta {

typedef QList<AmarokUrlPtr> BookmarkList;
typedef KSharedPtr<AmarokUrl> AmarokUrlPtr;
/**
* This capability determines whether a track has timecodes
* that can be loaded from it, and supplies them if it can.
* @author Casey Link
*/
class AMAROK_EXPORT TimecodeLoadCapability : public Capability
{
    Q_OBJECT
public:
    virtual ~TimecodeLoadCapability();

    /**
     * @return true if the track has timecodes, false if not
     */
    virtual bool hasTimecodes() = 0 ;

    /**
     * @return a QList of AmarokUrlPtrs representing the track's timecodes. Might return an empty list.
     */
    virtual BookmarkList loadTimecodes()  = 0;

    /**
    * Get the capabilityInterfaceType of this capability
    * @return The capabilityInterfaceType ( always Meta::Capability::LoadTimecode; )
    */
    static Type capabilityInterfaceType()
    {
        return Meta::Capability::LoadTimecode;
    }
};

}

#endif // TIMECODELOADCAPABILITY_H
