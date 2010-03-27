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
#include "core/support/Debug.h"

SqlCollectionDBusHandler::SqlCollectionDBusHandler( QObject *parent )
    : QObject( parent )
    , m_collection( 0 )
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
    if( m_collection )
        return m_collection->isDirInCollection( path );
    else
        return false;
}

#include "SqlCollectionDBusHandler.moc"
