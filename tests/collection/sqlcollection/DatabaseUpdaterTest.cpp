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

#include "DatabaseUpdaterTest.h"

#include "Debug.h"
#include "DatabaseUpdater.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"

#include <KTempDir>

#include <QMutex>
#include <QString>
#include <QStringList>

//required for Debug.h
QMutex Debug::mutex;

DatabaseUpdaterTest::DatabaseUpdaterTest()
{
}

void
DatabaseUpdaterTest::testNeedsUpdate()
{
    KTempDir tempDir;
    MySqlEmbeddedStorage storage( tempDir.name() );
    DatabaseUpdater updater;
    updater.setStorage( &storage );
    QVERIFY( updater.needsUpdate() );
}

void
DatabaseUpdaterTest::testNeedsNoUpdate()
{
    KTempDir tempDir;
    MySqlEmbeddedStorage storage( tempDir.name() );
    DatabaseUpdater updater;
    updater.setStorage( &storage );
    storage.query( "CREATE TABLE admin (component VARCHAR(40), version INTEGER);" );
    storage.query( "INSERT INTO admin(component, version) VALUES('DB_VERSION', 12);");
    QVERIFY( !updater.needsUpdate() );
}

void
DatabaseUpdaterTest::testCreatePermanentTables()
{
    KTempDir tempDir;
    MySqlEmbeddedStorage storage( tempDir.name() );
    QStringList rs = storage.query( "select * from admin" );
    qDebug() << rs;
    DatabaseUpdater updater;
    updater.setStorage( &storage );
    updater.update();
    QStringList tables = storage.query( "select table_name from INFORMATION_SCHEMA.tables WHERE table_schema='amarok'" );
    //this test currently fails as the MySql connection opened in previous test methods has not been
    //properly closed yet. In particular, the database still contains the table and row created/inserted
    //in the "testsNeedsNoUpdate" test, which stops DatabaseUpdater from creating all tables (correctly).

    //MySqlEmbeddedStorage needs to be fixed so that it really destroys the connection to the embedded database
    //and creates a new one for this test. otherwise we need multiple tests.
    QEXPECT_FAIL( "", "Connection to MySql Embedded database not closed completely", Abort );
    QCOMPARE( tables.count(), 17 );
}

#include "DatabaseUpdaterTest.moc"
