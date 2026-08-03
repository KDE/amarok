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

#include "TestSqlCollectionLocation.h"

#include "core/support/Components.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "../SqlMountPointManagerMock.h"
#include "core/collections/MockCollectionLocationDelegate.h"

QTEST_GUILESS_MAIN( TestSqlCollectionLocation )

void
TestSqlCollectionLocation::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    SqlMountPointManagerMock *mock = new SqlMountPointManagerMock( this, m_storage );
    mock->setCollectionFolders( QStringList() << s_tmpDir->path() ); // the target folder needs to have enough space and be writable
    m_collection->setMountPointManager( mock );

    // I just need the table and not the whole playlist manager
    m_storage->query( QStringLiteral( "CREATE TABLE playlist_tracks ("
            " id ") + m_storage->idType() +
            QStringLiteral(", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url ") + m_storage->exactTextColumnType() +
            QStringLiteral(", title ") + m_storage->textColumnType() +
            QStringLiteral(", album ") + m_storage->textColumnType() +
            QStringLiteral(", artist ") + m_storage->textColumnType() +
            QStringLiteral(", length INTEGER "
            ", uniqueid ") + m_storage->textColumnType(128) + QStringLiteral(") ENGINE = MyISAM;" ) );
}

void
TestSqlCollectionLocation::cleanup()
{
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE labels;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls_labels;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
}
