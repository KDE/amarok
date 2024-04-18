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

#include "TestSqlArtist.h"

#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlMountPointManagerMock.h"


QTEST_GUILESS_MAIN( TestSqlArtist )

QTemporaryDir *TestSqlArtist::s_tmpDir = nullptr;

TestSqlArtist::TestSqlArtist()
    : QObject()
    , m_collection( nullptr )
    , m_storage( nullptr )
{
    std::atexit([]() { delete TestSqlArtist::s_tmpDir; } );
}

void
TestSqlArtist::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );
}

void
TestSqlArtist::cleanupTestCase()
{
    delete m_collection;
}

void
TestSqlArtist::init()
{
    //setup base data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'The Foo');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'No The Foo');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );

    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (1, -1, './IDoNotExist.mp3', 'uid://1');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (2, -1, './IDoNotExistAsWell.mp3', 'uid://2');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (3, -1, './MeNeither.mp3', 'uid:/3');" );
}

void
TestSqlArtist::cleanup()
{
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE urls;" );
    m_storage->query( "TRUNCATE TABLE labels;" );
    m_storage->query( "TRUNCATE TABLE urls_labels;" );
}

void
TestSqlArtist::testSortableName()
{
    Meta::ArtistPtr artistWithThe = m_collection->registry()->getArtist( 1 );
    QCOMPARE( artistWithThe->sortableName(), QString( "Foo, The" ) );

    Meta::ArtistPtr artistWithoutThe = m_collection->registry()->getArtist( 2 );
    QCOMPARE( artistWithoutThe->sortableName(), QString( "No The Foo" ) );
}


