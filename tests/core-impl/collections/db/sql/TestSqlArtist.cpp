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
#include "core-impl/storage/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlMountPointManagerMock.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlArtist )

TestSqlArtist::TestSqlArtist()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
{
}

void
TestSqlArtist::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );
}

void
TestSqlArtist::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    delete m_tmpDir;
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


#include "TestSqlArtist.moc"
