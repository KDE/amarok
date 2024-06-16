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

#include "amarokconfig.h"
#include "ui_UmsConfiguration.h"
#include "collectionscanner/Track.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/umscollection/UmsCollectionLocation.h"
#include "core-impl/collections/umscollection/UmsTranscodeCapability.h"
#include "core-impl/meta/file/File.h"
#include "dialogs/OrganizeCollectionDialog.h"
#include "dialogs/TrackOrganizer.h" //TODO: move to core/utils
#include "scanner/GenericScanManager.h"

#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include <QStorageInfo>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <KConfigGroup>


UmsCollectionFactory::UmsCollectionFactory()
    : CollectionFactory()
{}

UmsCollectionFactory::~UmsCollectionFactory()
{
}

void
UmsCollectionFactory::init()
{
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
             this, &UmsCollectionFactory::slotAddSolidDevice );
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
             this, &UmsCollectionFactory::slotRemoveSolidDevice );

    // detect UMS devices that were already connected on startup
    QString query( "IS StorageAccess" );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( query );
    for( const Solid::Device &device : devices )
    {
        if( identifySolidDevice( device.udi() ) )
            createCollectionForSolidDevice( device.udi() );
    }
    m_initialized = true;
}

void
UmsCollectionFactory::slotAddSolidDevice( const QString &udi )
{
    if( m_collectionMap.contains( udi ) )
        return; // a device added twice (?)

    if( identifySolidDevice( udi ) )
        createCollectionForSolidDevice( udi );
}

void
UmsCollectionFactory::slotAccessibilityChanged( bool accessible, const QString &udi )
{
    if( accessible )
        slotAddSolidDevice( udi );
    else
        slotRemoveSolidDevice( udi );
}

void
UmsCollectionFactory::slotRemoveSolidDevice( const QString &udi )
{
    UmsCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotDestroy();
}

void
UmsCollectionFactory::slotRemoveAndTeardownSolidDevice( const QString &udi )
{
    UmsCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotEject();
}

void
UmsCollectionFactory::slotCollectionDestroyed( QObject *collection )
{
    // remove destroyed collection from m_collectionMap
    QMutableMapIterator<QString, UmsCollection *> it( m_collectionMap );
    while( it.hasNext() )
    {
        it.next();
        if( (QObject *) it.value() == collection )
            it.remove();
    }
}

bool
UmsCollectionFactory::identifySolidDevice( const QString &udi ) const
{
    Solid::Device device( udi );
    if( !device.is<Solid::StorageAccess>() )
        return false;
    // HACK to exclude iPods until UMS and iPod have common collection factory
    if( device.vendor().contains( "Apple", Qt::CaseInsensitive ) )
        return false;

    // everything okay, check whether the device is a data CD
    if( device.is<Solid::OpticalDisc>() )
    {
        const Solid::OpticalDisc *disc = device.as<Solid::OpticalDisc>();
        if( disc && ( disc->availableContent() & Solid::OpticalDisc::Data ) )
            return true;
        return false;
    }

    // check whether there is parent USB StorageDrive device
    while( device.isValid() )
    {
        if( device.is<Solid::StorageDrive>() )
        {
            Solid::StorageDrive *sd = device.as<Solid::StorageDrive>();
            if( sd->driveType() == Solid::StorageDrive::CdromDrive )
                return false;
            // USB Flash discs are usually hotpluggable, SD/MMC card slots are usually removable
            return sd->isHotpluggable() || sd->isRemovable();
        }
        device = device.parent();
    }
    return false; // no valid parent USB StorageDrive
}

void
UmsCollectionFactory::createCollectionForSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    Solid::Device device( udi );
    Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
    if( !ssa )
    {
        warning() << __PRETTY_FUNCTION__ << "called for non-StorageAccess device!?!";
        return;
    }
    if( ssa->isIgnored() )
    {
        debug() << "device" << udi << "ignored, ignoring :-)";
        return;
    }

    // we are definitely interested in this device, listen for accessibility changes
    disconnect( ssa, &Solid::StorageAccess::accessibilityChanged, this, nullptr );
    connect( ssa, &Solid::StorageAccess::accessibilityChanged,
             this, &UmsCollectionFactory::slotAccessibilityChanged );

    if( !ssa->isAccessible() )
    {
        debug() << "device" << udi << "not accessible, ignoring for now";
        return;
    }

    UmsCollection *collection = new UmsCollection( device );
    m_collectionMap.insert( udi, collection );

    // when the collection is destroyed by someone else, remove it from m_collectionMap:
    connect( collection, &QObject::destroyed, this, &UmsCollectionFactory::slotCollectionDestroyed );

    // try to gracefully destroy collection when unmounting is requested using
    // external means: (Device notifier plasmoid etc.). Because the original action could
    // fail if we hold some files on the device open, we try to tearDown the device too.
    connect( ssa, &Solid::StorageAccess::teardownRequested, this, &UmsCollectionFactory::slotRemoveAndTeardownSolidDevice );

    Q_EMIT newCollection( collection );
}

//UmsCollection

QString UmsCollection::s_settingsFileName( ".is_audio_player" );
QString UmsCollection::s_musicFolderKey( "audio_folder" );
QString UmsCollection::s_musicFilenameSchemeKey( "music_filenamescheme" );
QString UmsCollection::s_vfatSafeKey( "vfat_safe" );
QString UmsCollection::s_asciiOnlyKey( "ascii_only" );
QString UmsCollection::s_postfixTheKey( "ignore_the" );
QString UmsCollection::s_replaceSpacesKey( "replace_spaces" );
QString UmsCollection::s_regexTextKey( "regex_text" );
QString UmsCollection::s_replaceTextKey( "replace_text" );
QString UmsCollection::s_podcastFolderKey( "podcast_folder" );
QString UmsCollection::s_autoConnectKey( "use_automatically" );
QString UmsCollection::s_collectionName( "collection_name" );
QString UmsCollection::s_transcodingGroup( "transcoding" );

UmsCollection::UmsCollection( const Solid::Device &device )
    : Collection()
    , m_device( device )
    , m_mc( nullptr )
    , m_tracksParsed( false )
    , m_autoConnect( false )
    , m_musicFilenameScheme( "%artist%/%album%/%track% %title%" )
    , m_vfatSafe( true )
    , m_asciiOnly( false )
    , m_postfixThe( false )
    , m_replaceSpaces( false )
    , m_regexText( QString() )
    , m_replaceText( QString() )
    , m_collectionName( QString() )
    , m_scanManager( nullptr )
    , m_lastUpdated( 0 )
{
    debug() << "Creating UmsCollection for device with udi: " << m_device.udi();

    m_updateTimer.setSingleShot( true );
    connect( this, &UmsCollection::startUpdateTimer, this, &UmsCollection::slotStartUpdateTimer );
    connect( &m_updateTimer, &QTimer::timeout, this, &UmsCollection::collectionUpdated );

    m_configureAction = new QAction( QIcon::fromTheme( "configure" ), i18n( "&Configure Device" ), this );
    m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
    connect( m_configureAction, &QAction::triggered, this, &UmsCollection::slotConfigure );

    m_parseAction = new QAction( QIcon::fromTheme( "checkbox" ), i18n(  "&Activate This Collection" ), this );
    m_parseAction->setProperty( "popupdropper_svg_id", "edit" );
    connect( m_parseAction, &QAction::triggered, this, &UmsCollection::slotParseActionTriggered );

    m_ejectAction = new QAction( QIcon::fromTheme( "media-eject" ), i18n( "&Eject Device" ),
                                 const_cast<UmsCollection*>( this ) );
    m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );
    connect( m_ejectAction, &QAction::triggered, this, &UmsCollection::slotEject );

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
    Solid::StorageVolume *ssv = m_device.as<Solid::StorageVolume>();
    m_collectionId = ssv ? ssv->uuid() : m_device.udi();
    debug() << "Mounted at: " << m_mountPoint << "collection id:" << m_collectionId;

    // read .is_audio_player from filesystem
    KConfig config( m_mountPoint + QLatin1Char('/') + s_settingsFileName, KConfig::SimpleConfig );
    KConfigGroup entries = config.group( QString() ); // default group
    if( entries.hasKey( s_musicFolderKey ) )
    {
        m_musicUrl = QUrl::fromLocalFile( m_mountPoint );
        m_musicUrl = m_musicUrl.adjusted(QUrl::StripTrailingSlash);
        m_musicUrl.setPath(m_musicUrl.path() + QLatin1Char('/') + ( entries.readPathEntry( s_musicFolderKey, QString() ) ));
        m_musicUrl.setPath( QDir::cleanPath(m_musicUrl.path()) );
        if( !QDir( m_musicUrl.toLocalFile() ).exists() )
        {
            QString message = i18n( "File <i>%1</i> suggests that we should use <i>%2</i> "
                    "as music folder on the device, but it doesn't exist. Falling back to "
                    "<i>%3</i> instead", m_mountPoint + QLatin1Char('/') + s_settingsFileName,
                    m_musicUrl.toLocalFile(), m_mountPoint );
            Amarok::Logger::longMessage( message, Amarok::Logger::Warning );
            m_musicUrl = QUrl::fromLocalFile(m_mountPoint);
        }
    }
    else if( !entries.keyList().isEmpty() )
        // config file exists, but has no s_musicFolderKey -> music should be disabled
        m_musicUrl = QUrl();
    else
        m_musicUrl = QUrl::fromLocalFile(m_mountPoint); // related BR 259849
    QString scheme = entries.readEntry( s_musicFilenameSchemeKey );
    m_musicFilenameScheme = !scheme.isEmpty() ? scheme : m_musicFilenameScheme;
    m_vfatSafe = entries.readEntry( s_vfatSafeKey, m_vfatSafe );
    m_asciiOnly = entries.readEntry( s_asciiOnlyKey, m_asciiOnly );
    m_postfixThe = entries.readEntry( s_postfixTheKey, m_postfixThe );
    m_replaceSpaces = entries.readEntry( s_replaceSpacesKey, m_replaceSpaces );
    m_regexText = entries.readEntry( s_regexTextKey, m_regexText );
    m_replaceText = entries.readEntry( s_replaceTextKey, m_replaceText );
    if( entries.hasKey( s_podcastFolderKey ) )
    {
        m_podcastUrl = QUrl::fromLocalFile( m_mountPoint );
        m_podcastUrl = m_podcastUrl.adjusted(QUrl::StripTrailingSlash);
        m_podcastUrl.setPath(m_podcastUrl.path() + QLatin1Char('/') + ( entries.readPathEntry( s_podcastFolderKey, QString() ) ));
        m_podcastUrl.setPath( QDir::cleanPath(m_podcastUrl.path()) );
    }
    m_autoConnect = entries.readEntry( s_autoConnectKey, m_autoConnect );
    m_collectionName = entries.readEntry( s_collectionName, m_collectionName );

    m_mc = QSharedPointer<MemoryCollection>(new MemoryCollection());

    if( m_autoConnect )
        QTimer::singleShot( 0, this, &UmsCollection::slotParseTracks );
}

bool
UmsCollection::possiblyContainsTrack( const QUrl &url ) const
{
    //not initialized yet.
    if( m_mc.isNull() )
        return false;

    QString u = QUrl::fromPercentEncoding( url.url().toUtf8() );
    return u.startsWith( m_mountPoint ) || u.startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
UmsCollection::trackForUrl( const QUrl &url )
{
    //not initialized yet.
    if( m_mc.isNull() )
        return Meta::TrackPtr();

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

QString
UmsCollection::uidUrlProtocol() const
{
    return QStringLiteral( "file://" );
}

QString
UmsCollection::collectionId() const
{
    return m_collectionId;
}

QString
UmsCollection::prettyName() const
{
    QString actualName;
    if( !m_collectionName.isEmpty() )
        actualName = m_collectionName;
    else if( !m_device.description().isEmpty() )
        actualName = m_device.description();
    else
    {
        actualName = m_device.vendor().simplified();
        if( !actualName.isEmpty() )
            actualName += ' ';
        actualName += m_device.product().simplified();
    }

    if( m_tracksParsed )
        return actualName;
    else
        return i18nc( "Name of the USB Mass Storage collection that has not yet been "
                      "activated. See also the 'Activate This Collection' action; %1 is "
                      "actual collection name", "%1 (not activated)", actualName );
}

QIcon
UmsCollection::icon() const
{
    if( m_device.icon().isEmpty() )
        return QIcon::fromTheme( "drive-removable-media-usb-pendrive" );
    else
        return QIcon::fromTheme( m_device.icon() );
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
    return QStorageInfo( m_mountPoint ).bytesTotal()-QStorageInfo( m_mountPoint ).bytesFree();
}

float
UmsCollection::totalCapacity() const
{
    return QStorageInfo( m_mountPoint ).bytesTotal();
}

CollectionLocation *
UmsCollection::location()
{
    return new UmsCollectionLocation( this );
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
        case Capabilities::Capability::Transcode:
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
            if( m_tracksParsed )
            {
                actions << m_configureAction;
                actions << m_ejectAction;
            }
            else
            {
                actions << m_parseAction;
            }
            return new Capabilities::ActionsCapability( actions );
        }
        case Capabilities::Capability::Transcode:
            return new UmsTranscodeCapability( m_mountPoint + QLatin1Char('/') + s_settingsFileName,
                                               s_transcodingGroup );
        default:
            return nullptr;
    }
}

void
UmsCollection::metadataChanged(const Meta::TrackPtr &track )
{
    if( MemoryMeta::MapChanger( m_mc.data() ).trackChanged( track ) )
        // big-enough change:
        Q_EMIT startUpdateTimer();
}

QUrl
UmsCollection::organizedUrl( const Meta::TrackPtr &track, const QString &fileExtension ) const
{
    TrackOrganizer trackOrganizer( Meta::TrackList() << track );
    //%folder% prefix required to get absolute url.
    trackOrganizer.setFormatString( "%collectionroot%/" + m_musicFilenameScheme + ".%filetype%" );
    trackOrganizer.setVfatSafe( m_vfatSafe );
    trackOrganizer.setAsciiOnly( m_asciiOnly );
    trackOrganizer.setFolderPrefix( m_musicUrl.path() );
    trackOrganizer.setPostfixThe( m_postfixThe );
    trackOrganizer.setReplaceSpaces( m_replaceSpaces );
    trackOrganizer.setReplace( m_regexText, m_replaceText );
    if( !fileExtension.isEmpty() )
        trackOrganizer.setTargetFileExtension( fileExtension );

    return QUrl::fromLocalFile( trackOrganizer.getDestinations().value( track ) );
}

void
UmsCollection::slotDestroy()
{
    //TODO: stop scanner if running
    //unregister PlaylistProvider
    //CollectionManager will call destructor.
    Q_EMIT remove();
}

void
UmsCollection::slotEject()
{
    slotDestroy();
    Solid::StorageAccess *storageAccess = m_device.as<Solid::StorageAccess>();
    storageAccess->teardown();
}

void
UmsCollection::slotTrackAdded( const QUrl &location )
{
    Q_ASSERT( m_musicUrl.isParentOf( location ) || m_musicUrl.matches( location , QUrl::StripTrailingSlash) );
    MetaFile::Track *fileTrack = new MetaFile::Track( location );
    fileTrack->setCollection( this );
    Meta::TrackPtr fileTrackPtr = Meta::TrackPtr( fileTrack );
    Meta::TrackPtr proxyTrack = MemoryMeta::MapChanger( m_mc.data() ).addTrack( fileTrackPtr );
    if( proxyTrack )
    {
        subscribeTo( fileTrackPtr );
        Q_EMIT startUpdateTimer();
    }
    else
        warning() << __PRETTY_FUNCTION__ << "Failed to add" << fileTrackPtr->playableUrl()
                  << "to MemoryCollection. Perhaps already there?!?";
}

void
UmsCollection::slotTrackRemoved( const Meta::TrackPtr &track )
{
    Meta::TrackPtr removedTrack = MemoryMeta::MapChanger( m_mc.data() ).removeTrack( track );
    if( removedTrack )
    {
        unsubscribeFrom( removedTrack );
        // we only added MetaFile::Tracks, following static cast is safe
        static_cast<MetaFile::Track*>( removedTrack.data() )->setCollection( nullptr );
        Q_EMIT startUpdateTimer();
    }
    else
        warning() << __PRETTY_FUNCTION__ << "Failed to remove" << track->playableUrl()
                  << "from MemoryCollection. Perhaps it was never there?";
}

void
UmsCollection::collectionUpdated()
{
    m_lastUpdated = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT updated();
}

void
UmsCollection::slotParseTracks()
{
    if( !m_scanManager )
    {
        m_scanManager = new GenericScanManager( this );
        connect( m_scanManager, &GenericScanManager::directoryScanned,
                 this, &UmsCollection::slotDirectoryScanned );
    }

    m_tracksParsed = true;
    m_scanManager->requestScan( QList<QUrl>() << m_musicUrl, GenericScanManager::FullScan );
}

void
UmsCollection::slotParseActionTriggered()
{
    if( m_mc->trackMap().isEmpty() )
        QTimer::singleShot( 0, this, &UmsCollection::slotParseTracks );
}

void
UmsCollection::slotConfigure()
{
    QDialog umsSettingsDialog;
    QWidget *settingsWidget = new QWidget( &umsSettingsDialog );
    QScopedPointer<Capabilities::TranscodeCapability> tc( create<Capabilities::TranscodeCapability>() );

    Ui::UmsConfiguration *settings = new Ui::UmsConfiguration();
    settings->setupUi( settingsWidget );

    settings->m_autoConnect->setChecked( m_autoConnect );

    settings->m_musicFolder->setMode( KFile::Directory );
    settings->m_musicCheckBox->setChecked( !m_musicUrl.isEmpty() );
    settings->m_musicWidget->setEnabled( settings->m_musicCheckBox->isChecked() );
    settings->m_musicFolder->setUrl( m_musicUrl.isEmpty() ? QUrl::fromLocalFile( m_mountPoint ) : m_musicUrl );
    settings->m_transcodeConfig->fillInChoices( tc->savedConfiguration() );

    settings->m_podcastFolder->setMode( KFile::Directory );
    settings->m_podcastCheckBox->setChecked( !m_podcastUrl.isEmpty() );
    settings->m_podcastWidget->setEnabled( settings->m_podcastCheckBox->isChecked() );
    settings->m_podcastFolder->setUrl( m_podcastUrl.isEmpty() ? QUrl::fromLocalFile( m_mountPoint )
                                         : m_podcastUrl );

    settings->m_collectionName->setText( prettyName() );

    OrganizeCollectionWidget *layoutWidget = new OrganizeCollectionWidget;
    //TODO: save the setting that are normally written in onAccept()
//    connect( this, SIGNAL(accepted()), &layoutWidget, SLOT(onAccept()) );
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget( layoutWidget );
    settings->m_filenameSchemeBox->setLayout( layout );
    //hide the unuse preset selector.
    //TODO: change the presets to concurrent presets for regular albums v.s. compilations
    // layoutWidget.setformatPresetVisible( false );
    layoutWidget->setScheme( m_musicFilenameScheme );

    OrganizeCollectionOptionWidget *optionsWidget = new OrganizeCollectionOptionWidget;
    optionsWidget->setVfatCompatible( m_vfatSafe );
    optionsWidget->setAsciiOnly( m_asciiOnly );
    optionsWidget->setPostfixThe( m_postfixThe );
    optionsWidget->setReplaceSpaces( m_replaceSpaces );
    optionsWidget->setRegexpText( m_regexText );
    optionsWidget->setReplaceText( m_replaceText );

    layout->addWidget( optionsWidget );

    umsSettingsDialog.setLayout( new QVBoxLayout );
    QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    connect( buttonBox, &QDialogButtonBox::accepted, &umsSettingsDialog, &QDialog::accept );
    connect( buttonBox, &QDialogButtonBox::rejected, &umsSettingsDialog, &QDialog::reject );
    umsSettingsDialog.layout()->addWidget( settingsWidget );
    umsSettingsDialog.layout()->addWidget( buttonBox );
    umsSettingsDialog.setWindowTitle( i18n( "Configure USB Mass Storage Device" ) );

    if( umsSettingsDialog.exec() == QDialog::Accepted )
    {
        debug() << "accepted";

        if( settings->m_musicCheckBox->isChecked() )
        {
            if( settings->m_musicFolder->url() != m_musicUrl )
            {
                debug() << "music location changed from " << m_musicUrl.toLocalFile() << " to ";
                debug() << settings->m_musicFolder->url().toLocalFile();
                m_musicUrl = settings->m_musicFolder->url();
                //TODO: reparse music
            }
            QString scheme = layoutWidget->getParsableScheme().simplified();
            //protect against empty string.
            if( !scheme.isEmpty() )
                m_musicFilenameScheme = scheme;
        }
        else
        {
            debug() << "music support is disabled";
            m_musicUrl = QUrl();
            //TODO: remove all tracks from the MemoryCollection.
        }

        m_asciiOnly = optionsWidget->asciiOnly();
        m_postfixThe = optionsWidget->postfixThe();
        m_replaceSpaces = optionsWidget->replaceSpaces();
        m_regexText = optionsWidget->regexpText();
        m_replaceText = optionsWidget->replaceText();
        m_collectionName = settings->m_collectionName->text();

        if( settings->m_podcastCheckBox->isChecked() )
        {
            if( settings->m_podcastFolder->url() != m_podcastUrl )
            {
                debug() << "podcast location changed from " << m_podcastUrl << " to ";
                debug() << settings->m_podcastFolder->url().url();
                m_podcastUrl = QUrl(settings->m_podcastFolder->url());
                //TODO: reparse podcasts
            }
        }
        else
        {
            debug() << "podcast support is disabled";
            m_podcastUrl = QUrl();
            //TODO: remove the PodcastProvider
        }

        m_autoConnect = settings->m_autoConnect->isChecked();
        if( !m_musicUrl.isEmpty() && m_autoConnect )
            QTimer::singleShot( 0, this, &UmsCollection::slotParseTracks );

        // write the data to the on-disk file
        KConfig config( m_mountPoint + QLatin1Char('/') + s_settingsFileName, KConfig::SimpleConfig );
        KConfigGroup entries = config.group( QString() ); // default group
        if( !m_musicUrl.isEmpty() )
            entries.writePathEntry( s_musicFolderKey, QDir( m_mountPoint ).relativeFilePath( m_musicUrl.toLocalFile() ));
        else
            entries.deleteEntry( s_musicFolderKey );
        entries.writeEntry( s_musicFilenameSchemeKey, m_musicFilenameScheme );
        entries.writeEntry( s_vfatSafeKey, m_vfatSafe );
        entries.writeEntry( s_asciiOnlyKey, m_asciiOnly );
        entries.writeEntry( s_postfixTheKey, m_postfixThe );
        entries.writeEntry( s_replaceSpacesKey, m_replaceSpaces );
        entries.writeEntry( s_regexTextKey, m_regexText );
        entries.writeEntry( s_replaceTextKey, m_replaceText );
        if( !m_podcastUrl.isEmpty() )
            entries.writePathEntry( s_podcastFolderKey, QDir( m_mountPoint ).relativeFilePath( m_podcastUrl.toLocalFile() ));
        else
            entries.deleteEntry( s_podcastFolderKey );
        entries.writeEntry( s_autoConnectKey, m_autoConnect );
        entries.writeEntry( s_collectionName, m_collectionName );
        config.sync();

        tc->setSavedConfiguration( settings->m_transcodeConfig->currentChoice() );
    }

    delete settings;
}

void
UmsCollection::slotDirectoryScanned( QSharedPointer<CollectionScanner::Directory> dir )
{
    debug() << "directory scanned: " << dir->path();
    if( dir->tracks().isEmpty() )
    {
        debug() << "does not have tracks";
        return;
    }

    for( const CollectionScanner::Track *scannerTrack : dir->tracks() )
    {
        //TODO: use proxy tracks so no real file read is required
        // following method calls startUpdateTimer(), no need to Q_EMIT updated()
        slotTrackAdded( QUrl::fromLocalFile(scannerTrack->path()) );
    }

    //TODO: read playlists
}

void
UmsCollection::slotStartUpdateTimer()
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
