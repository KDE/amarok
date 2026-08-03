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

#include "TestSqlScanManager.h"

#include "config-amarok-test.h"
#include "amarokconfig.h"
#include "scanner/GenericScanManager.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "../SqlMountPointManagerMock.h"

QTEST_GUILESS_MAIN( TestSqlScanManager )

void
TestSqlScanManager::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));
    m_autoGetCoverArt = AmarokConfig::autoGetCoverArt();
    AmarokConfig::setAutoGetCoverArt( false );

    // setenv( "LC_ALL", "", 1 ); // this breaks the test
    // Amarok does not force LC_ALL=C but obviously the test does it which
    // will prevent scanning of files with umlauts.

    //Tell GenericScanManager that we want to use the recently built scanner, not an installed version.
    const QString overridePath = QStringLiteral( AMAROK_OVERRIDE_UTILITIES_PATH );
    qApp->setProperty( "overrideUtilitiesPath", overridePath );

    // that is the original mp3 file that we use to generate the "real" tracks
    m_sourcePath = QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QStringLiteral("/data/audio/Platz 01.mp3") );
    QVERIFY( QFile::exists( m_sourcePath ) );

    if( !s_tmpDatabaseDir )
        s_tmpDatabaseDir = new QTemporaryDir();
    QVERIFY( s_tmpDatabaseDir->isValid() );
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDatabaseDir->path() ) );

    m_collection = new Collections::SqlCollection( m_storage );
    connect( m_collection, &Collections::SqlCollection::updated, this, &TestSqlScanManager::slotCollectionUpdated );

    // TODO: change the mock mount point manager so that it doesn't pull
    //       in all the devices. Not much of a mock like this.
    SqlMountPointManagerMock *mock = new SqlMountPointManagerMock( this, m_storage );
    m_collection->setMountPointManager( mock );
    m_scanManager = m_collection->scanManager();

    AmarokConfig::setScanRecursively( true );
    AmarokConfig::setMonitorChanges( false );

    // switch on writing back so that we can create the test files with all the information
    AmarokConfig::setWriteBack( true );
    AmarokConfig::setWriteBackStatistics( true );
    AmarokConfig::setWriteBackCover( true );

    // I just need the table and not the whole playlist manager
    /*
    m_storage->query( QString( "CREATE TABLE playlist_tracks ("
            " id " + m_storage->idType() +
            ", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url " + m_storage->exactTextColumnType() +
            ", title " + m_storage->textColumnType() +
            ", album " + m_storage->textColumnType() +
            ", artist " + m_storage->textColumnType() +
            ", length INTEGER "
            ", uniqueid " + m_storage->textColumnType(128) + ") ENGINE = MyISAM;" ) );
            */
}

void
TestSqlScanManager::cleanup()
{
    m_scanManager->abort();

    m_storage->query( QStringLiteral("BEGIN") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE statistics;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("COMMIT") );
    TestSqlScanManagerBase::cleanup(); // call SqlRegistry emptyCache() from a friend class

    delete m_tmpCollectionDir;
}
