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
 
#include "CollectionDBusHandler.h"

#include "CollectionAdaptor.h"
#include "core-impl/collections/support/XmlQueryReader.h"
#include "dbus/DBusQueryHelper.h"
#include "core/support/Debug.h"

class QueryMaker;

CollectionDBusHandler::CollectionDBusHandler( QObject *parent )
    : QObject( parent )
    , QDBusContext()
{
    setObjectName("CollectionDBusHandler");
    qDBusRegisterMetaType<VariantMapList>();
    
    new CollectionAdaptor( this );
    bool result = QDBusConnection::sessionBus().registerObject("/Collection", this);
    debug() << "Register object: " << result;
}

VariantMapList
CollectionDBusHandler::Query( const QString &xmlQuery )
{
    if( !calledFromDBus() )
        return VariantMapList();

    Collections::QueryMaker* qm = XmlQueryReader::getQueryMaker( xmlQuery, XmlQueryReader::IgnoreReturnValues );
    
    //probably invalid XML
    if( !qm )
    {
        debug() << "Invalid XML query: " << xmlQuery;
        sendErrorReply( QDBusError::InvalidArgs, "Invalid XML: " + xmlQuery );
        return VariantMapList();
    }

    setDelayedReply( true );

    new DBusQueryHelper( this, qm, connection(), message(), false );
    
    return VariantMapList();
}

VariantMapList
CollectionDBusHandler::MprisQuery( const QString &xmlQuery )
{
    if( !calledFromDBus() )
        return VariantMapList();

    Collections::QueryMaker* qm = XmlQueryReader::getQueryMaker( xmlQuery, XmlQueryReader::IgnoreReturnValues );

    //probably invalid XML
    if( !qm )
    {
        debug() << "Invalid XML query: " << xmlQuery;
        sendErrorReply( QDBusError::InvalidArgs, "Invalid XML: " + xmlQuery );
        return VariantMapList();
    }

    setDelayedReply( true );

    new DBusQueryHelper( this, qm, connection(), message(), true );

    return VariantMapList();
}
