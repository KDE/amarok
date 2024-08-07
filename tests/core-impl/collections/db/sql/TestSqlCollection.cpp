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

#include <core/collections/Collection.h>
#include <core-impl/collections/db/sql/SqlCollection.h>
#include <core-impl/collections/db/sql/DatabaseUpdater.h>
#include <core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h>

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>


QTEST_GUILESS_MAIN( TestSqlCollection )

QTemporaryDir *TestSqlCollection::s_tmpDir = nullptr;

TestSqlCollection::TestSqlCollection()
{
    std::atexit([]() { delete TestSqlCollection::s_tmpDir; } );
}

void
TestSqlCollection::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_mpmMock = new SqlMountPointManagerMock( this, m_storage );
    m_collection->setMountPointManager( m_mpmMock );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath) VALUES (1, 1, './IDoNotExist.mp3');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath) VALUES (2, 2, './IDoNotExistAsWell.mp3');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id, url,title) VALUES ( 1,1,'test1');") );
}

void
TestSqlCollection::cleanupTestCase()
{
    delete m_collection;
    //m_mpMock is deleted by SqlCollection

}

void
TestSqlCollection::testDeviceAddedWithTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceAdded( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollection::testDeviceAddedWithoutTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceAdded( 2 );
    QCOMPARE( spy.count(), 0 );
}

void
TestSqlCollection::testDeviceRemovedWithTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceRemoved( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollection::testDeviceRemovedWithoutTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceRemoved( 0 );
    QCOMPARE( spy.count(), 0 );
}

