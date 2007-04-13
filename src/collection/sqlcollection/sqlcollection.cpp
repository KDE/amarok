/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "sqlcollection.h"

#include "collectiondb.h"
#include "sqlquerybuilder.h"

#include <klocale.h>

#include <QTimer>

AMAROK_EXPORT_PLUGIN( SqlCollectionFactory )

void
SqlCollectionFactory::init()
{
    Collection* collection = new SqlCollection( "localCollection", i18n( "Local collection" ) );
    emit newCollection( collection );
    QTimer::singleShot( 30000, this, SLOT( testMultipleCollections() ) );
}

void
SqlCollectionFactory::testMultipleCollections()
{
    Collection* secondCollection = new SqlCollection( "anotherLocalCollection", "2nd local collection" );
    emit newCollection( secondCollection );
}

SqlCollection::SqlCollection( const QString &id, const QString &prettyName )
    : Collection()
    , m_collectionDb( CollectionDB::instance() )
    , m_registry( new SqlRegistry( this ) )
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}

SqlCollection::~SqlCollection()
{
    delete m_registry;
}

QString
SqlCollection::collectionId() const
{
    return m_collectionId;
}

QString
SqlCollection::prettyName() const
{
    return m_prettyName;
}

QueryMaker*
SqlCollection::queryBuilder()
{
    return new SqlQueryBuilder( this );
}

QStringList
SqlCollection::query( const QString &statement )
{
    return m_collectionDb->query( statement );
}

SqlRegistry*
SqlCollection::registry() const
{
    return m_registry;
}

#include "sqlcollection.moc"

