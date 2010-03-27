/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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
 
#include "core/collections/Collection.h"

#include "core/collections/CollectionLocation.h"
#include "core/meta/Meta.h"

Amarok::CollectionFactory::CollectionFactory() : QObject()
{
}

Amarok::CollectionFactory::~CollectionFactory()
{
}


Amarok::TrackProvider::TrackProvider()
{
}

Amarok::TrackProvider::~TrackProvider()
{
}

bool
Amarok::TrackProvider::possiblyContainsTrack( const KUrl &url ) const
{
    Q_UNUSED( url )
    return false;
}

Meta::TrackPtr
Amarok::TrackProvider::trackForUrl( const KUrl &url )
{
    Q_UNUSED( url )
    return Meta::TrackPtr();
}

// CollectionBase

bool
Amarok::CollectionBase::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    Q_UNUSED( type );
    return false;
}

Capabilities::Capability*
Amarok::CollectionBase::createCapabilityInterface( Capabilities::Capability::Type type )
{
    Q_UNUSED( type );
    return 0;
}

// Collection

Amarok::Collection::Collection()
    : QObject()
    , Amarok::TrackProvider()
{
}

Amarok::Collection::~Collection()
{
}

QString
Amarok::Collection::uidUrlProtocol() const
{
    return QString();
}

CollectionLocation*
Amarok::Collection::location() const
{
    return new CollectionLocation( this );
}

bool
Amarok::Collection::isWritable() const
{
    CollectionLocation* loc = this->location();
    bool writable = loc->isWritable();
    delete loc;
    return writable;
}

bool
Amarok::Collection::isOrganizable() const
{
    CollectionLocation* loc = this->location();
    bool org = loc->isOrganizable();
    delete loc;
    return org;  
}

#include "Collection.moc"
