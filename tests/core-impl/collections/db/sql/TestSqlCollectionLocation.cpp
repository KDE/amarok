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

#include "DatabaseUpdater.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core-impl/logger/ProxyLogger.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlCollectionLocation.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"
#include "core/collections/MockCollectionLocationDelegate.h"

#include "config-amarok-test.h"

#include <QMetaObject>
#include <QProcess>
#include <QTimer>

#include <KCmdLineArgs>
#include <KGlobal>

#include <qtest_kde.h>

#include <gmock/gmock.h>

using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::_;

/** A SqlCollectionLocation that claims writing is possible even though it doesn't have
 *  a valid directory.
 */
class MySqlCollectionLocation : public Collections::SqlCollectionLocation
{
public:
    MySqlCollectionLocation( Collections::SqlCollection *coll ) : Collections::SqlCollectionLocation( coll ) {}
    virtual ~MySqlCollectionLocation() {}

    bool isWritable() const { return true; }
};

class MyOrganizeCollectionDelegate : public OrganizeCollectionDelegate
{
public:
    MyOrganizeCollectionDelegate() : OrganizeCollectionDelegate(), overwrite( false ), migrate( false ) {}
    virtual ~ MyOrganizeCollectionDelegate() {}

    void setTracks( const Meta::TrackList &tracks ) { Q_UNUSED( tracks ) }
    void setFolders( const QStringList &folders ) { Q_UNUSED( folders ) }
    void setIsOrganizing( bool organizing ) { Q_UNUSED( organizing ) }
    void setTranscodingConfiguration(const Transcoding::Configuration &configuration)
    { Q_UNUSED( configuration ) }
    void setCaption( const QString& ) {}

    void show() { emit accepted(); }

    bool overwriteDestinations() const { return overwrite; }
    QMap<Meta::TrackPtr, QString> destinations() const { return dests; }
    bool migrateLabels() const { return migrate; }

    bool overwrite;
    bool migrate;
    QMap<Meta::TrackPtr, QString> dests;
};

class MyOrganizeCollectionDelegateFactory : public OrganizeCollectionDelegateFactory
{
public:
    MyOrganizeCollectionDelegateFactory( OrganizeCollectionDelegate *d )
        : OrganizeCollectionDelegateFactory()
        , delegate( d ) {}
    virtual ~ MyOrganizeCollectionDelegateFactory() {}

    //warning: SqlCollectionLocation will delete the delegate
    OrganizeCollectionDelegate* createDelegate() { return delegate; }

    OrganizeCollectionDelegate *delegate;
};



QTEST_KDEMAIN_CORE( TestSqlCollectionLocation )

TestSqlCollectionLocation::TestSqlCollectionLocation()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
}

void
TestSqlCollectionLocation::initTestCase()
{
    Amarok::Components::setLogger( new ProxyLogger() );
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new Collections::SqlCollection( m_storage );
    SqlMountPointManagerMock *mock = new SqlMountPointManagerMock( this, m_storage );
    mock->setCollectionFolders( QStringList() << m_tmpDir->name() ); // the target folder needs to have enough space and be writable
    m_collection->setMountPointManager( mock );

    // I just need the table and not the whole playlist manager
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
}

void
TestSqlCollectionLocation::cleanupTestCase()
{
    delete m_collection;
    delete Amarok::Components::setLogger( 0 );
    //m_storage is deleted by SqlCollection
    delete m_tmpDir;
}

void
TestSqlCollectionLocation::init()
{
    //setup base data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'artist1');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'artist2');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);" );

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );

    m_storage->query( "INSERT INTO directories(id,deviceid,dir) VALUES (1, -1, '." + m_tmpDir->name() + "ab/')");
    m_storage->query( "INSERT INTO directories(id,deviceid,dir) VALUES (2, -1, '." + m_tmpDir->name() + "b/')");
    m_storage->query( "INSERT INTO directories(id,deviceid,dir) VALUES (3, -1, '." + m_tmpDir->name() + "c/')");

    m_storage->query( QString( "INSERT INTO urls(id, deviceid, rpath, uniqueid, directory ) VALUES (1, -1, '%1', 'uid://1', 1);" ).arg( setupFileInTempDir( "ab/IDoNotExist.mp3" ) ) );
    m_storage->query( QString( "INSERT INTO urls(id, deviceid, rpath, uniqueid, directory ) VALUES (2, -1, '%1', 'uid://2', 2);" ).arg( setupFileInTempDir( "b/IDoNotExistAsWell.mp3") ) );
    m_storage->query( QString( "INSERT INTO urls(id, deviceid, rpath, uniqueid, directory ) VALUES (3, -1, '%1', 'uid:/3', 3);" ).arg( setupFileInTempDir( "c/MeNeither.mp3" ) ) );

    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',2,3,1,1,1);" );

    m_collection->registry()->emptyCache();
}

void
TestSqlCollectionLocation::cleanup()
{
    delete Amarok::Components::setCollectionLocationDelegate( 0 );
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE urls;" );
    m_storage->query( "TRUNCATE TABLE labels;" );
    m_storage->query( "TRUNCATE TABLE urls_labels;" );
    m_storage->query( "TRUNCATE TABLE directories;" );
}

void
TestSqlCollectionLocation::testOrganizingCopiesLabels()
{
    {
        Collections::MockCollectionLocationDelegate *d = new Collections::MockCollectionLocationDelegate();
        EXPECT_CALL( *d, reallyMove( _, _ ) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
        EXPECT_CALL( *d, transcode( _, _, _, _, _ ) ).WillOnce( Return(
                Transcoding::Configuration( Transcoding::INVALID ) ) );
        Amarok::Components::setCollectionLocationDelegate( d );
    }

    {
        Meta::TrackPtr track = m_collection->registry()->getTrackFromUid( "uid://1" );
        QVERIFY( track );
        QVERIFY( track->playableUrl().path().endsWith( "ab/IDoNotExist.mp3" ) );

        track->addLabel( "test" );

        QCOMPARE( track->labels().count(), 1 );

        Collections::SqlCollectionLocation *source = new MySqlCollectionLocation( m_collection );
        Collections::SqlCollectionLocation *dest = new MySqlCollectionLocation( m_collection );

        {
            MyOrganizeCollectionDelegate *delegate = new MyOrganizeCollectionDelegate();
            delegate->overwrite = true;
            delegate->migrate = true;
            delegate->dests.insert( track, m_tmpDir->name() + "b/IDoNotExist.mp3" );
            dest->setOrganizeCollectionDelegateFactory( new MyOrganizeCollectionDelegateFactory( delegate ) );
        }

        source->prepareMove( track, dest );

        QTest::kWaitForSignal( source, SIGNAL(destroyed(QObject*)), 1000 );

        QCOMPARE( track->labels().count(), 1 );
        QVERIFY( track->playableUrl().path().endsWith( "b/IDoNotExist.mp3" ) );
    }

    //force a reload from the database
    m_collection->registry()->emptyCache();

    {
        // Meta::TrackPtr track = m_collection->registry()->getTrack( m_tmpDir->name() + "b/IDoNotExist.mp3" );
        Meta::TrackPtr track = m_collection->registry()->getTrack(1);
        QVERIFY( track );
        QVERIFY( track->playableUrl().path().endsWith( "b/IDoNotExist.mp3" ) );
        // TODO: check that the db urls entry really specifies the exiting directories entry
        QCOMPARE( track->labels().count(), 1 );
    }
}

void
TestSqlCollectionLocation::testCopiesLabelFromExternalTracks()
{

}

void
TestSqlCollectionLocation::testCopyTrackToDirectoryWithExistingTracks()
{

}

QString
TestSqlCollectionLocation::setupFileInTempDir( const QString &relativeName )
{
    QString absoluteName = m_tmpDir->name() + relativeName;

    //TODO: unix specific
    //create directory where necessary
    int index = absoluteName.lastIndexOf( QDir::separator() );
    if(index > 0 )
    {
        QString dir = absoluteName.left( index );
        QProcess::execute( "mkdir", QStringList() << "-p" << dir );
    }
    else
    {
        qDebug() << "huh? index was " << index << " relative name was " << relativeName << " tmpDir " << m_tmpDir->name();
    }

    QProcess::execute( "touch", QStringList() << absoluteName );
    return '.' + absoluteName;
}
