/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Nikolaj Hald Nielsenn <nhnFreespirit@gmail.com>
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

#include "servicesqlcollection.h"

#include "collectionmanager.h"
#include "servicesqlquerymaker.h"
#include "SqlStorage.h"

#include <klocale.h>

#include <QTimer>


ServiceSqlCollection::ServiceSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory )
    : Collection()
    , m_metaFactory( metaFactory )
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}

ServiceSqlCollection::~ServiceSqlCollection()
{
}

QString
ServiceSqlCollection::collectionId() const
{
    return m_collectionId;
}

QString
ServiceSqlCollection::prettyName() const
{
    return m_prettyName;
}

QueryMaker*
ServiceSqlCollection::queryMaker()
{
    return new ServiceSqlQueryMaker( this, m_metaFactory );
}

QStringList
ServiceSqlCollection::query( const QString &statement )
{
    return CollectionManager::instance()->sqlStorage()->query( statement );
}

int
ServiceSqlCollection::insert( const QString &statement, const QString &table )
{
    return CollectionManager::instance()->sqlStorage()->insert( statement, table );
}


QString
ServiceSqlCollection::escape( QString text ) const           //krazy:exclude=constref
{
    return CollectionManager::instance()->sqlStorage()->escape( text );
}

#include "servicesqlcollection.moc"

