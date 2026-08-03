/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2026 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#include "TestSqlCollectionBase.h"

#include <core/collections/Collection.h>
#include <core-impl/collections/db/sql/SqlCollection.h>
#include <core-impl/collections/db/sql/DatabaseUpdater.h>

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>


QTemporaryDir *TestSqlCollectionBase::s_tmpDir = nullptr;

TestSqlCollectionBase::TestSqlCollectionBase()
{
    std::atexit([]() { delete TestSqlCollectionBase::s_tmpDir; } );
}

void
TestSqlCollectionBase::cleanupTestCase()
{
    delete m_collection;
    //m_mpMock is deleted by SqlCollection

}

void
TestSqlCollectionBase::testDeviceAddedWithTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceAdded( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollectionBase::testDeviceAddedWithoutTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceAdded( 2 );
    QCOMPARE( spy.count(), 0 );
}

void
TestSqlCollectionBase::testDeviceRemovedWithTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceRemoved( 1 );
    QCOMPARE( spy.count(), 1 );
}

void
TestSqlCollectionBase::testDeviceRemovedWithoutTracks()
{
    QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
    m_mpmMock->emitDeviceRemoved( 0 );
    QCOMPARE( spy.count(), 0 );
}

