/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "TestSqlAlbum.h"

#include "amarokconfig.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "../SqlMountPointManagerMock.h"

QTEST_MAIN( TestSqlAlbum )

void
TestSqlAlbum::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    QVERIFY( s_tmpDir->isValid() );
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );
}

void
TestSqlAlbum::cleanup()
{
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls_labels;") );
}
