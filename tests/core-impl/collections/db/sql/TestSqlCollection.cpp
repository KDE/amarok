/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlCollection.h"

#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "../SqlMountPointManagerMock.h"

QTEST_GUILESS_MAIN( TestSqlCollection )

void
TestSqlCollection::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_mpmMock = new SqlMountPointManagerMock( this, m_storage );
    m_collection->setMountPointManager( m_mpmMock );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath) VALUES (1, 1, './IDoNotExist.mp3');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath) VALUES (2, 2, './IDoNotExistAsWell.mp3');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id, url,title) VALUES ( 1,1,'test1');") );
}
