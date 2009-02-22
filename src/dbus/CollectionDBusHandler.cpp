/*
 *  Copyright 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include "CollectionDBusHandler.h"

#include "CollectionAdaptor.h"
#include "collection/support/XmlQueryReader.h"
#include "dbus/DBusQueryHelper.h"
#include "Debug.h"

#include <QUuid>

class QueryMaker;

CollectionDBusHandler::CollectionDBusHandler( QObject *parent )
    : QObject( parent )
{
    setObjectName("CollectionDBusHandler");
    
    new CollectionAdaptor( this );
    QDBusConnection::sessionBus().registerObject("/Collection", this);
}

QString
CollectionDBusHandler::query( const QString &xmlQuery )
{
    QueryMaker* qm = XmlQueryReader::getQueryMaker( xmlQuery, XmlQueryReader::IgnoreReturnValues );
    
    //probably invalid XML
    if( !qm )
    {
        debug() << "Invalid XML query: " << xmlQuery;
        return QString();
    }
        
    const QString token = QUuid::createUuid().toString();
    DBusQueryHelper *helper = new DBusQueryHelper( this, qm, token );
    connect( helper, SIGNAL( queryResult( QString, QList<QMap<QString, QVariant> > ) ), this, SIGNAL( queryResult( QString, QList<QMap<QString, QVariant> > ) ) );
    
    return token;
}
