/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#define DEBUG_PREFIX "UmsCollection"

#include "UmsCollection.h"
#include "UmsCollectionLocation.h"

#include "amarokconfig.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/meta/file/File.h"

#include "dialogs/FilenameLayoutDialog.h"
#include "dialogs/TrackOrganizer.h" //TODO: move to core/utils

#include "scanner/GenericScanManager.h"

#include "ui_UmsConfiguration.h"

#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/genericinterface.h>
#include <solid/opticaldisc.h>
#include <solid/portablemediaplayer.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/storagevolume.h>

#include <KDiskFreeSpaceInfo>
#include <kmimetype.h>
#include <KUrl>

#include <QThread>
#include <QTimer>

AMAROK_EXPORT_COLLECTION( UmsCollectionFactory, umscollection )

UmsCollectionFactory::UmsCollectionFactory( QObject *parent, const QVariantList &args )
    : CollectionFactory( parent, args )
    , m_initialized( false )
{
    m_info = KPluginInfo( "amarok_collection-umscollection.desktop", "services" );
}

UmsCollectionFactory::~UmsCollectionFactory()
{
}

void
UmsCollectionFactory::init()
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString & ) ),
             SLOT( slotAddSolidDevice( const QString & ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString & ) ),
             SLOT( slotRemoveSolidDevice( const QString & ) ) );

    m_initialized = true;

    QList<Solid::Device> usbDrives =
            Solid::Device::listFromQuery( "[IS StorageDrive AND StorageDrive.bus=='Usb']" );
    foreach( Solid::Device drive, usbDrives )
    {
        debug() << "USB StorageDrive detected on startup: " << drive.udi();
        //search for volumes on this USB drive
        QList<Solid::Device> usbStorages =
                Solid::Device::listFromType( Solid::DeviceInterface::StorageAccess, drive.udi() );
        foreach( Solid::Device storage, usbStorages )
        {
            debug() << "partition on a USB drive discovered: " << storage.udi();
            //HACK: seems to be a bug in Solid
            if( !storage.is<Solid::StorageAccess>() )
                continue;
            //TODO: blacklisted devices
            slotAddSolidDevice( storage.udi() );
        }
    }
}

void
UmsCollectionFactory::slotAddSolidDevice( const QString &udi )
{
    if( m_umsCollectionMap.keys().contains( udi ) )
        return;

    Solid::Device device( udi );
    if( !device.is<Solid::StorageAccess>() )
        return;

    //HACK: ignore apple stuff until we have common MediaDeviceFactory.
    if( device.vendor().contains("apple", Qt::CaseInsensitive) )
        return;

    debug() << "Found new USB Mass Storage device with udi = " << device.udi();
    debug() << "Device name is " << device.product() << " and was made by " << device.vendor();
    debug() << "Device product is " << device.product();
    debug() << "Device description: " << device.description();

    UmsCollection *umsCollection = new UmsCollection( device );
    m_umsCollectionMap.insert( udi, umsCollection );
    emit newCollection( umsCollection );
}

void
UmsCollectionFactory::slotRemoveSolidDevice( const QString &udi )
{
    if( !m_umsCollectionMap.keys().contains( udi ) )
        return;

    UmsCollection *removedCollection = m_umsCollectionMap.take( udi );
    removedCollection->slotDeviceRemoved();
}

//UmsCollection

QString UmsCollection::s_settingsFileName( ".is_audio_player" );
QString UmsCollection::s_musicFolderKey( "audio_folder" );
QString UmsCollection::s_musicFilenameSchemeKey( "music_filenamescheme" );
QString UmsCollection::s_vfatSafeKey( "vfat_safe" );
QString UmsCollection::s_asciiOnlyKey( "ascii_only" );
QString UmsCollection::s_ignoreTheKey( "ignore_the" );
QString UmsCollection::s_replaceSpacesKey( "replace_spaces" );
QString UmsCollection::s_regexTextKey( "regex_text" );
QString UmsCollection::s_replaceTextKey( "replace_text" );
QString UmsCollection::s_podcastFolderKey( "podcast_folder" );
QString UmsCollection::s_autoConnectKey( "use_automatically" );

UmsCollection::UmsCollection( Solid::Device device )
    : Collection()
    , m_device( device )
    , m_mc( new MemoryCollection() )
    , m_initialized( false )
    , m_autoConnect( false )
    , m_vfatSafe( true )
    , m_asciiOnly( false )
    , m_ignoreThe( false )
    , m_replaceSpaces( false )
    , m_regexText( QString() )
    , m_replaceText( QString() )
    , m_scanManager( 0 )
{
    debug() << "creating device with udi: " << m_device.udi();
    Solid::StorageAccess *storageAccess = m_device.as<Solid::StorageAccess>();
    connect( storageAccess, SIGNAL(accessibilityChanged( bool, QString )),
             SLOT(slotAccessibilityChanged( bool, QString )) );

    m_configureAction = new QAction( KIcon( "configure" ), i18n( "&Configure %1", prettyName() ),
                this );
    m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
    connect( m_configureAction, SIGNAL( triggered() ), SLOT( slotConfigure() ) );

    m_parseAction = new QAction( KIcon( "checkbox" ), i18n(  "&Use as Collection" ), this );
    m_parseAction->setProperty( "popupdropper_svg_id", "edit" );
    connect( m_parseAction, SIGNAL( triggered() ), this, SLOT( slotParseActionTriggered() ) );

    m_ejectAction = new QAction( KIcon( "media-eject" ), i18n( "&Disconnect Device" ),
                                 const_cast<UmsCollection*>( this ) );
    m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );
    connect( m_ejectAction, SIGNAL( triggered() ), SLOT( slotEject() ) );

    if( storageAccess->isAccessible() )
        init();
}

UmsCollection::~UmsCollection()
{
    DEBUG_BLOCK
}

void
UmsCollection::init()
{
    Solid::StorageAccess *storageAccess = m_device.as<Solid::StorageAccess>();
    m_mountPoint = storageAccess->filePath();
    debug() << "Mounted at: " << m_mountPoint;

    //read .is_audio_player from filesystem
    KUrl playerFilePath( m_mountPoint );
    playerFilePath.addPath( s_settingsFileName );
    QFile playerFile( playerFilePath.toLocalFile() );
    //prevent BR 259849: no audio_folder key in .is_audio_player file.
    m_musicPath = m_mountPoint;

    if( playerFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        debug() << QString( "Found %1 file").arg( s_settingsFileName );
        QTextStream in(&playerFile);
        while( !in.atEnd() )
        {
            QString line = in.readLine();
            if( line.startsWith( s_musicFolderKey + "=" ) )
            {
                debug() << QString( "Found %1" ).arg( s_musicFolderKey );
                debug() << line;
                m_musicPath = KUrl( m_mountPoint );
                m_musicPath.addPath( line.section( '=', 1, 1 ) );
                m_musicPath.cleanPath();
                debug() << "Scan for music in " << m_musicPath.toLocalFile();
                if( !QDir( m_musicPath.toLocalFile() ).exists() )
                {
                    debug() << "Music path doesn't exist! Using the mountpoint instead";
                    //TODO: add user-visible warning after string freeze.
                    m_musicPath = m_mountPoint;
                }
            }
            else if( line.startsWith( s_musicFilenameSchemeKey + "=" ) )
            {
                m_musicFilenameScheme = line.section( '=', 1, 1 );
                debug() << QString( "filename scheme: %1" ).arg( m_musicFilenameScheme );
            }
            else if( line.startsWith( s_vfatSafeKey + "=" ) )
            {
                m_vfatSafe = ( line.section( '=', 1, 1 ) == "true" );
                debug() << "vfat compatible: " << m_vfatSafe;
            }
            else if( line.startsWith( s_asciiOnlyKey + "=" ) )
            {
                m_asciiOnly = ( line.section( '=', 1, 1 ) == "true" );
                debug() << "ASCII only: " << m_asciiOnly;
            }
            else if( line.startsWith( s_ignoreTheKey + "=" ) )
            {
                m_ignoreThe = ( line.section( '=', 1, 1 ) == "true" );
                debug() << "ignore The in artist names: " << m_ignoreThe;
            }
            else if( line.startsWith( s_replaceSpacesKey + "=" ) )
            {
                m_replaceSpaces = ( line.section( '=', 1, 1 ) == "true" );
                debug() << "replace spaces with underscores: " << m_replaceSpaces;
            }
            else if( line.startsWith( s_regexTextKey + "=" ) )
            {
                m_regexText = line.section( '=', 1, 1 );
                debug() << "Regex match string: " << m_regexText;
            }
            else if( line.startsWith( s_replaceTextKey + "=" ) )
            {
                m_replaceText = line.section( '=', 1, 1 );
                debug() << "Replace string: " << m_replaceText;
            }
            else if( line.startsWith( s_podcastFolderKey + "=" ) )
            {
                debug() << QString( "Found %1, initializing UMS podcast provider" )
                                .arg( s_podcastFolderKey );
                debug() << line;
                m_podcastPath = KUrl( m_mountPoint );
                m_podcastPath.addPath( line.section( '=', 1, 1 ) );
                m_podcastPath.cleanPath();
                debug() << "scan for podcasts in " <<
                        m_podcastPath.toLocalFile( KUrl::AddTrailingSlash );
            }
            else if( line.startsWith( s_autoConnectKey + "=" ) )
            {
                debug() << "Use automatically: " << line.section( '=', 1, 1 );
                m_autoConnect = ( line.section( '=', 1, 1 ) == "true" );
            }
        }
    }

    m_initialized = true;

    if( m_autoConnect )
        QTimer::singleShot( 0, this, SLOT(slotParseTracks()) );
}

void
UmsCollection::deInit()
{
    m_initialized = false;
    m_mc.clear();

    emit updated();
}

bool
UmsCollection::possiblyContainsTrack( const KUrl &url ) const
{
    QString u = QUrl::fromPercentEncoding( url.url().toUtf8() );
    return u.startsWith( m_mountPoint ) || u.startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
UmsCollection::trackForUrl( const KUrl &url )
{
    QString uid = QUrl::fromPercentEncoding( url.url().toUtf8() );
    if( uid.startsWith("file://") )
        uid = uid.remove( 0, 7 );
    return m_mc->trackMap().value( uid, Meta::TrackPtr() );
}

QueryMaker *
UmsCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

bool
UmsCollection::isDirInCollection( const QString &path )
{
    return path.startsWith( m_musicPath.path() );
}

QString
UmsCollection::uidUrlProtocol() const
{
    return QString( "file://" );
}

QString
UmsCollection::prettyName() const
{
    if( !m_device.description().isEmpty() )
        return m_device.description();

    QString name = m_device.vendor().simplified();
    if( !name.isEmpty() )
        name += " ";
    name += m_device.product().simplified();

    return name;
}

KIcon
UmsCollection::icon() const
{
    if( m_device.icon().isEmpty() )
        return KIcon( "drive-removable-media-usb-pendrive" );
    else
        return KIcon( m_device.icon() );
}

bool
UmsCollection::hasCapacity() const
{
    if( m_device.isValid() && m_device.is<Solid::StorageAccess>() )
        return m_device.as<Solid::StorageAccess>()->isAccessible();
    return false;
}

float
UmsCollection::usedCapacity() const
{
    return KDiskFreeSpaceInfo::freeSpaceInfo( m_mountPoint ).used();
}

float
UmsCollection::totalCapacity() const
{
    return KDiskFreeSpaceInfo::freeSpaceInfo( m_mountPoint ).size();
}

CollectionLocation *
UmsCollection::location() const
{
    return new UmsCollectionLocation( this );
}

bool
UmsCollection::isWritable() const
{
    //TODO: check writability of music folder
    return true;
}

bool
UmsCollection::isOrganizable() const
{
    return isWritable();
}

bool
UmsCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            return true;

        default:
            return false;
    }
}

Capabilities::Capability *
UmsCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        {
            QList<QAction *> actions;
            if( m_initialized )
            {
                //HACK: use a bool or something else less unsafe
                if( m_mc->trackMap().isEmpty() )
                    actions << m_parseAction;
                actions << m_configureAction;
                actions << m_ejectAction;
            }
            return new Capabilities::ActionsCapability( actions );
        }
        default:
            return 0;
    }
}

KUrl
UmsCollection::organizedUrl( Meta::TrackPtr track ) const
{
    TrackOrganizer trackOrganizer( Meta::TrackList() << track );
    //%folder% prefix required to get absolute url.
    trackOrganizer.setFormatString( "%folder%/" + m_musicFilenameScheme + ".%filetype%" );
    trackOrganizer.setVfatSafe( m_vfatSafe );
    trackOrganizer.setAsciiOnly( m_asciiOnly );
    trackOrganizer.setFolderPrefix( m_musicPath.path() );
    trackOrganizer.setIgnoreThe( m_ignoreThe );
    trackOrganizer.setReplaceSpaces( m_replaceSpaces );
    trackOrganizer.setReplace( m_regexText, m_replaceText );

    return KUrl( trackOrganizer.getDestinations().value( track ) );
}

void
UmsCollection::slotDeviceRemoved()
{
    //TODO: stop scanner if running
    //unregister PlaylistProvider
    //CollectionManager will call destructor.
    emit remove();
}

void
UmsCollection::slotTrackAdded( KUrl location )
{
    Q_ASSERT( m_musicPath.isParentOf( location ) );
    MetaFile::TrackPtr track = MetaFile::TrackPtr( new MetaFile::Track( location ) );
    MemoryMeta::MapAdder( m_mc.data() ).addTrack( Meta::TrackPtr::dynamicCast( track ) );
}

void
UmsCollection::slotAccessibilityChanged( bool accessible, const QString &udi )
{
    Q_UNUSED(udi)
    if( accessible )
        init();
    else
        deInit();
}

void
UmsCollection::slotParseTracks()
{
    if( !m_scanManager )
    {
        m_scanManager = new GenericScanManager( this );
        connect( m_scanManager, SIGNAL(directoryScanned( CollectionScanner::Directory * )),
                SLOT(slotDirectoryScanned(CollectionScanner::Directory*)), Qt::DirectConnection );
    }

    m_scanManager->requestFullScan( QList<KUrl>() << m_musicPath );
}

void
UmsCollection::slotParseActionTriggered()
{
    if( m_mc->trackMap().isEmpty() )
        QTimer::singleShot( 0, this, SLOT(slotParseTracks()) );
}

void
UmsCollection::slotConfigure()
{
    KDialog umsSettingsDialog;
    QWidget *settingsWidget = new QWidget( &umsSettingsDialog );

    Ui::UmsConfiguration *settings = new Ui::UmsConfiguration();
    settings->setupUi( settingsWidget );

    settings->m_autoConnect->setChecked( m_autoConnect );

    settings->m_musicFolder->setMode( KFile::Directory );
    settings->m_musicCheckBox->setChecked( !m_musicPath.isEmpty() );
    settings->m_musicWidget->setEnabled( settings->m_musicCheckBox->isChecked() );
    settings->m_musicFolder->setUrl( m_musicPath.isEmpty() ? KUrl( m_mountPoint ) : m_musicPath );

    settings->m_podcastFolder->setMode( KFile::Directory );
    settings->m_podcastCheckBox->setChecked( !m_podcastPath.isEmpty() );
    settings->m_podcastWidget->setEnabled( settings->m_podcastCheckBox->isChecked() );
    settings->m_podcastFolder->setUrl( m_podcastPath.isEmpty() ? KUrl( m_mountPoint )
                                         : m_podcastPath );

    FilenameLayoutDialog filenameLayoutDialog( &umsSettingsDialog, 1 );
    //TODO: save the setting that are normally written in onAccept()
//    connect( this, SIGNAL(accepted()), &filenameLayoutDialog, SLOT(onAccept()) );
    QVBoxLayout layout( &umsSettingsDialog );
    layout.addWidget( &filenameLayoutDialog );
    settings->m_filenameSchemeBox->setLayout( &layout );
    //hide the unuse preset selector.
    //TODO: change the presets to concurent presets for regular albums v.s. compilations
    filenameLayoutDialog.setformatPresetVisible( false );

    filenameLayoutDialog.setScheme( m_musicFilenameScheme );
    filenameLayoutDialog.setVfatCompatible( m_vfatSafe );
    filenameLayoutDialog.setAsciiOnly( m_asciiOnly );
    filenameLayoutDialog.setIgnoreThe( m_ignoreThe );
    filenameLayoutDialog.setReplaceSpaces( m_replaceSpaces );
    filenameLayoutDialog.setRegexpText( m_regexText );
    filenameLayoutDialog.setReplaceText( m_replaceText );

    umsSettingsDialog.setButtons( KDialog::Ok | KDialog::Cancel );
    umsSettingsDialog.setMainWidget( settingsWidget );

    umsSettingsDialog.setWindowTitle( i18n( "Configure USB Mass Storage Device" ) );

    if( umsSettingsDialog.exec() == QDialog::Accepted )
    {
        debug() << "accepted";

        if( settings->m_musicCheckBox->isChecked() )
        {
            if( settings->m_musicFolder->url() != m_musicPath )
            {
                debug() << "music location changed from " << m_musicPath.toLocalFile() << " to ";
                debug() << settings->m_musicFolder->url().toLocalFile();
                m_musicPath = settings->m_musicFolder->url();
                //TODO: reparse music
            }
            m_musicFilenameScheme = filenameLayoutDialog.getParsableScheme().simplified();
        }
        else
        {
            debug() << "music support is disabled";
            m_musicPath = KUrl();
            //TODO: remove all tracks from the MemoryCollection.
        }

        m_vfatSafe = filenameLayoutDialog.vfatCompatible();
        m_asciiOnly = filenameLayoutDialog.asciiOnly();
        m_ignoreThe = filenameLayoutDialog.ignoreThe();
        m_replaceSpaces = filenameLayoutDialog.replaceSpaces();
        m_regexText = filenameLayoutDialog.regexpText();
        m_replaceText = filenameLayoutDialog.replaceText();

        if( settings->m_podcastCheckBox->isChecked() )
        {
            if( settings->m_podcastFolder->url() != m_podcastPath )
            {
                debug() << "podcast location changed from " << m_podcastPath << " to ";
                debug() << settings->m_podcastFolder->url().url();
                m_podcastPath = settings->m_podcastFolder->url().toLocalFile();
                //TODO: reparse podcasts
            }
        }
        else
        {
            debug() << "podcast support is disabled";
            m_podcastPath = KUrl();
            //TODO: remove the PodcastProvider
        }

        m_autoConnect = settings->m_autoConnect->isChecked();
        if( !m_musicPath.isEmpty() && m_autoConnect )
            QTimer::singleShot( 0, this, SLOT(slotParseTracks()) );

        //write the date to the on-disk file
        KUrl localFile = KUrl( m_mountPoint );
        localFile.addPath( s_settingsFileName );
        QFile settingsFile( localFile.toLocalFile() );
        if( settingsFile.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
        {
            QTextStream s( &settingsFile );
            QString keyValuePair( "%1=%2\n" );

#define TRUE_FALSE(x) x ? "true" : "false"
            s << keyValuePair.arg( s_autoConnectKey, TRUE_FALSE(m_autoConnect) );

            if( !m_musicPath.isEmpty() )
            {
                s << keyValuePair.arg( s_musicFolderKey, KUrl::relativePath( m_mountPoint,
                    m_musicPath.toLocalFile() ) );
            }

            if( !m_musicFilenameScheme.isEmpty() )
            {
                s << keyValuePair.arg( s_musicFilenameSchemeKey, m_musicFilenameScheme );
            }

            s << keyValuePair.arg( s_vfatSafeKey, TRUE_FALSE(m_vfatSafe) );
            s << keyValuePair.arg( s_asciiOnlyKey, TRUE_FALSE(m_asciiOnly) );
            s << keyValuePair.arg( s_ignoreTheKey, TRUE_FALSE(m_ignoreThe) );
            s << keyValuePair.arg( s_replaceSpacesKey, TRUE_FALSE(m_replaceSpaces) );
            s << keyValuePair.arg( s_regexTextKey, m_regexText );
            s << keyValuePair.arg( s_replaceTextKey, m_replaceText );

            if( !m_podcastPath.isEmpty() )
            {
                s << keyValuePair.arg( s_podcastFolderKey, KUrl::relativePath( m_mountPoint,
                    m_podcastPath.toLocalFile() ) );
            }

            settingsFile.close();
        }
        else
            error() << "Could not open settingsfile " << localFile.toLocalFile();
    }

    delete settings;
}

void
UmsCollection::slotEject()
{
    Solid::StorageAccess *storageAccess = m_device.as<Solid::StorageAccess>();
    storageAccess->teardown();
}

void
UmsCollection::slotDirectoryScanned( CollectionScanner::Directory *dir )
{
    debug() << "directory scanned: " << dir->path();
    if( dir->tracks().isEmpty() )
    {
        debug() << "does not have tracks";
        return;
    }

    foreach( const CollectionScanner::Track *scannerTrack, dir->tracks() )
    {
        //TODO: use proxy tracks so no real file read is required
        slotTrackAdded( scannerTrack->path() );
    }

    emit updated();

    //TODO: read playlists
}
