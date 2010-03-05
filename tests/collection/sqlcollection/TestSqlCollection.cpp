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

#include "collection/Collection.h"
#include "DatabaseUpdater.h"
#include "Debug.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlCollection )

TestSqlCollection::TestSqlCollection()
{
}

void
TestSqlCollection::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    m_mpmMock = new SqlMountPointManagerMock();
    m_collection->setMountPointManager( m_mpmMock );
    DatabaseUpdater updater;
    updater.setStorage( m_storage );
    updater.setCollection( m_collection );
    updater.update();

    m_storage->query( "INSERT INTO urls(id, deviceid, rpath) VALUES (1, 1, './IDoNotExist.mp3');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath) VALUES (2, 2, './IDoNotExistAsWell.mp3');" );

    m_storage->query( "INSERT INTO tracks(id, url,title) VALUES ( 1,1,'test1');" );
}

void
TestSqlCollection::cleanupTestCase()
{
    delete m_collection;
    //m_mpMock is deleted by SqlCollection
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;

}

void
TestSqlCollection::testDeviceAddedWithTracks()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    m_mpmMock->emitDeviceAdded( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollection::testDeviceAddedWithoutTracks()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    m_mpmMock->emitDeviceAdded( 2 );
    QCOMPARE( spy.count(), 0 );
}

void
TestSqlCollection::testDeviceRemovedWithTracks()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    m_mpmMock->emitDeviceRemoved( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollection::testDeviceRemovedWithoutTracks()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    m_mpmMock->emitDeviceRemoved( 0 );
    QCOMPARE( spy.count(), 0 );
}

#include "TestSqlCollection.moc"
