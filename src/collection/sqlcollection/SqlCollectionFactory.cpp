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

#include "SqlCollectionFactory.h"

#include "CapabilityDelegate.h"
#include "DatabaseUpdater.h"
#include "ScanManager.h"
#include "SqlCollection.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "MountPointManager.h"

class SqlCollectionLocationFactoryImpl : public SqlCollectionLocationFactory
{
public:
    SqlCollectionLocationFactoryImpl()
        : SqlCollectionLocationFactory()
        , m_collection( 0 ) {}

    SqlCollectionLocation *createSqlCollectionLocation() const
    {
        Q_ASSERT( m_collection );
        return new SqlCollectionLocation( m_collection );
    }

    SqlCollection *m_collection;
};

class SqlQueryMakerFactoryImpl : public SqlQueryMakerFactory
{
public:
    SqlQueryMakerFactoryImpl()
        : SqlQueryMakerFactory()
        , m_collection( 0 ) {}

    SqlQueryMaker *createQueryMaker() const
    {
        Q_ASSERT( m_collection );
        return new SqlQueryMaker( m_collection );
    }

    SqlCollection *m_collection;
};

SqlCollectionFactory::SqlCollectionFactory()
{
}

SqlCollection*
SqlCollectionFactory::createSqlCollection( const QString &id, const QString &prettyName, SqlStorage *storage ) const
{
    SqlCollection *coll = new SqlCollection( id, prettyName );
    coll->setCapabilityDelegate( new CollectionCapabilityDelegate() );
    DatabaseUpdater *updater = new DatabaseUpdater();
    updater->setStorage( storage );
    updater->setCollection( coll );
    coll->setUpdater( updater );
    ScanManager *scanMgr = new ScanManager( coll );
    scanMgr->setCollection( coll );
    scanMgr->setStorage( storage );
    scanMgr->setMountPointManager( MountPointManager::instance() );
    coll->setScanManager( scanMgr );
    coll->setSqlStorage( storage );
    SqlRegistry *registry = new SqlRegistry( coll );
    registry->setMountPointManager( MountPointManager::instance() );
    registry->setStorage( storage );
    coll->setRegistry( registry );

    SqlCollectionLocationFactoryImpl *clFactory = new SqlCollectionLocationFactoryImpl();
    clFactory->m_collection = coll;
    coll->setCollectionLocationFactory( clFactory );
    SqlQueryMakerFactoryImpl *qmFactory = new SqlQueryMakerFactoryImpl();
    qmFactory->m_collection = coll;
    coll->setQueryMakerFactory( qmFactory );

    //everything has been set up
    coll->init();
    return coll;
}
