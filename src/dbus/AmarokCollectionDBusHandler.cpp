/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokCollectionDBusHandler.h"

#include "amarokconfig.h"
#include "App.h"
#include "CollectionAdaptor.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"
#include "collection/sqlcollection/SqlCollectionLocation.h"
#include "Debug.h"

namespace Amarok
{
    AmarokCollectionDBusHandler::AmarokCollectionDBusHandler()
        : QObject(kapp)
    {
        setObjectName("AmarokCollectionDBusHandler");

        new CollectionAdaptor( this );
        QDBusConnection::sessionBus().registerObject("/Collection", this);

    }

    QStringList AmarokCollectionDBusHandler::collectionLocation()
    {
        CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        QStringList result = location->actualLocation();
        delete location;
        return result;
    }

    bool AmarokCollectionDBusHandler::isDirInCollection( const QString& path )
    {
        DEBUG_BLOCK

        KUrl url = KUrl( path );
        KUrl parentUrl;
        foreach( const QString &dir, collectionLocation() )
        {
            debug() << "Collection Location: " << dir;
            debug() << "path: " << path;
            debug() << "scan Recursively: " << AmarokConfig::scanRecursively();
            parentUrl.setPath( dir );
            if ( !AmarokConfig::scanRecursively() )
            {
                if ( ( dir == path ) || ( dir + "/" == path ) )
                    return true;
            }
            else //scan recursively
            {
                if ( parentUrl.isParentOf( path ) )
                    return true;
            }
        }
        return false;
    }
} // namespace Amarok

#include "AmarokCollectionDBusHandler.moc"
