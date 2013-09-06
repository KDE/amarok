/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "TestImporterSqlProvider.h"

#include "importers/ImporterSqlProvider.h"

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Thread>
#include <ThreadWeaver/Weaver>

#include <QApplication>
#include <QSqlDatabase>

#include <gmock/gmock.h>
#include <qtest_kde.h>

#include <functional>

QTEST_KDEMAIN_CORE( TestImporterSqlProvider )

using namespace StatSyncing;
using namespace ::testing;

class MockImporterSqlProvider : public ImporterSqlProvider
{
public:
    MockImporterSqlProvider( const QVariantMap &cfg )
        : ImporterSqlProvider( cfg, 0 )
    {
    }

    QString connectionName() const { return m_connectionName; }

    MOCK_CONST_METHOD0( reliableTrackMetaData, qint64() );
    MOCK_CONST_METHOD0( writableTrackStatsData, qint64() );
    MOCK_METHOD0( prepareConnection, void() );
    MOCK_METHOD1( getArtists, QSet<QString>( QSqlDatabase db ) );
    MOCK_METHOD2( getArtistTracks,
                  TrackList( const QString &artistName, QSqlDatabase db ) );
};

void
TestImporterSqlProvider::checkThread()
{
    QCOMPARE( m_mock->thread(), ThreadWeaver::Thread::currentThread() );
}

void
TestImporterSqlProvider::init()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QSQLITE" );
    cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                          "/importers_files/collection.db" );
    m_mock = new NiceMock<MockImporterSqlProvider>( cfg );
}

void
TestImporterSqlProvider::cleanup()
{
    delete m_mock;
    m_mock = 0;
}

void
TestImporterSqlProvider::constructorShouldCreateDatabaseConnection()
{
     QVERIFY( QSqlDatabase::contains( m_mock->connectionName() ) );
}

void
TestImporterSqlProvider::createdConnectionShouldHaveProperSettings()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QMYSQL" );
    cfg.insert( "dbName", "myName" );
    cfg.insert( "dbHost", "myHost" );
    cfg.insert( "dbUser", "myUser" );
    cfg.insert( "dbPass", "myPass" );
    cfg.insert( "dbPort", 12345 );

    NiceMock<MockImporterSqlProvider> mock( cfg );
    QSqlDatabase db = QSqlDatabase::database( mock.connectionName(), /*open*/ false );

    QCOMPARE( db.driverName(), cfg.value( "dbDriver" ).toString() );
    QCOMPARE( db.databaseName(), cfg.value( "dbName" ).toString() );
    QCOMPARE( db.hostName(), cfg.value( "dbHost" ).toString() );
    QCOMPARE( db.userName(), cfg.value( "dbUser" ).toString() );
    QCOMPARE( db.password(), cfg.value( "dbPass" ).toString() );
    QCOMPARE( db.port(), cfg.value( "dbPort" ).toInt() );
}

void
TestImporterSqlProvider::createdSQLiteConnectionShouldHavePathSet()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QSQLITE" );
    cfg.insert( "dbName", "myName" );
    cfg.insert( "dbPath", "myPath" );

    NiceMock<MockImporterSqlProvider> mock( cfg );
    QSqlDatabase db = QSqlDatabase::database( mock.connectionName(), /*open*/ false );

    QCOMPARE( db.driverName(), cfg.value( "dbDriver" ).toString() );
    QCOMPARE( db.databaseName(), cfg.value( "dbPath" ).toString() );
}

void
TestImporterSqlProvider::createdNonSQLiteConnectionShouldHaveNameSet()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QPSQL" );
    cfg.insert( "dbName", "myName" );
    cfg.insert( "dbPath", "myPath" );

    NiceMock<MockImporterSqlProvider> mock( cfg );
    QSqlDatabase db = QSqlDatabase::database( mock.connectionName(), /*open*/ false );

    QCOMPARE( db.driverName(), cfg.value( "dbDriver" ).toString() );
    QCOMPARE( db.databaseName(), cfg.value( "dbName" ).toString() );
}

static bool
operator==( const QSqlDatabase &lhs, const QSqlDatabase &rhs )
{
    return lhs.connectionName() == rhs.connectionName();
}

void
TestImporterSqlProvider::createdConnectionIsObjectUnique()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QSQLITE" );
    NiceMock<MockImporterSqlProvider> mock( cfg );

    const QSqlDatabase db1 =
            QSqlDatabase::database( m_mock->connectionName(), /*open*/ false );
    const QSqlDatabase db2 =
            QSqlDatabase::database( mock.connectionName(), /*open*/ false );

    QVERIFY( !(db1 == db2) );
}

void
TestImporterSqlProvider::destructorShouldRemoveDatabaseConnection()
{
    const QString connectionName = m_mock->connectionName();
    delete m_mock;
    m_mock = 0;

    QVERIFY( !QSqlDatabase::contains( connectionName ) );
}

void
TestImporterSqlProvider::getArtistsShouldReceiveCreatedConnection()
{
    QSqlDatabase db = QSqlDatabase::database( m_mock->connectionName(), /*open*/ false );
    EXPECT_CALL( *m_mock, getArtists( Eq( db ) ) ).WillOnce( Return( QSet<QString>() ) );

    m_mock->artists();
}

void
TestImporterSqlProvider::getArtistTracksShouldReceiveCreatedConnection()
{
    QSqlDatabase db = QSqlDatabase::database( m_mock->connectionName(), /*open*/ false );
    EXPECT_CALL( *m_mock, getArtistTracks( _, Eq( db ) ) )
                    .WillOnce( Return( TrackList() ) );

    m_mock->artistTracks( "Artist" );
}

class JobRunArtists : public ThreadWeaver::Job
{
public:
    JobRunArtists( ImporterProvider *provider, QObject* parent = 0 )
        : Job( parent ), m_provider( provider ) {}

protected:
    void run() { m_provider->artists(); }
    ImporterProvider *m_provider;
};

class JobRunArtistTracks : public ThreadWeaver::Job
{
public:
    JobRunArtistTracks( ImporterProvider *provider, QObject* parent = 0 )
        : Job( parent ), m_provider( provider ) {}

protected:
    void run() { m_provider->artistTracks( "artist" ); }
    ImporterProvider *m_provider;
};

void
TestImporterSqlProvider::getArtistsShouldBeCalledInObjectsThread()
{
    EXPECT_CALL( *m_mock, getArtists( _ ) ).WillOnce( DoAll(
                    InvokeWithoutArgs( this, &TestImporterSqlProvider::checkThread ),
                    Return( QSet<QString>()) ) );

    JobRunArtists job( m_mock );
    ThreadWeaver::Weaver::instance()->enqueue( &job );

    if( !ThreadWeaver::Weaver::instance()->isIdle() )
        QVERIFY2( QTest::kWaitForSignal( ThreadWeaver::Weaver::instance(),
                SIGNAL(finished()), 5000 ), "threads did not finish in timeout" );
}

void
TestImporterSqlProvider::getArtistTracksShouldBeCalledInObjectsThread()
{
    EXPECT_CALL( *m_mock, getArtistTracks( _, _ ) ).WillOnce( DoAll(
                    InvokeWithoutArgs( this, &TestImporterSqlProvider::checkThread ),
                    Return( TrackList()) ) );

    JobRunArtistTracks job( m_mock );
    ThreadWeaver::Weaver::instance()->enqueue( &job );

    if( !ThreadWeaver::Weaver::instance()->isIdle() )
        QVERIFY2( QTest::kWaitForSignal( ThreadWeaver::Weaver::instance(),
                SIGNAL(finished()), 5000 ), "threads did not finish in timeout" );
}

void
TestImporterSqlProvider::artistsShouldDelegateToGetArtists()
{
    const QSet<QString> result = QSet<QString>() << "Artist1" << "Artist2" << "Artist3";
    EXPECT_CALL( *m_mock, getArtists( _ ) ).WillOnce( Return( result ) );

    QCOMPARE( m_mock->artists(), result );
}

void
TestImporterSqlProvider::artistTracksShouldDelegateToGetArtistTracks()
{
    const TrackList result = TrackList() << TrackPtr( 0 ) << TrackPtr( 0 );
    EXPECT_CALL( *m_mock, getArtistTracks( _, _ ) ).WillOnce( Return( result ) );

    QCOMPARE( m_mock->artistTracks( "artist" ), result );
}

void
TestImporterSqlProvider::artistsShouldCallPrepareConnectionBeforeGetArtists()
{
    InSequence s;
    Q_UNUSED( s )

    EXPECT_CALL( *m_mock, prepareConnection() );
    EXPECT_CALL( *m_mock, getArtists( _ ) ).WillOnce( Return( QSet<QString>() ) );

    m_mock->artists();
}

void
TestImporterSqlProvider::artistTracksShouldCallPrepareConnectionBeforeGetArtistTracks()
{
    InSequence s;
    Q_UNUSED( s )

    EXPECT_CALL( *m_mock, prepareConnection() );
    EXPECT_CALL( *m_mock, getArtistTracks( _, _ ) ).WillOnce( Return( TrackList() ) );

    m_mock->artistTracks( "artist" );
}

#include "TestImporterSqlProvider.moc"
