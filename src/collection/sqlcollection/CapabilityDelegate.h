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

#include "meta/Capability.h"

#include <QAction>

class SqlCollection;

namespace Meta
{
    class SqlTrack;
    class SqlArtist;
    class SqlAlbum;
}

class TrackCapabilityDelegate
{
public:
    TrackCapabilityDelegate();

    bool hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlTrack *track ) const;
    Meta::Capability* createCapabilityInterface( Meta::Capability::Type type, Meta::SqlTrack *track );
};

class ArtistCapabilityDelegate
{
public:
    ArtistCapabilityDelegate();
    ~ArtistCapabilityDelegate();

    bool hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlArtist *artist ) const;
    Meta::Capability* createCapabilityInterface( Meta::Capability::Type type, Meta::SqlArtist *artist );

private:
    QAction * m_bookmarkAction;
};

class AlbumCapabilityDelegate
{
public:
    AlbumCapabilityDelegate();
    ~AlbumCapabilityDelegate();

    bool hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlAlbum *album ) const;
    Meta::Capability* createCapabilityInterface( Meta::Capability::Type type, Meta::SqlAlbum *album );

private:
    QAction * m_bookmarkAction;
};

class CollectionCapabilityDelegate
{
public:
    CollectionCapabilityDelegate();

    bool hasCapabilityInterface( Meta::Capability::Type type, const SqlCollection *collection ) const;
    Meta::Capability* createCapabilityInterface( Meta::Capability::Type type, SqlCollection *collection );
};


#endif // CAPABILITYDELEGATE_H
