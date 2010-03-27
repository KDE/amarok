/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "ServiceCollectionLocation.h"
#include "core/support/Debug.h"
using namespace Meta;

ServiceCollectionLocation::ServiceCollectionLocation()
    : CollectionLocation()
    , m_removeSources( false )
    , m_overwriteFiles( false )
{}

ServiceCollectionLocation::ServiceCollectionLocation( const ServiceCollection* parentCollection )
    : CollectionLocation()
    , m_collection( const_cast<ServiceCollection*>( parentCollection ) )
    , m_removeSources( false )
    , m_overwriteFiles( false )
{}

ServiceCollectionLocation::~ServiceCollectionLocation()
{
}

QString ServiceCollectionLocation::prettyLocation() const
{
    return QString();
}

bool ServiceCollectionLocation::isWritable() const
{
    return false;
}

bool ServiceCollectionLocation::isOrganizable() const
{
    return false;
}

void
ServiceCollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    QMap<Meta::TrackPtr, KUrl> urls;
    foreach( Meta::TrackPtr track, tracks )
    {
        ServiceTrack *servtrack = static_cast<ServiceTrack *>( track.data() );
        if( servtrack->isPlayable() )
            urls.insert( track, servtrack->downloadableUrl() );
    }

    slotGetKIOCopyableUrlsDone( urls );
}
