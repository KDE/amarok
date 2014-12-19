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

Collections::CollectionFactory::CollectionFactory( QObject *parent, const QVariantList &args )
    : Plugins::PluginFactory( parent, args )
{
}

Collections::CollectionFactory::~CollectionFactory()
{
}


Collections::TrackProvider::TrackProvider()
{
}

Collections::TrackProvider::~TrackProvider()
{
}

bool
Collections::TrackProvider::possiblyContainsTrack( const KUrl &url ) const
{
    Q_UNUSED( url )
    return false;
}

Meta::TrackPtr
Collections::TrackProvider::trackForUrl( const KUrl &url )
{
    Q_UNUSED( url )
    return Meta::TrackPtr();
}

// Collection

Collections::Collection::~Collection()
{
}

QString
Collections::Collection::uidUrlProtocol() const
{
    return QString();
}

Collections::CollectionLocation*
Collections::Collection::location()
{
    return new Collections::CollectionLocation( this );
}

bool
Collections::Collection::isWritable() const
{
    Collections::CollectionLocation* loc = const_cast<Collections::Collection *>(this)->location();
    if( !loc )
        return false;
    bool writable = loc->isWritable();
    delete loc;
    return writable;
}

bool
Collections::Collection::isOrganizable() const
{
    Collections::CollectionLocation* loc = const_cast<Collections::Collection *>(this)->location();
    if( !loc )
        return false;
    bool organizable = loc->isOrganizable();
    delete loc;
    return organizable;
}

#include "Collection.moc"
