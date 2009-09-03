/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "SqlCollectionDBusHandler.h"

#include "SqlCollectionAdaptor.h"
#include "SqlCollection.h"
#include "Debug.h"

SqlCollectionDBusHandler::SqlCollectionDBusHandler( SqlCollection *coll )
    : QObject( coll )
    , m_collection( coll )
{
    DEBUG_BLOCK

    setObjectName("SqlCollectionDBusHandler");

    new SqlCollectionAdaptor( this );
    bool result = QDBusConnection::sessionBus().registerObject( "/SqlCollection", this );
    //-" + coll->collectionId() + "-" + QString::number( QApplication::applicationPid() ), this);
    debug() << "Register object: " << result;
}

bool
SqlCollectionDBusHandler::isDirInCollection( const QString& path )
{
    return m_collection->isDirInCollection( path );
}

#include "SqlCollectionDBusHandler.moc"
