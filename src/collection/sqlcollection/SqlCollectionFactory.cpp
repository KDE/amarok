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
#include "SqlCollection.h"
#include "SqlCollectionLocation.h"
#include "SqlRegistry.h"

class SqlCollectionLocationFactoryImpl : public SqlCollectionLocationFactory
{
public:
    SqlCollectionLocation *createSqlCollectionLocation( const SqlCollection *collection ) const
    {
        return new SqlCollectionLocation( collection );
    }
};

SqlCollectionFactory::SqlCollectionFactory()
{
}

SqlCollection*
SqlCollectionFactory::createSqlCollection( const QString &id, const QString &prettyName ) const
{
    SqlCollection *coll = new SqlCollection( id, prettyName );
    coll->setCapabilityDelegate( new CollectionCapabilityDelegate() );
    coll->setUpdater( new DatabaseUpdater() );
    coll->setRegistry( new SqlRegistry() );
}
