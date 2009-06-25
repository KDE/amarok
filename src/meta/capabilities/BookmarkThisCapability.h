/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef METABOOKMARKTHISCAPABILITY_H
#define METABOOKMARKTHISCAPABILITY_H

#include "amarok_export.h"
#include "meta/Capability.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"

namespace Meta {

/**
This capability dertermines wheter a meta item in a collection can be directly bookmarked. Not all collections/services supports bookmarks on all levels, and some might not support Item level bookmarks at all as they have no query field and some might only support simple queries.

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT BookmarkThisCapability : public Capability {
    Q_OBJECT
public:
    virtual ~BookmarkThisCapability();

    virtual bool isBookmarkable() { return false; }
    virtual QString browserName() = 0;
    virtual QString collectionName() = 0;
    virtual bool simpleFiltering() { return false; }
    virtual PopupDropperAction * bookmarkAction() = 0;

    /**
     * Get the capabilityInterfaceType of this capability
     * @return The capabilityInterfaceType ( always Meta::Capability::BookmarkThis; )
    */
    static Type capabilityInterfaceType() { return Meta::Capability::BookmarkThis; }

};

}

#endif
