/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlTrack.h"

#include "amarokconfig.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"

#include "SqlCollection.h"

#include "../SqlMountPointManagerMock.h"


QTEST_GUILESS_MAIN( TestSqlTrack )

void
TestSqlTrack::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));

    s_tmpDir = new QTemporaryDir();
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );

    // I just need the table and not the whole playlist manager
    m_storage->query( QStringLiteral("CREATE TABLE playlist_tracks ("
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
TestSqlTrack::cleanup()
{
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE statistics;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE labels;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls_labels;") );
}
