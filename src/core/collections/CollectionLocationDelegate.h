/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef COLLECTIONLOCATIONDELEGATE_H
#define COLLECTIONLOCATIONDELEGATE_H

#include "shared/amarok_export.h"
#include "core/meta/Meta.h"

class CollectionLocation;

namespace Collections {

class AMAROK_EXPORT CollectionLocationDelegate
{
public:
    CollectionLocationDelegate() {};
    virtual ~ CollectionLocationDelegate() {};

    virtual bool reallyDelete( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual bool reallyMove( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual void errorDeleting( CollectionLocation *loc, const Meta::TrackList &tracks ) const = 0;
    virtual void notWriteable( CollectionLocation *loc ) const = 0;
};

} //namespace Collections

#endif
