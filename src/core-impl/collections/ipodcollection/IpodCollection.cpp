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
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "playlistmanager/PlaylistManager.h"

#include <KDiskFreeSpaceInfo>
#include <solid/device.h>
#include <solid/predicate.h>
#include <solid/storageaccess.h>
#include <ThreadWeaver/Weaver>
#include <KMessageBox>

#include <QWeakPointer>

#include <gpod/itdb.h>


const QString IpodCollection::s_uidUrlProtocol = QString( "amarok-ipodtrackuid" );
const QStringList IpodCollection::s_audioFileTypes = QStringList() << "mp3" << "aac"
    << "m4a" /* MPEG-4 AAC and also ALAC */ << "m4b" /* audiobook */ << "aiff" << "wav";
const QStringList IpodCollection::s_videoFileTypes = QStringList() << "m4v" << "mov";
const QStringList IpodCollection::s_audioVideoFileTypes = QStringList() << "mp4";

IpodCollection::IpodCollection( const QDir &mountPoint )
    : Collections::Collection()
    , m_configureDialog( 0 )
    , m_mc( new Collections::MemoryCollection() )
    , m_itdb( 0 )
    , m_lastUpdated( 0 )
    , m_preventUnmountTempFile( 0 )
    , m_mountPoint( mountPoint.absolutePath() )
    , m_iphoneAutoMountpoint( 0 )
    , m_playlistProvider( 0 )
    , m_configureAction( 0 )
    , m_ejectAction( 0 )
{
    DEBUG_BLOCK
}

IpodCollection::IpodCollection( const QString &uuid )
    : Collections::Collection()
    , m_configureDialog( 0 )
    , m_mc( new Collections::MemoryCollection() )
    , m_itdb( 0 )
    , m_lastUpdated( 0 )
    , m_preventUnmountTempFile( 0 )
    , m_playlistProvider( 0 )
    , m_configureAction( 0 )
    , m_ejectAction( 0 )
{
    DEBUG_BLOCK
    m_iphoneAutoMountpoint = new IphoneMountPoint( uuid );
    m_mountPoint = m_iphoneAutoMountpoint->mountPoint();
}

bool IpodCollection::init()
{
    if( m_mountPoint.isEmpty() )
    {
        KMessageBox::sorry( 0, i18n("Cannot connect to iPhone or iPad. More "
            "information is available in the Amarok debug log.") );
        return false;
    }

    m_updateTimer.setSingleShot( true );
    connect( this, SIGNAL(startUpdateTimer()), SLOT(slotStartUpdateTimer()) );
    connect( &m_updateTimer, SIGNAL(timeout()), SLOT(collectionUpdated()) );

    m_writeDatabaseTimer.setSingleShot( true );
    connect( this, SIGNAL(startWriteDatabaseTimer()), SLOT(slotStartWriteDatabaseTimer()) );
    connect( &m_writeDatabaseTimer, SIGNAL(timeout()), SLOT(slotInitiateDatabaseWrite()) );

    m_configureAction = new QAction( KIcon( "configure" ), i18n( "&Configure Device" ), this );
    m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
    connect( m_configureAction, SIGNAL( triggered() ), SLOT( slotShowConfigureDialog() ) );

    m_ejectAction = new QAction( KIcon( "media-eject" ), i18n( "&Eject Device" ), this );
    m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );
    connect( m_ejectAction, SIGNAL(triggered()), SLOT(slotEject()) );

    QString parseErrorMessage;
    m_itdb = IpodDeviceHelper::parseItdb( m_mountPoint, parseErrorMessage );
    m_prettyName = IpodDeviceHelper::collectionName( m_itdb ); // allows null m_itdb

    // provider needs to be up before IpodParseTracksJob is started
    m_playlistProvider = new IpodPlaylistProvider( this );
    connect( m_playlistProvider, SIGNAL(startWriteDatabaseTimer()), SIGNAL(startWriteDatabaseTimer()) );
    The::playlistManager()->addProvider( m_playlistProvider, m_playlistProvider->category() );

    if( m_itdb )
    {
        // parse tracks in a thread in order not to block main thread
        IpodParseTracksJob *job = new IpodParseTracksJob( this );
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
        ThreadWeaver::Weaver::instance()->enqueue( job );
    }
    else
        slotShowConfigureDialog( parseErrorMessage ); // shows error message and allows initializing

    return true;  // we have found iPod, even if it might not be initialised
}

IpodCollection::~IpodCollection()
{
    DEBUG_BLOCK
    // this is not racy: destructor should be called in a main thread, the timer fires in the
    // same thread
    if( m_writeDatabaseTimer.isActive() )
    {
        m_writeDatabaseTimer.stop();
        // call directly from main thread in destructor, we have no other chance:
        writeDatabase();
    }
    delete m_preventUnmountTempFile; // this shoud have been certaily 0, but why not
    m_preventUnmountTempFile = 0;

    /* because m_itdb takes ownership of the tracks added to it, we need to remove the
     * tracks from itdb before we delete it because in Amarok, IpodMeta::Track is the owner
     * of the track */
    IpodDeviceHelper::unlinkTracksFromItdb( m_itdb );  // does nothing if m_itdb is null
    itdb_free( m_itdb );  // does nothing if m_itdb is null
    m_itdb = 0;

    delete m_configureDialog;
    delete m_iphoneAutoMountpoint; // this can unmount iPhone and remove temporary dir

    The::playlistManager()->removeProvider( m_playlistProvider );
    delete m_playlistProvider;
}

bool
IpodCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.toLocalFile().startsWith( m_mountPoint );
}

Meta::TrackPtr
IpodCollection::trackForUrl( const KUrl &url )
{
    QString uidUrl = QString( "%1://%2" )
                     .arg( s_uidUrlProtocol )
                     .arg( url.toLocalFile() );
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
    return 0;
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
    return QString( "%1://%2" )
           .arg( s_uidUrlProtocol )
           .arg( m_mountPoint );
}

QString
IpodCollection::prettyName() const
{
    return m_prettyName;
}

KIcon
IpodCollection::icon() const
{
    return KIcon("multimedia-player-apple-ipod");
}

bool
IpodCollection::hasCapacity() const
{
    return KDiskFreeSpaceInfo::freeSpaceInfo( m_mountPoint ).isValid();
}

float
IpodCollection::usedCapacity() const
{
    return KDiskFreeSpaceInfo::freeSpaceInfo( m_mountPoint ).used();
}

float
IpodCollection::totalCapacity() const
{
    return KDiskFreeSpaceInfo::freeSpaceInfo( m_mountPoint ).size();
}

Collections::CollectionLocation*
IpodCollection::location()
{
    return new IpodCollectionLocation( QWeakPointer<IpodCollection>( this ) );
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
IpodCollection::metadataChanged( Meta::TrackPtr track )
{
    // reflect change to ouside world:
    bool mapsChanged = MemoryMeta::MapChanger( m_mc.data() ).trackChanged( track );
    if( mapsChanged )
        // while docs say somehting different, collection browser doesnt update unless we emit updated()
        emit startUpdateTimer();
    emit startWriteDatabaseTimer();
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
    // this is not racy: destroy() is delivered to main thread, the timer fires in the
    // same thread
    if( m_writeDatabaseTimer.isActive() )
    {
        // write database in a thread so that it need not be written in destructor
        m_writeDatabaseTimer.stop();
        IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), SIGNAL(remove()) );
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
        ThreadWeaver::Weaver::instance()->enqueue( job );
    }
    else
        emit remove();  // CollectionManager will call deleteLater()
}

void
IpodCollection::slotEject()
{
    // this is not racy: destroy() is delivered to main thread, the timer fires in the
    // same thread
    if( m_writeDatabaseTimer.isActive() )
    {
        // write database now because iPod will be unmounted in destructor
        m_writeDatabaseTimer.stop();
        IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(slotPerformTeardownAndRemove()) );
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
        ThreadWeaver::Weaver::instance()->enqueue( job );
    }
    else
    {
        slotPerformTeardownAndRemove();
    }
}

void
IpodCollection::slotShowConfigureDialog( const QString &errorMessage )
{
    if( !m_configureDialog )
    {
        // create the dialog
        m_configureDialog = new KDialog();
        QWidget *settingsWidget = new QWidget( m_configureDialog );
        m_configureDialogUi.setupUi( settingsWidget );

        m_configureDialog->setButtons( KDialog::Ok | KDialog::Cancel );
        m_configureDialog->setMainWidget( settingsWidget );
        m_configureDialog->setWindowTitle( settingsWidget->windowTitle() );  // setupUi() sets this
        if( m_itdb )
        {
            // we will never initialize this iPod this time, hide ui for it completely
            m_configureDialogUi.modelComboLabel->hide();
            m_configureDialogUi.modelComboBox->hide();
            m_configureDialogUi.initializeLabel->hide();
            m_configureDialogUi.initializeButton->hide();
        }

        connect( m_configureDialogUi.initializeButton, SIGNAL(clicked(bool)), SLOT(slotInitialize()) );
        connect( m_configureDialog, SIGNAL(okClicked()), SLOT(slotApplyConfiguration()) );
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
    emit updated();
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
        slotShowConfigureDialog( errorMessage );
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
        connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
        ThreadWeaver::Weaver::instance()->enqueue( job );
    }
    else
        slotShowConfigureDialog( errorMessage ); // shows error message and allows initializing
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
        slotStartWriteDatabaseTimer(); // the change should be written down to file
        emit updated();  // no need to wait using startUpdateTimer()
    }

    QScopedPointer<Capabilities::TranscodeCapability> tc( create<Capabilities::TranscodeCapability>() );
    switch( m_configureDialogUi.transcodeComboBox->currentChoice() )
    {
        case Transcoding::SelectConfigWidget::DontChange:
            break;
        case Transcoding::SelectConfigWidget::JustCopy:
            tc->setSavedConfiguration( Transcoding::Configuration( Transcoding::JUST_COPY ) );
            break;
        case Transcoding::SelectConfigWidget::Forget:
            tc->setSavedConfiguration( Transcoding::Configuration( Transcoding::INVALID ) );
            break;
    }
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
    IpodWriteDatabaseJob *job = new IpodWriteDatabaseJob( this );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void IpodCollection::slotPerformTeardownAndRemove()
{
    /* try to eject the device from system. Following technique potentially catches more
     * cases than simply passing the udi from IpodCollectionFactory, think of fuse-based
     * filesystems for mounting iPhones et caetera.. */
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

    emit remove(); // CollectionManager will call deleteLater()
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
        emit startWriteDatabaseTimer();
    }
    track->setCollection( QWeakPointer<IpodCollection>( this ) );

    Meta::TrackPtr trackPtr( track );
    Meta::TrackPtr memTrack = MemoryMeta::MapChanger( m_mc.data() ).addTrack( trackPtr );
    if( !memTrack && justAdded )
    {
        /* this new track was not added to MemoryCollection, it may vanish soon, prevent
         * dangling pointer in m_itdb */
        itdb_playlist_remove_track( 0 /* = MPL */, itdbTrack );
        itdb_track_unlink( itdbTrack );
    }
    m_itdbMutex.unlock();

    if( memTrack )
    {
        subscribeTo( trackPtr );
        emit startUpdateTimer();
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
        emit startWriteDatabaseTimer();
    }

    emit startUpdateTimer();
}

bool IpodCollection::writeDatabase()
{
    if( !IpodDeviceHelper::safeToWrite( m_mountPoint, m_itdb ) ) // returns false if m_itdb is null
    {
        // we have to delete unmount-preventing file even in this case
        delete m_preventUnmountTempFile;
        m_preventUnmountTempFile = 0;
        warning() << "Refusing to write iTunes database to iPod becauase device is not safe to write";
        return false;
    }

    m_itdbMutex.lock();
    GError *error = 0;
    bool success = itdb_write( m_itdb, &error );
    m_itdbMutex.unlock();
    QString gpodError;
    if( error )
    {
        gpodError = QString::fromUtf8( error->message );
        g_error_free( error );
        error = 0;
    }
    delete m_preventUnmountTempFile;  // this deletes the file
    m_preventUnmountTempFile = 0;

    if( success )
    {
        QString message = i18nc( "%1: iPod collection name",
                         "iTunes database successfully written to %1", prettyName() );
        Amarok::Components::logger()->shortMessage( message );
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
        Amarok::Components::logger()->longMessage( message );
    }
    return success;
}

#include "IpodCollection.moc"
