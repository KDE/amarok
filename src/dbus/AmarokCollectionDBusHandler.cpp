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
#include "Collection.h"
#include "CollectionAdaptor.h"
#include "collection/CollectionManager.h"
#include "collection/sqlcollection/SqlCollection.h"
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

    bool AmarokCollectionDBusHandler::isDirInCollection( const QString& path )
    {
        DEBUG_BLOCK

        // Assume the primary location is the sql collection. This is true for now, and we do this in other places too

        Collection *coll = CollectionManager::instance()->primaryCollection();
        SqlCollection* sqlColl = (SqlCollection*)( coll ); //FIXME: god-cast is bad. dynamic_cast won't compile for me unfortunately.
        if( !sqlColl )
            return false;
        return sqlColl->isDirInCollection( path );
    }
} // namespace Amarok

#include "AmarokCollectionDBusHandler.moc"
