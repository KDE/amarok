/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "SqlCollection"

#include "SqlCollection.h"

#include "DefaultSqlQueryMakerFactory.h"
#include "DatabaseUpdater.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/capabilities/TranscodeCapability.h"
#include "core/transcoding/TranscodingController.h"
#include "core-impl/collections/db/MountPointManager.h"
#include "scanner/GenericScanManager.h"
#include "scanner/AbstractDirectoryWatcher.h"
#include "dialogs/OrganizeCollectionDialog.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SqlScanResultProcessor.h"
#include "SvgHandler.h"
#include "MainWindow.h"

#include "collectionscanner/BatchFile.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include <KConfigGroup>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>


/** Concrete implementation of the directory watcher */
class SqlDirectoryWatcher : public AbstractDirectoryWatcher
{
public:
    SqlDirectoryWatcher( Collections::SqlCollection* collection )
        : AbstractDirectoryWatcher()
        , m_collection( collection )
    { }

    ~SqlDirectoryWatcher() override
    { }

protected:
    QList<QString> collectionFolders() override
    { return m_collection->mountPointManager()->collectionFolders(); }

    Collections::SqlCollection* m_collection;
};

class SqlScanManager : public GenericScanManager
{
public:
    SqlScanManager( Collections::SqlCollection* collection, QObject* parent )
        : GenericScanManager( parent )
        , m_collection( collection )
    { }

    ~SqlScanManager() override
    { }

protected:
    QList< QPair<QString, uint> > getKnownDirs()
    {
        QList< QPair<QString, uint> > result;

        // -- get all (mounted) mount points
        QList<int> idList = m_collection->mountPointManager()->getMountedDeviceIds();

        // -- query all known directories
        QString deviceIds;
        for( int id : idList )
        {
            if( !deviceIds.isEmpty() )
                deviceIds += QLatin1Char(',');
            deviceIds += QString::number( id );
        }
        QString query = QStringLiteral( "SELECT deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" );

        QStringList values = m_collection->sqlStorage()->query( query.arg( deviceIds ) );
        for( QListIterator<QString> iter( values ); iter.hasNext(); )
        {
            int deviceid = iter.next().toInt();
            QString dir = iter.next();
            uint mtime = iter.next().toUInt();

            QString folder = m_collection->mountPointManager()->getAbsolutePath( deviceid, dir );
            result.append( QPair<QString, uint>( folder, mtime ) );
        }

        return result;
    }

    QString getBatchFile( const QStringList &scanDirsRequested ) override
    {
        // -- write the batch file
        // the batch file contains the known modification dates so that the scanner only
        // needs to report changed directories
        QList<QPair<QString, uint> > knownDirs = getKnownDirs();
        if( !knownDirs.isEmpty() )
        {
            QString path = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + QStringLiteral("/amarok/amarokcollectionscanner_batchscan.xml");
            while( QFile::exists( path ) )
                path += QLatin1Char('_');

            CollectionScanner::BatchFile batchfile;
            batchfile.setTimeDefinitions( knownDirs );
            batchfile.setDirectories( scanDirsRequested );
            if( !batchfile.write( path ) )
            {
                warning() << "Failed to write batch file" << path;
                return QString();
            }
            return path;
        }
        return QString();
    }

    Collections::SqlCollection* m_collection;
};

namespace Collections {

class OrganizeCollectionDelegateImpl : public OrganizeCollectionDelegate
{
public:
    OrganizeCollectionDelegateImpl()
        : OrganizeCollectionDelegate()
        , m_dialog( nullptr )
        , m_organizing( false ) {}
    ~ OrganizeCollectionDelegateImpl() override { delete m_dialog; }

    void setTracks( const Meta::TrackList &tracks ) override { m_tracks = tracks; }
    void setFolders( const QStringList &folders ) override { m_folders = folders; }
    void setIsOrganizing( bool organizing ) override { m_organizing = organizing; }
    void setTranscodingConfiguration( const Transcoding::Configuration &configuration ) override
    { m_targetFileExtension =
      Amarok::Components::transcodingController()->format( configuration.encoder() )->fileExtension(); }
    void setCaption( const QString &caption ) override { m_caption = caption; }

    void show() override
    {
        m_dialog = new OrganizeCollectionDialog( m_tracks,
                    m_folders,
                    m_targetFileExtension,
                    The::mainWindow(), //parent
                    "", //name is unused
                    true, //modal
                    m_caption //caption
                );

        connect( m_dialog, &OrganizeCollectionDialog::accepted,
                 this, &OrganizeCollectionDelegateImpl::accepted );
        connect( m_dialog, &OrganizeCollectionDialog::rejected,
                 this, &OrganizeCollectionDelegateImpl::rejected );
        m_dialog->show();
    }

    bool overwriteDestinations() const override { return m_dialog->overwriteDestinations(); }
    QMap<Meta::TrackPtr, QString> destinations() const override { return m_dialog->getDestinations(); }

private:
    Meta::TrackList m_tracks;
    QStringList m_folders;
    OrganizeCollectionDialog *m_dialog;
    bool m_organizing;
    QString m_targetFileExtension;
    QString m_caption;
};


class OrganizeCollectionDelegateFactoryImpl : public OrganizeCollectionDelegateFactory
{
public:
    OrganizeCollectionDelegate* createDelegate() override { return new OrganizeCollectionDelegateImpl(); }
};


class SqlCollectionLocationFactoryImpl : public SqlCollectionLocationFactory
{
public:
    SqlCollectionLocationFactoryImpl( SqlCollection *collection )
        : SqlCollectionLocationFactory()
        , m_collection( collection ) {}

    SqlCollectionLocation *createSqlCollectionLocation() const override
    {
        Q_ASSERT( m_collection );
        SqlCollectionLocation *loc = new SqlCollectionLocation( m_collection );
        loc->setOrganizeCollectionDelegateFactory( new OrganizeCollectionDelegateFactoryImpl() );
        return loc;
    }

    Collections::SqlCollection *m_collection;
};

} //namespace Collections

using namespace Collections;

SqlCollection::SqlCollection( const QSharedPointer<SqlStorage> &storage )
    : DatabaseCollection()
    , m_registry( nullptr )
    , m_sqlStorage( storage )
    , m_scanProcessor( nullptr )
    , m_directoryWatcher( nullptr )
    , m_collectionLocationFactory( nullptr )
    , m_queryMakerFactory( nullptr )
{
    qRegisterMetaType<TrackUrls>( "TrackUrls" );
    qRegisterMetaType<ChangedTrackUrls>( "ChangedTrackUrls" );

    // update database to current schema version; this must be run *before* MountPointManager
    // is initialized or its handlers may try to insert
    // into the database before it's created/updated!
    DatabaseUpdater updater( this );
    if( updater.needsUpdate() )
    {
        if( updater.schemaExists() ) // this is an update
        {
            QMessageBox dialog;
            dialog.setText( i18n( "Updating Amarok database schema. Please don't terminate "
                                              "Amarok now as it may result in database corruption." ) );
            dialog.setWindowTitle( i18n( "Updating Amarok database schema" ) );

            dialog.setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            dialog.show();
            dialog.raise();
            // otherwise the splash screen doesn't load image and this dialog is not shown:
            qApp->processEvents();

            updater.update();

            dialog.hide();
            qApp->processEvents();
        }
        else // this is new schema creation
            updater.update();
    }

    //perform a quick check of the database
    updater.cleanupDatabase();

    m_registry = new SqlRegistry( this );

    m_collectionLocationFactory = new SqlCollectionLocationFactoryImpl( this );
    m_queryMakerFactory = new DefaultSqlQueryMakerFactory( this );

    // scanning
    m_scanManager = new SqlScanManager( this, this );
    m_scanProcessor = new SqlScanResultProcessor( m_scanManager, this, this );
    auto directoryWatcher = QSharedPointer<SqlDirectoryWatcher>::create( this );
    m_directoryWatcher = directoryWatcher.toWeakRef();
    connect( directoryWatcher.data(), &AbstractDirectoryWatcher::done,
             directoryWatcher.data(), &AbstractDirectoryWatcher::deleteLater ); // auto delete
    connect( directoryWatcher.data(), &AbstractDirectoryWatcher::requestScan,
             m_scanManager, &GenericScanManager::requestScan );
    ThreadWeaver::Queue::instance()->enqueue( directoryWatcher );
}

SqlCollection::~SqlCollection()
{
    DEBUG_BLOCK

    if( auto directoryWatcher = m_directoryWatcher.toStrongRef() )
        directoryWatcher->requestAbort();

    delete m_scanProcessor; // this prevents any further commits from the scanner
    delete m_collectionLocationFactory;
    delete m_queryMakerFactory;
    delete m_registry;
}

QString
SqlCollection::uidUrlProtocol() const
{
    return QStringLiteral("amarok-sqltrackuid");
}

QString
SqlCollection::generateUidUrl( const QString &hash )
{
    return uidUrlProtocol() + QStringLiteral("://") + hash;
}

QueryMaker*
SqlCollection::queryMaker()
{
    Q_ASSERT( m_queryMakerFactory );
    return m_queryMakerFactory->createQueryMaker();
}

SqlRegistry*
SqlCollection::registry() const
{
    Q_ASSERT( m_registry );
    return m_registry;
}

QSharedPointer<SqlStorage>
SqlCollection::sqlStorage() const
{
    Q_ASSERT( m_sqlStorage );
    return m_sqlStorage;
}

bool
SqlCollection::possiblyContainsTrack( const QUrl &url ) const
{
    if( url.isLocalFile() )
    {
        for( const QString &folder : collectionFolders() )
        {
            QUrl q = QUrl::fromLocalFile( folder );
            if( q.isParentOf( url ) || q.matches( url , QUrl::StripTrailingSlash) )
                return true;
        }
        return false;
    }
    else
        return url.scheme() == uidUrlProtocol();
}

Meta::TrackPtr
SqlCollection::trackForUrl( const QUrl &url )
{
    if( url.scheme() == uidUrlProtocol() )
        return m_registry->getTrackFromUid( url.url() );
    else if( url.scheme() == QStringLiteral("file") )
        return m_registry->getTrack( url.path() );
    else
        return Meta::TrackPtr();
}

Meta::TrackPtr
SqlCollection::getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl )
{
    return m_registry->getTrack( deviceId, rpath, directoryId, uidUrl );
}

Meta::TrackPtr
SqlCollection::getTrackFromUid( const QString &uniqueid )
{
    return m_registry->getTrackFromUid( uniqueid );
}

Meta::AlbumPtr
SqlCollection::getAlbum( const QString &album, const QString &artist )
{
    return m_registry->getAlbum( album, artist );
}

CollectionLocation*
SqlCollection::location()
{
    Q_ASSERT( m_collectionLocationFactory );
    return m_collectionLocationFactory->createSqlCollectionLocation();
}

void
SqlCollection::slotDeviceAdded( int id )
{
    QString query = QStringLiteral("select count(*) from tracks inner join urls on tracks.url = urls.id where urls.deviceid = %1");
    QStringList rs = m_sqlStorage->query( query.arg( id ) );
    if( !rs.isEmpty() )
    {
        int count = rs.first().toInt();
        if( count > 0 )
        {
            collectionUpdated();
        }
    }
    else
    {
        warning() << "Query " << query << "did not return a result! Is the database available?";
    }
}

void
SqlCollection::slotDeviceRemoved( int id )
{
    QString query = QStringLiteral("select count(*) from tracks inner join urls on tracks.url = urls.id where urls.deviceid = %1");
    QStringList rs = m_sqlStorage->query( query.arg( id ) );
    if( !rs.isEmpty() )
    {
        int count = rs.first().toInt();
        if( count > 0 )
        {
            collectionUpdated();
        }
    }
    else
    {
        warning() << "Query " << query << "did not return a result! Is the database available?";
    }
}

bool
SqlCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
    case Capabilities::Capability::Transcode:
        return true;
    default:
        return DatabaseCollection::hasCapabilityInterface( type );
    }
}

Capabilities::Capability*
SqlCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
    case Capabilities::Capability::Transcode:
        return new SqlCollectionTranscodeCapability();
    default:
        return DatabaseCollection::createCapabilityInterface( type );
    }
}

void
SqlCollection::dumpDatabaseContent()
{
    DatabaseUpdater updater( this );

    QStringList tables = m_sqlStorage->query( QStringLiteral( "select table_name from INFORMATION_SCHEMA.tables WHERE table_schema='amarok'" ) );
    for( const QString &table : tables )
    {
        QString filePath =
            QDir::home().absoluteFilePath( table + QLatin1Char('-') + QDateTime::currentDateTime().toString( Qt::ISODate ) + QStringLiteral(".csv") );
        updater.writeCSVFile( table, filePath, true );
    }
}

// ---------- SqlCollectionTranscodeCapability -------------

SqlCollectionTranscodeCapability::~SqlCollectionTranscodeCapability()
{
    // nothing to do
}

Transcoding::Configuration
SqlCollectionTranscodeCapability::savedConfiguration()
{
    KConfigGroup transcodeGroup = Amarok::config( SQL_TRANSCODING_GROUP_NAME );
    return Transcoding::Configuration::fromConfigGroup( transcodeGroup );
}

void
SqlCollectionTranscodeCapability::setSavedConfiguration( const Transcoding::Configuration &configuration )
{
    KConfigGroup transcodeGroup = Amarok::config( SQL_TRANSCODING_GROUP_NAME );
    configuration.saveToConfigGroup( transcodeGroup );
    transcodeGroup.sync();
}

