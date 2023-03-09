/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "IpodCollection.h"

#include "IpodCollectionLocation.h"
#include "IpodMeta.h"
#include "IpodPlaylistProvider.h"
#include "jobs/IpodWriteDatabaseJob.h"
#include "jobs/IpodParseTracksJob.h"
#include "support/IphoneMountPoint.h"
#include "support/IpodDeviceHelper.h"
#include "support/IpodTranscodeCapability.h"

#include "core/capabilities/ActionsCapability.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "playlistmanager/PlaylistManager.h"

#include <solid/device.h>
#include <solid/predicate.h>
#include <solid/storageaccess.h>
#include <ThreadWeaver/Queue>

#include <QTemporaryFile>
#include <QWeakPointer>

#include <gpod/itdb.h>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStorageInfo>
#include <QVBoxLayout>


const QString IpodCollection::s_uidUrlProtocol = QString( "amarok-ipodtrackuid" );
const QStringList IpodCollection::s_audioFileTypes = QStringList() << "mp3" << "aac"
    << "m4a" /* MPEG-4 AAC and also ALAC */ << "m4b" /* audiobook */ << "aiff" << "wav";
const QStringList IpodCollection::s_videoFileTypes = QStringList() << "m4v" << "mov";
const QStringList IpodCollection::s_audioVideoFileTypes = QStringList() << "mp4";

IpodCollection::IpodCollection( const QDir &mountPoint, const QString &uuid )
    : Collections::Collection()
    , m_configureDialog( nullptr )
    , m_mc( new Collections::MemoryCollection() )
    , m_itdb( nullptr )
    , m_lastUpdated( 0 )
    , m_preventUnmountTempFile( nullptr )
    , m_mountPoint( mountPoint.absolutePath() )
    , m_uuid( uuid )
    , m_iphoneAutoMountpoint( nullptr )
    , m_playlistProvider( nullptr )
    , m_configureAction( nullptr )
    , m_ejectAction( nullptr )
    , m_consolidateAction( nullptr )
{
    DEBUG_BLOCK
    if( m_uuid.isEmpty() )
        m_uuid = m_mountPoint;
}

IpodCollection::IpodCollection( const QString &uuid )
    : Collections::Collection()
    , m_configureDialog( nullptr )
    , m_mc( new Collections::MemoryCollection() )
    , m_itdb( nullptr )
    , m_lastUpdated( 0 )
    , m_preventUnmountTempFile( nullptr )
    , m_uuid( uuid )
    , m_playlistProvider( nullptr )
    , m_configureAction( nullptr )
    , m_ejectAction( nullptr )
    , m_consolidateAction( nullptr )
{
    DEBUG_BLOCK
    // following constructor displays error message if it cannot mount iPhone:
    m_iphoneAutoMountpoint = new IphoneMountPoint( uuid );
    m_mountPoint = m_iphoneAutoMountpoint->mountPoint();
    if( m_uuid.isEmpty() )
        m_uuid = m_mountPoint;
}

bool IpodCollection::init()
{
    if( m_mountPoint.isEmpty() )
        return false;  // we have already displayed error message

    m_updateTimer.setSingleShot( true );
    connect( this, &IpodCollection::startUpdateTimer, this, &IpodCollection::slotStartUpdateTimer );
    connect( &m_updateTimer, &QTimer::timeout, this, &IpodCollection::collectionUpdated );

    m_writeDatabaseTimer.setSingleShot( true );
    connect( this, &IpodCollection::startWriteDatabaseTimer, this, &IpodCollection::slotStartWriteDatabaseTimer );
    connect( &m_writeDatabaseTimer, &QTimer::timeout, this, &IpodCollection::slotInitiateDatabaseWrite );

    m_configureAction = new QAction( QIcon::fromTheme( "configure" ), i18n( "&Configure Device" ), this );
    m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
    connect( m_configureAction, &QAction::triggered, this, &IpodCollection::slotShowConfigureDialog );

    m_ejectAction = new QAction( QIcon::fromTheme( "media-eject" ), i18n( "&Eject Device" ), this );
    m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );
    connect( m_ejectAction, &QAction::triggered, this, &IpodCollection::slotEject );

    QString parseErrorMessage;
    m_itdb = IpodDeviceHelper::parseItdb( m_mountPoint, parseErrorMessage );
    m_prettyName = IpodDeviceHelper::collectionName( m_itdb ); // allows null m_itdb

    // m_consolidateAction is used by the provider
    m_consolidateAction = new QAction( QIcon::fromTheme( "dialog-ok-apply" ), i18n( "Re-add orphaned and forget stale tracks" ), this );
    // provider needs to be up before IpodParseTracksJob is started
    m_playlistProvider = new IpodPlaylistProvider( this );
    connect( m_playlistProvider, &IpodPlaylistProvider::startWriteDatabaseTimer, this, &IpodCollection::startWriteDatabaseTimer );
    connect( m_consolidateAction, &QAction::triggered, m_playlistProvider, &IpodPlaylistProvider::slotConsolidateStaleOrphaned );
    The::playlistManager()->addProvider( m_playlistProvider, m_playlistProvider->category() );

    if( m_itdb )
    {
        // parse tracks in a thread in order not to block main thread
        IpodParseTracksJob *job = new IpodParseTracksJob( this );
        m_parseTracksJob = job;
        connect( job, &IpodParseTracksJob::done, job, &QObject::deleteLater );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
    }
    else
        slotShowConfigureDialogWithError( parseErrorMessage ); // shows error message and allows initializing

    return true;  // we have found iPod, even if it might not be initialised
}

IpodCollection::~IpodCollection()
{
    DEBUG_BLOCK
    The::playlistManager()->removeProvider( m_playlistProvider );

    // this is not racy: destructor should be called in a main thread, the timer fires in the
    // same thread
    if( m_writeDatabaseTimer.isActive() )
    {
        m_writeDatabaseTimer.stop();
        // call directly from main thread in destructor, we have no other chance:
        writeDatabase();
    }
    delete m_preventUnmountTempFile; // this should have been certainly 0, but why not
    m_preventUnmountTempFile = nullptr;

    /* because m_itdb takes ownership of the tracks added to it, we need to remove the
     * tracks from itdb before we delete it because in Amarok, IpodMeta::Track is the owner
     * of the track */
    IpodDeviceHelper::unlinkPlaylistsTracksFromItdb( m_itdb );  // does nothing if m_itdb is null
    itdb_free( m_itdb );  // does nothing if m_itdb is null
    m_itdb = nullptr;

    delete m_configureDialog;
    delete m_iphoneAutoMountpoint; // this can unmount iPhone and remove temporary dir
}

bool
IpodCollection::possiblyContainsTrack( const QUrl &url ) const
{
    return url.toLocalFile().startsWith( m_mountPoint );
}

Meta::TrackPtr
IpodCollection::trackForUrl( const QUrl &url )
{
    QString relativePath = url.toLocalFile().mid( m_mountPoint.size() + 1 );
    QString uidUrl = QString( "%1/%2" ).arg( collectionId(), relativePath );
    return trackForUidUrl( uidUrl );
}

bool
IpodCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        case Capabilities::Capability::Transcode:
            return true;
        default:
            break;
    }
    return false;
}

Capabilities::Capability*
IpodCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        {
            QList<QAction *> actions;
            if( m_configureAction )
                actions << m_configureAction;
            if( m_ejectAction )
                actions << m_ejectAction;
            if( m_consolidateAction && m_playlistProvider && m_playlistProvider->hasStaleOrOrphaned() )
                actions << m_consolidateAction;
            return new Capabilities::ActionsCapability( actions );
        }
        case Capabilities::Capability::Transcode:
        {
            gchar *deviceDirChar = itdb_get_device_dir( QFile::encodeName( m_mountPoint ) );
            QString deviceDir = QFile::decodeName( deviceDirChar );
            g_free( deviceDirChar );
            return new Capabilities::IpodTranscodeCapability( this, deviceDir );
        }
        default:
            break;
    }
    return nullptr;
}

Collections::QueryMaker*
IpodCollection::queryMaker()
{
    return new Collections::MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
IpodCollection::uidUrlProtocol() const
{
    return s_uidUrlProtocol;
}

QString
IpodCollection::collectionId() const
{
    return QStringLiteral( "%1://%2" ).arg( s_uidUrlProtocol, m_uuid );
}

QString
IpodCollection::prettyName() const
{
    return m_prettyName;
}

QIcon
IpodCollection::icon() const
{
    return QIcon::fromTheme("multimedia-player-apple-ipod");
}

bool
IpodCollection::hasCapacity() const
{
    return QStorageInfo( m_mountPoint ).isValid();
}

float
IpodCollection::usedCapacity() const
{
    return QStorageInfo( m_mountPoint ).bytesTotal() - QStorageInfo( m_mountPoint ).bytesFree();
}

float
IpodCollection::totalCapacity() const
{
    return QStorageInfo( m_mountPoint ).bytesTotal();
}

Collections::CollectionLocation*
IpodCollection::location()
{
    return new IpodCollectionLocation( QPointer<IpodCollection>( this ) );
}

bool
IpodCollection::isWritable() const
{
    return IpodDeviceHelper::safeToWrite( m_mountPoint, m_itdb ); // returns false if m_itdb is null
}

bool
IpodCollection::isOrganizable() const
{
    return false; // iPods are never organizable
}

void
IpodCollection::metadataChanged(const Meta::TrackPtr &track )
{
    // reflect change to outside world:
    bool mapsChanged = MemoryMeta::MapChanger( m_mc.data() ).trackChanged( track );
    if( mapsChanged )
        // while docs say something different, collection browser doesn't update unless we Q_EMIT updated()
        Q_EMIT startUpdateTimer();
    Q_EMIT startWriteDatabaseTimer();
}

QString
IpodCollection::mountPoint()
{
    return m_mountPoint;
}

float
IpodCollection::capacityMargin() const
{
    return 20*1024*1024; // 20 MiB
}

QStringList
IpodCollection::supportedFormats() const
{
    QStringList ret( s_audioFileTypes );
    if( m_itdb && itdb_device_supports_video( m_itdb->device ) )
        ret << s_videoFileTypes << s_audioVideoFileTypes;
    return ret;
}

Playlists::UserPlaylistProvider*
IpodCollection::playlistProvider() const
{
    return m_playlistProvider;
}

Meta::TrackPtr
IpodCollection::trackForUidUrl( const QString &uidUrl )
{
    m_mc->acquireReadLock();
    Meta::TrackPtr ret = m_mc->trackMap().value( uidUrl, Meta::TrackPtr() );
    m_mc->releaseLock();
    return ret;
}

void
IpodCollection::slotDestroy()
{
    // guard against user hitting the button twice or hitting it while there is another
    // write database job already running
    if( m_writeDatabaseJob )
    {
        IpodWriteDatabaseJob *job = m_writeDatabaseJob.data();
        // don't create duplicate connections:
        disconnect( job, &QObject::destroyed, this, &IpodCollection::slotRemove );
        disconnect( job, &QObject::destroyed, this, &IpodCollection::slotPerformTeardownAndRemove );
        connect( job, &QObject::destroyed, this, &IpodCollection::slotRemove );
    }
    // this is not racy: slotDestroy() is delivered to main thread, the timer fires in the
    // same thread
    else if( m_writeDatabaseTimer.isActive() )
    {
        // write database in a thread so that it need not be written in destructor
        m_writeDatabaseTimer.stop();
        IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
        m_writeDatabaseJob = job;
        connect( job, &IpodWriteDatabaseJob::done, job, &QObject::deleteLater );
        connect( job, &QObject::destroyed, this, &IpodCollection::slotRemove );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
    }
    else
        slotRemove();
}

void
IpodCollection::slotEject()
{
    // guard against user hitting the button twice or hitting it while there is another
    // write database job already running
    if( m_writeDatabaseJob )
    {
        IpodWriteDatabaseJob *job = m_writeDatabaseJob.data();
        // don't create duplicate connections:
        disconnect( job, &QObject::destroyed, this, &IpodCollection::slotRemove );
        disconnect( job, &QObject::destroyed, this, &IpodCollection::slotPerformTeardownAndRemove );
        connect( job, &QObject::destroyed, this, &IpodCollection::slotPerformTeardownAndRemove );
    }
    // this is not racy: slotEject() is delivered to main thread, the timer fires in the
    // same thread
    else if( m_writeDatabaseTimer.isActive() )
    {
        // write database now because iPod will be already unmounted in destructor
        m_writeDatabaseTimer.stop();
        IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
        m_writeDatabaseJob = job;
        connect( job, &IpodWriteDatabaseJob::done, job, &QObject::deleteLater );
        connect( job, &QObject::destroyed, this, &IpodCollection::slotPerformTeardownAndRemove );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
    }
    else
        slotPerformTeardownAndRemove();
}

void
IpodCollection::slotShowConfigureDialog()
{
    slotShowConfigureDialogWithError( QString() );
}

void
IpodCollection::slotShowConfigureDialogWithError( const QString &errorMessage )
{
    if( !m_configureDialog )
    {
        // create the dialog
        m_configureDialog = new QDialog();
        QWidget *settingsWidget = new QWidget( m_configureDialog );
        m_configureDialogUi.setupUi( settingsWidget );

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        QWidget *mainWidget = new QWidget;
        QVBoxLayout *mainLayout = new QVBoxLayout;
        m_configureDialog->setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, m_configureDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, m_configureDialog, &QDialog::reject);

        mainLayout->addWidget(settingsWidget);
        mainLayout->addWidget(buttonBox);

        m_configureDialog->setWindowTitle( settingsWidget->windowTitle() );  // setupUi() sets this
        if( m_itdb )
        {
            // we will never initialize this iPod this time, hide ui for it completely
            m_configureDialogUi.modelComboLabel->hide();
            m_configureDialogUi.modelComboBox->hide();
            m_configureDialogUi.initializeLabel->hide();
            m_configureDialogUi.initializeButton->hide();
        }

        connect( m_configureDialogUi.initializeButton, &QPushButton::clicked, this, &IpodCollection::slotInitialize );
        connect( m_configureDialog, &QDialog::accepted, this, &IpodCollection::slotApplyConfiguration );
    }
    QScopedPointer<Capabilities::TranscodeCapability> tc( create<Capabilities::TranscodeCapability>() );
    IpodDeviceHelper::fillInConfigureDialog( m_configureDialog, &m_configureDialogUi,
                                             m_mountPoint, m_itdb, tc->savedConfiguration(),
                                             errorMessage );

    // don't allow to resize the dialog too small:
    m_configureDialog->setMinimumSize( m_configureDialog->sizeHint() );
    m_configureDialog->show();
    m_configureDialog->raise();
}

void IpodCollection::collectionUpdated()
{
    m_lastUpdated = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT updated();
}

void
IpodCollection::slotInitialize()
{
    if( m_itdb )
        return;  // why the hell we were called?

    m_configureDialogUi.initializeButton->setEnabled( false );
    QString errorMessage;
    bool success = IpodDeviceHelper::initializeIpod( m_mountPoint, &m_configureDialogUi, errorMessage );
    if( !success )
    {
        slotShowConfigureDialogWithError( errorMessage );
        return;
    }

    errorMessage.clear();
    m_itdb = IpodDeviceHelper::parseItdb( m_mountPoint, errorMessage );
    m_prettyName = IpodDeviceHelper::collectionName( m_itdb ); // allows null m_itdb

    if( m_itdb )
    {
        QScopedPointer<Capabilities::TranscodeCapability> tc( create<Capabilities::TranscodeCapability>() );
        errorMessage = i18nc( "iPod was successfully initialized", "Initialization successful." );
        // so that the buttons are re-enabled, info filled etc:
        IpodDeviceHelper::fillInConfigureDialog( m_configureDialog, &m_configureDialogUi,
            m_mountPoint, m_itdb, tc->savedConfiguration(), errorMessage );

        // there will be probably 0 tracks, but it may do more in future, for example stale
        // & orphaned track search.
        IpodParseTracksJob *job = new IpodParseTracksJob( this );
        connect( job, &IpodParseTracksJob::done, job, &QObject::deleteLater );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
    }
    else
        slotShowConfigureDialogWithError( errorMessage ); // shows error message and allows initializing
}

void
IpodCollection::slotApplyConfiguration()
{
    if( !isWritable() )
        return; // we can do nothing if we are not writeable

    QString newName = m_configureDialogUi.nameLineEdit->text();
    if( !newName.isEmpty() && newName != IpodDeviceHelper::ipodName( m_itdb ) )
    {
        IpodDeviceHelper::setIpodName( m_itdb, newName );
        m_prettyName = IpodDeviceHelper::collectionName( m_itdb );
        Q_EMIT startWriteDatabaseTimer(); // the change should be written down to the database
        Q_EMIT startUpdateTimer();
    }

    QScopedPointer<Capabilities::TranscodeCapability> tc( create<Capabilities::TranscodeCapability>() );
    tc->setSavedConfiguration( m_configureDialogUi.transcodeComboBox->currentChoice() );
}

void
IpodCollection::slotStartUpdateTimer()
{
    // there are no concurrency problems, this method can only be called from the main
    // thread and that's where the timer fires
    if( m_updateTimer.isActive() )
        return; // already running, nothing to do

    // number of milliseconds to next desired update, may be negative
    int timeout = m_lastUpdated + 1000 - QDateTime::currentMSecsSinceEpoch();
    // give at least 50 msecs to catch multi-tracks edits nicely on the first frame
    m_updateTimer.start( qBound( 50, timeout, 1000 ) );
}

void
IpodCollection::slotStartWriteDatabaseTimer()
{
    m_writeDatabaseTimer.start( 30000 );
    // ensure we have a file on iPod open that prevents unmounting it if db is dirty
    if( !m_preventUnmountTempFile )
    {
        m_preventUnmountTempFile = new QTemporaryFile();
        QString name( "/.itunes_database_dirty_in_amarok_prevent_unmounting" );
        m_preventUnmountTempFile->setFileTemplate( m_mountPoint + name );
        m_preventUnmountTempFile->open();
    }
}

void IpodCollection::slotInitiateDatabaseWrite()
{
    if( m_writeDatabaseJob )
    {
        warning() << __PRETTY_FUNCTION__ << "called while m_writeDatabaseJob still points"
                  << "to an older job. Not doing anything.";
        return;
    }
    IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
    m_writeDatabaseJob = job;
    connect( job, &IpodWriteDatabaseJob::done, job, &QObject::deleteLater );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
}

void IpodCollection::slotPerformTeardownAndRemove()
{
    /* try to eject the device from system. Following technique potentially catches more
     * cases than simply passing the udi from IpodCollectionFactory, think of fuse-based
     * filesystems for mounting iPhones et cetera.. */
    Solid::Predicate query( Solid::DeviceInterface::StorageAccess, QString( "filePath" ),
                            m_mountPoint );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( query );
    if( devices.count() == 1 )
    {
        Solid::Device device = devices.at( 0 );
        Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
        if( ssa )
            ssa->teardown();
    }

    slotRemove();
}

void IpodCollection::slotRemove()
{
    // this is not racy, we are in the main thread and parseTracksJob can be deleted only
    // in the main thread
    if( m_parseTracksJob )
    {
        // we need to wait until parseTracksJob finishes, because it accesses IpodCollection
        // and IpodPlaylistProvider in an asynchronous way that cannot safely cope with
        // IpodCollection disappearing
        connect( m_parseTracksJob.data(), &QObject::destroyed, this, &IpodCollection::remove );
        m_parseTracksJob->abort();
    }
    else
        Q_EMIT remove();
}

Meta::TrackPtr
IpodCollection::addTrack( IpodMeta::Track *track )
{
    if( !track || !m_itdb )
        return Meta::TrackPtr();

    Itdb_Track *itdbTrack = track->itdbTrack();
    bool justAdded = false;

    m_itdbMutex.lock();
    Q_ASSERT( !itdbTrack->itdb || itdbTrack->itdb == m_itdb /* refuse to take track from another itdb */ );
    if( !itdbTrack->itdb )
    {
        itdb_track_add( m_itdb, itdbTrack, -1 );
        // if it wasn't in itdb, it couldn't have legally been in master playlist
        // TODO: podcasts should not go into MPL
        itdb_playlist_add_track( itdb_playlist_mpl( m_itdb ), itdbTrack, -1 );

        justAdded = true;
        Q_EMIT startWriteDatabaseTimer();
    }
    track->setCollection( QPointer<IpodCollection>( this ) );

    Meta::TrackPtr trackPtr( track );
    Meta::TrackPtr memTrack = MemoryMeta::MapChanger( m_mc.data() ).addTrack( trackPtr );
    if( !memTrack && justAdded )
    {
        /* this new track was not added to MemoryCollection, it may vanish soon, prevent
         * dangling pointer in m_itdb */
        itdb_playlist_remove_track( nullptr /* = MPL */, itdbTrack );
        itdb_track_unlink( itdbTrack );
    }
    m_itdbMutex.unlock();

    if( memTrack )
    {
        subscribeTo( trackPtr );
        Q_EMIT startUpdateTimer();
    }
    return memTrack;
}

void
IpodCollection::removeTrack( const Meta::TrackPtr &track )
{
    if( !track )
        return; // nothing to do
    /* Following call ensures thread-safety even when this method is called multiple times
     * from different threads with the same track: only one thread will get non-null
     * deletedTrack from MapChanger. */
    Meta::TrackPtr deletedTrack = MemoryMeta::MapChanger( m_mc.data() ).removeTrack( track );
    if( !deletedTrack )
    {
        warning() << __PRETTY_FUNCTION__ << "attempt to delete a track that was not in"
                  << "MemoryCollection or not added using MapChanger";
        return;
    }
    IpodMeta::Track *ipodTrack = dynamic_cast<IpodMeta::Track *>( deletedTrack.data() );
    if( !ipodTrack )
    {
        warning() << __PRETTY_FUNCTION__ << "attempt to delete a track that was not"
                  << "internally iPod track";
        return;
    }

    Itdb_Track *itdbTrack = ipodTrack->itdbTrack();
    if( itdbTrack->itdb && m_itdb )
    {
        // remove from all playlists excluding the MPL:
        m_playlistProvider->removeTrackFromPlaylists( track );

        QMutexLocker locker( &m_itdbMutex );
        // remove track from the master playlist:
        itdb_playlist_remove_track( itdb_playlist_mpl( m_itdb ), itdbTrack );
        // remove it from the db:
        itdb_track_unlink( itdbTrack );
        Q_EMIT startWriteDatabaseTimer();
    }

    Q_EMIT startUpdateTimer();
}

bool IpodCollection::writeDatabase()
{
    if( !IpodDeviceHelper::safeToWrite( m_mountPoint, m_itdb ) ) // returns false if m_itdb is null
    {
        // we have to delete unmount-preventing file even in this case
        delete m_preventUnmountTempFile;
        m_preventUnmountTempFile = nullptr;
        warning() << "Refusing to write iTunes database to iPod becauase device is not safe to write";
        return false;
    }

    m_itdbMutex.lock();
    GError *error = nullptr;
    bool success = itdb_write( m_itdb, &error );
    m_itdbMutex.unlock();
    QString gpodError;
    if( error )
    {
        gpodError = QString::fromUtf8( error->message );
        g_error_free( error );
        error = nullptr;
    }
    delete m_preventUnmountTempFile;  // this deletes the file
    m_preventUnmountTempFile = nullptr;

    if( success )
    {
        QString message = i18nc( "%1: iPod collection name",
                         "iTunes database successfully written to %1", prettyName() );
        Amarok::Logger::shortMessage( message );
    }
    else
    {
        QString message;
        if( gpodError.isEmpty() )
            message = i18nc( "%1: iPod collection name",
                             "Writing iTunes database to %1 failed without an indication of error",
                             prettyName() );
        else
            message = i18nc( "%1: iPod collection name, %2: technical error from libgpod",
                             "Writing iTunes database to %1 failed: %2", prettyName(), gpodError );
        Amarok::Logger::longMessage( message );
    }
    return success;
}

