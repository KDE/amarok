/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
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

#include "SqlCollection.h"

#include "DefaultSqlQueryMakerFactory.h"
#include "CapabilityDelegate.h"
#include "CapabilityDelegateImpl.h"
#include "DatabaseUpdater.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/capabilities/TranscodeCapability.h"
#include "core/transcoding/TranscodingController.h"
#include "core-impl/collections/db/MountPointManager.h"
#include "core-impl/collections/db/ScanManager.h"
#include "dialogs/OrganizeCollectionDialog.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SqlScanResultProcessor.h"
#include "SvgHandler.h"
#include "MainWindow.h"

#include <KApplication>
#include <KMessageBox>

#include <QApplication>
#include <QDir>

namespace Collections {

class OrganizeCollectionDelegateImpl : public OrganizeCollectionDelegate
{
public:
    OrganizeCollectionDelegateImpl()
        : OrganizeCollectionDelegate()
        , m_dialog( 0 )
        , m_organizing( false ) {}
    virtual ~ OrganizeCollectionDelegateImpl() { delete m_dialog; }

    virtual void setTracks( const Meta::TrackList &tracks ) { m_tracks = tracks; }
    virtual void setFolders( const QStringList &folders ) { m_folders = folders; }
    virtual void setIsOrganizing( bool organizing ) { m_organizing = organizing; }
    virtual void setTranscodingConfiguration( const Transcoding::Configuration &configuration )
    { m_targetFileExtension =
      Amarok::Components::transcodingController()->format( configuration.encoder() )->fileExtension(); }
    virtual void setCaption( const QString &caption ) { m_caption = caption; }

    virtual void show()
    {
        m_dialog = new OrganizeCollectionDialog( m_tracks,
                    m_folders,
                    m_targetFileExtension,
                    The::mainWindow(), //parent
                    "", //name is unused
                    true, //modal
                    m_caption //caption
                );

        connect( m_dialog, SIGNAL( accepted() ), SIGNAL( accepted() ) );
        connect( m_dialog, SIGNAL( rejected() ), SIGNAL( rejected() ) );
        m_dialog->show();
    }

    virtual bool overwriteDestinations() const { return m_dialog->overwriteDestinations(); }
    virtual QMap<Meta::TrackPtr, QString> destinations() const { return m_dialog->getDestinations(); }

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
    virtual OrganizeCollectionDelegate* createDelegate() { return new OrganizeCollectionDelegateImpl(); }
};


class SqlCollectionLocationFactoryImpl : public SqlCollectionLocationFactory
{
public:
    SqlCollectionLocationFactoryImpl( SqlCollection *collection )
        : SqlCollectionLocationFactory()
        , m_collection( collection ) {}

    SqlCollectionLocation *createSqlCollectionLocation() const
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

SqlCollection::SqlCollection( const QString &id, const QString &prettyName, SqlStorage* storage )
    : DatabaseCollection( id, prettyName )
    , m_registry( 0 )
    , m_albumCapabilityDelegate( 0 )
    , m_artistCapabilityDelegate( 0 )
    , m_trackCapabilityDelegate( 0 )
    , m_sqlStorage( storage )
    , m_collectionLocationFactory( 0 )
    , m_queryMakerFactory( 0 )
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
            KDialog dialog( 0, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint );
            QLabel label( i18n( "Updating Amarok database schema. Please don't terminate "
                "Amarok now as it may result in database corruption." ) );
            label.setWordWrap( true );
            dialog.setMainWidget( &label );
            dialog.setCaption( i18n( "Updating Amarok database schema" ) );
            dialog.setButtons( KDialog::None );
            dialog.setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            dialog.show();
            dialog.raise();
            // otherwise the splash screen doesn't load image and this dialog is not shown:
            kapp->processEvents();

            updater.update();

            dialog.hide();
            kapp->processEvents();
        }
        else // this is new schema creation
            updater.update();
    }

    //perform a quick check of the database
    updater.cleanupDatabase();

    m_registry = new SqlRegistry( this );
    m_albumCapabilityDelegate = new Capabilities::AlbumCapabilityDelegateImpl();
    m_artistCapabilityDelegate = new Capabilities::ArtistCapabilityDelegateImpl();
    m_trackCapabilityDelegate = new Capabilities::TrackCapabilityDelegateImpl();

    m_collectionLocationFactory = new SqlCollectionLocationFactoryImpl( this );
    m_queryMakerFactory = new DefaultSqlQueryMakerFactory( this );
    m_scanManager = new ScanManager( this, this );
    connect( m_scanManager, SIGNAL(scanStarted(ScannerJob*)), SLOT(slotScanStarted(ScannerJob*)) );

    // we need a UI-dialog service, but for now just output the messages
    if( !storage->getLastErrors().isEmpty() )
    {
        if( QApplication::type() != QApplication::Tty )
        {
            KMessageBox::error( The::mainWindow(), //parent
                                i18n( "The amarok database reported the following errors:\n"
                                      "%1\n"
                                      "In most cases you will need to resolve these errors before Amarok will run properly." ).
                                arg(storage->getLastErrors().join( "\n" ) ) );
        }
        else
        {
            warning() << QString( "The amarok database reported the following errors:\n"
                                  "%1\n"
                                  "In most cases you will need to resolve these errors before Amarok will run properly.").
                arg(storage->getLastErrors().join( "\n" ) );
        }
    }
}

SqlCollection::~SqlCollection()
{
    if( m_scanManager )
        m_scanManager->blockScan();
    delete m_albumCapabilityDelegate;
    delete m_artistCapabilityDelegate;
    delete m_trackCapabilityDelegate;
    delete m_collectionLocationFactory;
    delete m_queryMakerFactory;
    delete m_sqlStorage;
    delete m_registry;
}

QString
SqlCollection::uidUrlProtocol() const
{
    return "amarok-sqltrackuid";
}

QString
SqlCollection::generateUidUrl( const QString &hash )
{
    return uidUrlProtocol() + "://" + hash;
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

SqlStorage*
SqlCollection::sqlStorage() const
{
    Q_ASSERT( m_sqlStorage );
    return m_sqlStorage;
}

void
SqlCollection::slotScanStarted( ScannerJob *job )
{
    if( !job )
    {
        warning() << "job is invalid";
        return;
    }

    if( Amarok::Components::logger() )
    {
        Amarok::Components::logger()->newProgressOperation( job,
                                                            i18n( "Scanning music" ),
                                                            100,
                                                            m_scanManager,
                                                            SLOT(abort()) );
    }
}

bool
SqlCollection::isDirInCollection( const QString &p )
{
    QString path = p;
    // In the database all directories have a trailing slash, so we must add that
    if( !path.endsWith( '/' ) )
        path += '/';

    const int deviceid = mountPointManager()->getIdForUrl( path );
    const QString rpath = mountPointManager()->getRelativePath( deviceid, path );

    const QStringList values =
            sqlStorage()->query( QString( "SELECT changedate FROM directories WHERE dir = '%2' AND deviceid = %1;" )
            .arg( QString::number( deviceid ), sqlStorage()->escape( rpath ) ) );

    return !values.isEmpty();
}

bool
SqlCollection::possiblyContainsTrack( const KUrl &url ) const
{
    // what about uidUrlProtocol?
    foreach( const QString &folder, collectionFolders() )
    {
        if ( url.path().contains( folder ) )
            return true;
    }
    return url.protocol() == "file" || url.protocol() == uidUrlProtocol();
}

Meta::TrackPtr
SqlCollection::trackForUrl( const KUrl &url )
{
    if( url.protocol() == uidUrlProtocol() )
        return m_registry->getTrackFromUid( url.url() );
    else if( url.protocol() == "file" )
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

QStringList
SqlCollection::getDatabaseDirectories( QList<int> idList ) const
{
    QString deviceIds;
    foreach( int id, idList )
    {  
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number( id );
    }

    QString query = QString( "SELECT deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" );
    return m_sqlStorage->query( query.arg( deviceIds ) );
}

void
SqlCollection::slotDeviceAdded( int id )
{
    QString query = "select count(*) from tracks inner join urls on tracks.url = urls.id where urls.deviceid = %1";
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
    QString query = "select count(*) from tracks inner join urls on tracks.url = urls.id where urls.deviceid = %1";
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
        case Capabilities::Capability::CollectionImport:
        case Capabilities::Capability::CollectionScan:
            return (bool) m_scanManager;
        case Capabilities::Capability::Transcode:
            return true;
        default:
            break;
    }
    return false;
}

Capabilities::Capability*
SqlCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::CollectionImport:
            return m_scanManager ? new SqlCollectionImportCapability( m_scanManager ) : 0;
        case Capabilities::Capability::CollectionScan:
            return m_scanManager ? new SqlCollectionScanCapability( m_scanManager ) : 0;
        case Capabilities::Capability::Transcode:
            return new SqlCollectionTranscodeCapability();
        default:
            break;
    }
    return 0;
}

void
SqlCollection::dumpDatabaseContent()
{
    DatabaseUpdater updater( this );

    QStringList tables = m_sqlStorage->query( "select table_name from INFORMATION_SCHEMA.tables WHERE table_schema='amarok'" );
    foreach( const QString &table, tables )
    {
        QString filePath =
            QDir::home().absoluteFilePath( table + '-' + QDateTime::currentDateTime().toString( Qt::ISODate ) + ".csv" );
        updater.writeCSVFile( table, filePath, true );
    }
}

ScanResultProcessor*
SqlCollection::getNewScanResultProcessor()
{
    return new SqlScanResultProcessor( this );
}


// --------- SqlCollectionScanCapability -------------

SqlCollectionScanCapability::SqlCollectionScanCapability( ScanManager* scanManager )
    : m_scanManager( scanManager )
{ }

SqlCollectionScanCapability::~SqlCollectionScanCapability()
{ }

void
SqlCollectionScanCapability::startFullScan()
{
    if( m_scanManager )
        m_scanManager->requestFullScan();
}

void
SqlCollectionScanCapability::startIncrementalScan( const QString &directory )
{
    if( m_scanManager )
        m_scanManager->requestIncrementalScan( directory );
}

void
SqlCollectionScanCapability::stopScan()
{
    if( m_scanManager )
        m_scanManager->abort( "Abort requested from SqlCollection::stopScan()" );
}

// --------- SqlCollectionImportCapability -------------

SqlCollectionImportCapability::SqlCollectionImportCapability( ScanManager* scanManager )
    : m_scanManager( scanManager )
{ }

SqlCollectionImportCapability::~SqlCollectionImportCapability()
{ }

void
SqlCollectionImportCapability::import( QIODevice *input, QObject *listener )
{
    DEBUG_BLOCK
    if( m_scanManager )
    {
        // ok. connecting of the signals is very specific for the SqlBatchImporter.
        // For now this works.

        /*
           connect( m_worker, SIGNAL( trackAdded( Meta::TrackPtr ) ),
           this, SIGNAL( trackAdded( Meta::TrackPtr ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackDiscarded( QString ) ),
           this, SIGNAL( trackDiscarded( QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ),
           this, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ),
           this, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( importError( QString ) ),
           this, SIGNAL( importError( QString ) ), Qt::QueuedConnection );
           */

        connect( m_scanManager, SIGNAL( finished() ),
                 listener, SIGNAL( importSucceeded() ) );
        connect( m_scanManager, SIGNAL( message( QString ) ),
                 listener, SIGNAL( showMessage( QString ) ) );

        m_scanManager->requestImport( input );
    }
}

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

#include "SqlCollection.moc"
