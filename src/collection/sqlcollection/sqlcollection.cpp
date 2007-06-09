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
#include "sqlitecollection.h"
#include "mysqlcollection.h"
#include "postgresqlcollection.h"

#include <klocale.h>

#include <QTimer>

AMAROK_EXPORT_PLUGIN( SqlCollectionFactory )

void
SqlCollectionFactory::init()
{
    Collection* collection;
    switch( CollectionDB::instance()->getDbConnectionType() )
    {
        case DbConnection::sqlite :
            collection = new SqliteCollection( "localCollection", i18n( "Local collection" ) );
            break;
        case DbConnection::mysql :
            collection = new MySqlCollection( "localCollection", i18n( "Local collection" ) );
            break;
        case DbConnection::postgresql :
            collection = new PostgreSqlCollection( "localCollection", i18n( "Local collection" ) );
            break;
        default :
            collection = new SqlCollection( "localCollection", i18n( "Local collection" ) );
            break;
    }
    emit newCollection( collection );
    QTimer::singleShot( 30000, this, SLOT( testMultipleCollections() ) );
}

void
SqlCollectionFactory::testMultipleCollections()
{
    SqlCollection* secondCollection = new SqlCollection( "anotherLocalCollection", "2nd local collection" );
    m_secondCollection = secondCollection;
    emit newCollection( secondCollection );
    QTimer::singleShot( 30000, this, SLOT( removeSecondCollection() ) );
}

void
SqlCollectionFactory::removeSecondCollection()
{
    m_secondCollection->removeCollection();
    m_secondCollection = 0;
    QTimer::singleShot( 30000, this, SLOT( testMultipleCollections() ) );
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
SqlCollection::queryMaker()
{
    return new SqlQueryBuilder( this );
}

SqlRegistry*
SqlCollection::registry() const
{
    return m_registry;
}

void
SqlCollection::removeCollection()
{
    emit remove();
}

bool
SqlCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "file";
}

Meta::TrackPtr
SqlCollection::trackForUrl( const KUrl &url )
{
    return m_registry->getTrack( url.path() );
}

QStringList
SqlCollection::query( const QString &statement )
{
    return m_collectionDb->query( statement );
}


QString
SqlCollection::escape( QString text ) const           //krazy:exclude=constref
{
    return text.replace( '\'', "''" );;
}


int
SqlCollection::sqlDatabasePriority() const
{
    return 1;
}

QString
SqlCollection::type() const
{
    return "sql";
}

int
SqlCollection::insert( const QString &statement, const QString &table )
{
    return CollectionDB::instance()->insert( statement, table );
}

QString
SqlCollection::boolTrue() const
{
    return "1";
}

QString
SqlCollection::boolFalse() const
{
    return "0";
}

QString
SqlCollection::textColumnType( int length ) const
{
    return QString( "VARCHAR(%1)" ).arg( length );
}

QString
SqlCollection::exactTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
SqlCollection::longTextColumnType() const
{
    return "TEXT";
}

QString
SqlCollection::randomFunc() const
{
    return "RAND()";
}

#include "sqlcollection.moc"

