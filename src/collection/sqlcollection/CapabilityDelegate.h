/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef CAPABILITYDELEGATE_H
#define CAPABILITYDELEGATE_H

#include "amarok_sqlcollection_export.h"
#include "core/capabilities/Capability.h"

class SqlCollection;

namespace Meta
{
    class SqlTrack;
    class SqlArtist;
    class SqlAlbum;
}

class AMAROK_SQLCOLLECTION_EXPORT_TESTS TrackCapabilityDelegate
{
public:
    TrackCapabilityDelegate() {};
    virtual ~ TrackCapabilityDelegate() {};

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlTrack *track ) const = 0;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlTrack *track ) = 0;
};

class AMAROK_SQLCOLLECTION_EXPORT_TESTS ArtistCapabilityDelegate
{
public:
    ArtistCapabilityDelegate() {};
    virtual ~ArtistCapabilityDelegate() {};

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlArtist *artist ) const = 0;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlArtist *artist ) = 0;
};

class AMAROK_SQLCOLLECTION_EXPORT_TESTS AlbumCapabilityDelegate
{
public:
    AlbumCapabilityDelegate() {};
    virtual ~AlbumCapabilityDelegate() {};

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlAlbum *album ) const = 0;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlAlbum *album ) = 0;
};

class AMAROK_SQLCOLLECTION_EXPORT_TESTS CollectionCapabilityDelegate
{
public:
    CollectionCapabilityDelegate() {};
    virtual ~ CollectionCapabilityDelegate() {};

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type, const SqlCollection *collection ) const = 0;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type, SqlCollection *collection ) = 0;
};


#endif // CAPABILITYDELEGATE_H
