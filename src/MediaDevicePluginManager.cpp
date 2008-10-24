/******************************************************************************
 * Copyright (c) 2005,2006 Jeff Mitchell <kde-dev@emailgoeshere.com>          *
 *           (c) 2006 Martin Aumueller <aumuell@reserv.at>                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "MediaDevicePluginManager.h"

#include "Amarok.h"
#include "Debug.h"
#include "deviceconfiguredialog.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "MediaDeviceCache.h"
#include "plugin/pluginconfig.h"
#include "PluginManager.h"
#include "statusbar/StatusBar.h"

#include <KApplication>
#include <KComboBox>
#include <KConfig>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>
#include <KVBox>
#include <Solid/Device>
#include <Solid/StorageAccess>

#include <QFile>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <KMessageBox>
#include <QTextDocument>
#include <QTableWidget>
#include <QToolTip>
#include <QWhatsThis>

MediaDevicePluginManagerDialog::MediaDevicePluginManagerDialog()
        : KDialog( Amarok::mainWindow() )
        , m_genericDevices( 0 )
        , m_addButton( 0 )
        , m_devicesBox( 0 )
        , m_location( 0 )
        , m_manager( 0 )
{
    setObjectName( "mediadevicepluginmanagerdialog" );
    setModal( false );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Manage Devices and Plugins" ) ) );

    KVBox *vbox = new KVBox( this );
    setMainWidget( vbox );

    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_location = new QGroupBox( i18n( "Devices" ), vbox );
    m_location->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    m_devicesBox = new KVBox( m_location );
    m_devicesBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_manager = new MediaDevicePluginManager( m_devicesBox );

    KHBox *hbox = new KHBox( vbox );
    m_genericDevices = new KPushButton( i18n( "Generic Devices and Volumes..." ), hbox);
    m_genericDevices->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_genericDevices, SIGNAL( clicked() ), m_manager, SLOT( slotGenericVolumes() ) );

    m_addButton = new KPushButton( i18n( "Add Device..." ), hbox );
    m_addButton->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_addButton, SIGNAL( clicked() ), m_manager, SLOT( slowNewDevice() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
}

MediaDevicePluginManagerDialog::~MediaDevicePluginManagerDialog()
{
    disconnect( m_genericDevices, SIGNAL( clicked() ), m_manager, SLOT( slotGenericVolumes() ) );
    disconnect( m_addButton, SIGNAL( clicked() ), m_manager, SLOT( slotNewDevice() ) );
    delete m_manager;
}

void
MediaDevicePluginManagerDialog::slotOk()
{
    m_manager->finished();
    disconnect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
}

MediaDevicePluginManager::MediaDevicePluginManager( QWidget *widget )
        : m_widget( widget )
{
    detectDevices();

    connect( this, SIGNAL( selectedPlugin( const QString &, const QString & ) ), MediaBrowser::instance(), SLOT( pluginSelected( const QString &, const QString & ) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceAdded(const QString &) ), this, SLOT( slotSolidDeviceAdded(const QString &) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved(const QString &) ), this, SLOT( slotSolidDeviceRemoved(const QString &) ) );
}

MediaDevicePluginManager::~MediaDevicePluginManager()
{
    foreach( MediaDeviceConfig* mdc, m_deviceList )
        disconnect( mdc, SIGNAL(deleteDevice(const QString &)), this, SLOT(slotDeleteDevice(const QString &)) );
    disconnect( this, SIGNAL( selectedPlugin( const QString &, const QString & ) ), MediaBrowser::instance(), SLOT( pluginSelected( const QString &, const QString & ) ) );
    disconnect( MediaDeviceCache::instance(), SIGNAL( deviceAdded(const QString &) ), this, SLOT( slotSolidDeviceAdded(const QString &) ) );
    disconnect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved(const QString &) ), this, SLOT( slotSolidDeviceRemoved(const QString &) ) );
}

void
MediaDevicePluginManager::slotGenericVolumes()
{
    MediaDeviceVolumeMarkerDialog *mdvmd = new MediaDeviceVolumeMarkerDialog();
    mdvmd->exec();
    const QStringList addedList = mdvmd->getAddedList();
    const QStringList removedList = mdvmd->getRemovedList();
    delete mdvmd;
    foreach( const QString &udi, addedList )
        MediaBrowser::instance()->deviceAdded( udi );
    foreach( const QString &udi, removedList )
        MediaBrowser::instance()->deviceRemoved( udi );
    detectDevices();
}

bool
MediaDevicePluginManager::detectDevices()
{
    DEBUG_BLOCK
    bool foundNew = false;
    KConfigGroup config = Amarok::config( "PortableDevices" );
    MediaDeviceCache::instance()->refreshCache();
    QStringList udiList = MediaDeviceCache::instance()->getAll();
    foreach( const QString &udi, udiList )
    {
        debug() << "Checking device udi " << udi;

        bool skipflag = false;

        foreach( MediaDeviceConfig* mediadevice, m_deviceList )
        {
            if( udi == mediadevice->udi() )
            {
                skipflag = true;
                debug() << "skipping: already listed";
                if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidVolumeType &&
                        !MediaDeviceCache::instance()->isGenericEnabled( udi ) )
                    slotDeleteDevice( udi );
            }
        }

        if( skipflag )
            continue;

        if( MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidVolumeType &&
                !MediaDeviceCache::instance()->isGenericEnabled( udi ) )
        {
            debug() << "Device generic but not enabled, skipping.";
            continue;
        }
        else
            slotAddDevice( udi );
    }
    return foundNew;
}

void
MediaDevicePluginManager::slotSolidDeviceAdded( const QString &udi )
{
    foreach( MediaDeviceConfig* mediadevice, m_deviceList )
    {
        if( udi == mediadevice->udi() )
        {
            debug() << "skipping: already listed";
            return;
        }
    }
    int deviceType = MediaDeviceCache::instance()->deviceType( udi );
    if( deviceType == MediaDeviceCache::SolidPMPType ||
            ( deviceType == MediaDeviceCache::SolidVolumeType &&
              MediaDeviceCache::instance()->isGenericEnabled( udi ) ) )
    {
        MediaDeviceConfig *dev = new MediaDeviceConfig( udi, m_widget );
        m_deviceList.append( dev );
        connect( dev, SIGNAL(deleteDevice(const QString &)), this, SLOT(slotDeleteDevice(const QString&)) );
    }
    else
    {
        debug() << "device was not PMP, or was volume but not enabled" << endl;
    }
}

void
MediaDevicePluginManager::slotSolidDeviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    debug() << "Trying to remove udi " << udi;
    for( int i = 0; i < m_deviceList.size(); ++i )
    {
        if( i < m_deviceList.size() && m_deviceList[i] && m_deviceList[i]->udi() == udi )
        {
            MediaDeviceConfig* config = m_deviceList[i];
            m_deviceList.removeAt( i );
            delete config;
            --i;
        }
    }
}

void
MediaDevicePluginManager::slotDeleteDevice( const QString &udi )
{
    DEBUG_BLOCK
    for( int i = 0; i < m_deviceList.size(); ++i )
    {
        if( i < m_deviceList.size() && m_deviceList[i] && m_deviceList[i]->udi() == udi )
        {
            debug() << "putting device " << udi << " on deleted map";
            m_deletedList.append( udi );
            MediaDeviceConfig *config = m_deviceList[i];
            m_deviceList.removeAt( i );
            delete config;
            --i;
        }
    }
}

void
MediaDevicePluginManager::slotAddDevice( const QString &udi )
{
    DEBUG_BLOCK
    MediaDeviceConfig *dev = new MediaDeviceConfig( udi, m_widget );
    m_deviceList.append( dev );
    connect( dev, SIGNAL(deleteDevice(const QString &)), this, SLOT(slotDeleteDevice(const QString &)) );
}

void
MediaDevicePluginManager::finished()
{
    DEBUG_BLOCK
    foreach( MediaDeviceConfig* device, m_deviceList )
    {
        int deviceType = MediaDeviceCache::instance()->deviceType( device->udi() );
        if( deviceType == MediaDeviceCache::SolidPMPType || deviceType == MediaDeviceCache::SolidVolumeType )
            continue;
        debug() << "checking device " << device->udi();
        debug() << "plugin = " << device->plugin();
        debug() << "oldPlugin = " << device->oldPlugin();
        device->setOldPlugin( device->plugin() );
        emit selectedPlugin( device->udi(), device->plugin() );
    }

    KConfigGroup config = Amarok::config( "PortableDevices" );
    foreach( const QString &udi, m_deletedList )
    {
        config.deleteEntry( udi );
        emit selectedPlugin( udi, "dummy" );
    }
    MediaDeviceCache::instance()->refreshCache();
    m_deletedList.clear();
}

void
MediaDevicePluginManager::slotNewDevice()
{
    DEBUG_BLOCK
    ManualDeviceAdder* mda = new ManualDeviceAdder();
    int accepted = mda->exec();
    if( accepted == QDialog::Accepted && mda->successful() )
    {
        if( !Amarok::config( "PortableDevices" ).readEntry( mda->getId(), QString() ).isNull() )
        {
            //abort!  Can't have the same device defined twice...should never
            //happen due to name checking earlier...right?
            The::statusBar()->longMessage( i18n("Sorry, you cannot define two devices\n"
                                                                       "with the same name and mountpoint!") );
        }
        else
        {
            Amarok::config( "PortableDevices" ).writeEntry( mda->getId(), mda->getPlugin() );
            MediaDeviceCache::instance()->refreshCache();
            detectDevices();
        }
    }
    delete mda;
}

/////////////////////////////////////////////////////////////////////

ManualDeviceAdder::ManualDeviceAdder()
: KDialog( Amarok::mainWindow() )
{
    setObjectName( "manualdeviceadder" );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );


    m_successful = false;
    m_newId = QString();

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Add New Device") ) );

    KHBox* hbox = new KHBox( this );
    setMainWidget( hbox );
    hbox->setSpacing( KDialog::spacingHint() );

    KVBox* vbox1 = new KVBox( hbox );

    new QLabel( i18n( "Select the plugin to use with this device:"), vbox1 );
    m_mdaCombo = new KComboBox( false, vbox1 );
    m_mdaCombo->setObjectName( "m_mdacombo" );
    KService::List::ConstIterator end = MediaBrowser::instance()->getPlugins().constEnd();
    for( KService::List::ConstIterator it = MediaBrowser::instance()->getPlugins().constBegin();
            it != end;
            ++it )
        m_mdaCombo->addItem( (*it)->name() );
    if( m_mdaCombo->count() > 0 )
        m_selectedPlugin = MediaBrowser::instance()->getInternalPluginName( m_mdaCombo->itemText( 0 ) );

    new QLabel( "", vbox1 );
    QLabel* nameLabel = new QLabel( vbox1 );
    nameLabel->setText( i18n( "Enter a &name for this device (required):" ) );
    m_mdaName = new HintLineEdit( QString(), vbox1);
    nameLabel->setBuddy( m_mdaName );
    m_mdaName->setHint( i18n( "Example: My_Ipod" ) );
    m_mdaName->setToolTip( i18n( "Enter a name for the device.  The name must be unique across all manually added devices.  It must not contain the pipe ( | ) character." ) );

    new QLabel( "", vbox1 );
    QLabel* mpLabel = new QLabel( vbox1 );
    mpLabel->setText( i18n( "Enter the &mount point of the device, if applicable:" ) );
    m_mdaMountPoint = new HintLineEdit( QString(), vbox1);
    mpLabel->setBuddy( m_mdaMountPoint );
    m_mdaMountPoint->setHint( i18n( "Example: /mnt/ipod" ) );
    m_mdaMountPoint->setToolTip( i18n( "Enter the device's mount point.  Some devices (such as MTP devices) may not have a mount point and this can be ignored.  All other devices (iPods, UMS/VFAT devices) should enter the mount point here." ) );

    connect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( slotComboChanged(const QString&) ) );
}

ManualDeviceAdder::~ManualDeviceAdder()
{
    disconnect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( slotComboChanged(const QString&) ) );
    delete m_mdaCombo;
    delete m_mdaName;
    delete m_mdaMountPoint;
}

void
ManualDeviceAdder::slotButtonClicked( int button )
{
    DEBUG_BLOCK
    if( button != KDialog::Ok )
    {
        debug() << "mda dialog canceled";
        KDialog::slotButtonClicked( button );
    }
    if( !getId( true ).isEmpty() &&
            MediaDeviceCache::instance()->deviceType( m_newId ) == MediaDeviceCache::InvalidType )
    {
        debug() << "returning with m_successful = true";
        m_successful = true;
        KDialog::slotButtonClicked( button );
    }
    else
    {
        The::statusBar()->longMessage( i18n("Sorry, every device must have a name and\n"
                                              "you cannot define two devices with the\n"
                                              "same name. These names must be unique\n"
                                              "across autodetected devices as well.\n") );
    }
}

void
ManualDeviceAdder::slotComboChanged( const QString &string )
{
    DEBUG_BLOCK
    //best thing to do here would be to find out if the plugin selected
    //has m_hasMountPoint set to false...but any way to do this
    //without instantiating it?  This way will suffice for now...
    if( MediaBrowser::instance()->getInternalPluginName( string ) == "ifp-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "daap-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "mtp-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "njb-mediadevice" )
    {
        m_mountPointOldText = m_mdaMountPoint->text();
        m_mdaMountPoint->setText( i18n("No mount point needed") );
        m_mdaMountPoint->setEnabled(false);
    }
    else if( m_mdaMountPoint->isEnabled() == false )
    {
        m_mdaMountPoint->setText( m_mountPointOldText );
        m_mdaMountPoint->setEnabled(true);
    }
    m_selectedPlugin = MediaBrowser::instance()->getInternalPluginName( string );
    debug() << "Selected plugin = " << m_selectedPlugin;
}

QString
ManualDeviceAdder::getId( bool recreate )
{
    DEBUG_BLOCK
    if( !recreate )
    {
        debug() << "No recreate, returning " << m_newId;
        return m_newId;
    }

    if( !m_newId.isEmpty() && recreate )
    {
        m_newId = QString();
    }

    if( m_mdaMountPoint->isEnabled() == false && m_mdaName->text().isNull() )
    {
        debug() << "Mount point not enabled, device name is null";
        return QString();
    }
    if( m_mdaMountPoint->text().isNull() && m_mdaName->text().isNull() )
    {
        debug() << "Mount point text is null and device name is null";
        return QString();
    }
    if( m_mdaName->text().count( '|' ) )
    {
        The::statusBar()->longMessage( i18n( "The device name cannot contain the '|' character" ) );
        return QString();
    }
    m_newId = "manual|" + m_mdaName->text() + '|' +
        ( m_mdaMountPoint->text().isNull() ||
        m_mdaMountPoint->isEnabled() == false ?
            "(none)" : m_mdaMountPoint->text() );
    debug() << "returning id = " << m_newId;
    return m_newId;
}

MediaDeviceConfig::MediaDeviceConfig( QString udi, QWidget *parent, const char *name )
    : KHBox( parent )
    , m_udi( udi )
    , m_name( MediaDeviceCache::instance()->deviceName( udi ) )
    , m_oldPlugin( QString() )
    , m_details( QString() )
    , m_pluginCombo( 0 )
    , m_removeButton( 0 )
    , m_label_details( 0 )
    , m_new( true )
{
    DEBUG_BLOCK
    setObjectName( name );

    KConfigGroup config = Amarok::config( "PortableDevices" );
    m_oldPlugin = config.readEntry( m_udi, QString() );
    debug() << "oldPlugin = " << m_oldPlugin;
    if( !m_oldPlugin.isEmpty() )
        m_new = false;

    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    setSpacing( 5 );

    QString row = "<tr><td>%1</td><td>%2</td></tr>";
    QString table;
    int deviceType = MediaDeviceCache::instance()->deviceType( m_udi );
    table += row.arg( Qt::escape( i18n( "Autodetected:" ) ),
            Qt::escape( deviceType == MediaDeviceCache::SolidPMPType || deviceType == MediaDeviceCache::SolidVolumeType ? i18n("Yes") : i18n("No") ) );
    table += row.arg( Qt::escape( i18n( "Unique ID:" ) ),
            Qt::escape( m_udi ) );
    if( deviceType == MediaDeviceCache::SolidPMPType || deviceType == MediaDeviceCache::SolidVolumeType )
    {
        Solid::Device device( m_udi );
        if( device.isValid() )
        {
            if( deviceType == MediaDeviceCache::SolidPMPType )
            {
                if( !device.vendor().isEmpty() )
                    table += row.arg( Qt::escape( i18n( "Vendor:" ) ),
                        Qt::escape( device.vendor() ) );
                if( !device.product().isEmpty() )
                    table += row.arg( Qt::escape( i18n( "Product:" ) ),
                        Qt::escape( device.product() ) );
            }
            else if( deviceType == MediaDeviceCache::SolidVolumeType )
            {
                if( !device.parent().vendor().isEmpty() )
                    table += row.arg( Qt::escape( i18n( "Vendor:" ) ),
                        Qt::escape( device.parent().vendor() ) );
                if( !device.parent().product().isEmpty() )
                    table += row.arg( Qt::escape( i18n( "Product:" ) ),
                        Qt::escape( device.parent().product() ) );
                Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
                if( ssa )
                    table += row.arg( Qt::escape( i18n( "Mount Point:" ) ),
                            ( ssa->isAccessible() ?
                                Qt::escape( ssa->filePath() ) :
                                Qt::escape( i18n( "Not Mounted" ) ) ) );
            }
        }
    }

    QString title = Qt::escape( i18n( "Device information for " ) ) + "<b>" + m_udi + "</b>";
    m_details = QString( "<em>%1</em><br />" "<table>%2</table>" ).arg( title, table );

    QLabel* label_name = new QLabel( i18n("Name: "), this );
    Q_UNUSED( label_name );
    QLabel* label_devicename = new QLabel( m_name, this );
    Q_UNUSED( label_devicename );
    QLabel* m_label_details = new QLabel( "<qt>(<a href='details'>" + i18n( "Details" ) + "</a>)</qt>", this );
    m_label_details->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    debug() << "Making link activated, m_label_details = " << m_label_details;
    connect( m_label_details, SIGNAL( linkActivated(const QString &) ), this, SLOT( slotDetailsActivated(const QString &) ) );

    QLabel* label_plugin = new QLabel( i18n("Plugin:"), this );
    Q_UNUSED( label_plugin );
    m_pluginCombo = new KComboBox( false, this );

    if( deviceType == MediaDeviceCache::SolidPMPType )
    {
        debug() << "sending to getDisplayPluginName: " << MediaBrowser::instance()->deviceFromId( m_udi )->type();
        QString name = MediaBrowser::instance()->getDisplayPluginName( MediaBrowser::instance()->deviceFromId( m_udi )->type() );
        debug() << "name of protocol from Solid is: " << name;
        if( name.isEmpty() )
        {
            m_pluginCombo->addItem( i18n( "Unknown" ) );
        }
        else
        {
            debug() << "Adding " << name << " to pluginCombo";
            m_pluginCombo->addItem( name );
        }
    }
    else if( deviceType == MediaDeviceCache::SolidVolumeType )
    {
        m_pluginCombo->addItem( i18n( "Generic Audio Player" ) );
    }
    else
    {
        m_pluginCombo->addItem( i18n( "Do not handle" ) );
        KService::List::ConstIterator end = MediaBrowser::instance()->getPlugins().constEnd();
        for( KService::List::ConstIterator it = MediaBrowser::instance()->getPlugins().constBegin();
                it != end;
                ++it ){
            m_pluginCombo->addItem( (*it)->name() );
            if ( (*it)->property( "X-KDE-Amarok-name" ).toString() == config.readEntry( m_udi, QString() ) )
                m_pluginCombo->setCurrentItem( (*it)->name() );
        }
    }
    m_pluginCombo->setEnabled( deviceType == MediaDeviceCache::ManualType );

    m_removeButton = new KPushButton( i18n( "Remove" ), this );
    connect( m_removeButton, SIGNAL(clicked()), this, SLOT(slotDeleteDevice()) );
    m_removeButton->setToolTip( i18n( "Remove entries corresponding to this device from configuration file" ) );
    m_removeButton->setEnabled( deviceType == MediaDeviceCache::ManualType );
}

MediaDeviceConfig::~MediaDeviceConfig()
{
    DEBUG_BLOCK
    debug() << "null here? m_label_details = " << m_label_details;
    disconnect( m_label_details, SIGNAL( linkActivated(const QString &) ), this, SLOT( slotDetailsActivated(const QString &) ) );
    debug() << "or here?";
    disconnect( m_removeButton, SIGNAL(clicked()), this, SLOT(slotDeleteDevice()) );
}

void
MediaDeviceConfig::slotConfigureDevice() //slot
{
    MediaDevice *device = MediaBrowser::instance()->deviceFromId( m_udi );
    if( device )
    {
        DeviceConfigureDialog* dcd = new DeviceConfigureDialog( device );
        dcd->exec();
        delete dcd;
    }
    else
        debug() << "Could not show the configuration dialog because the device could not be found.";
}

void
MediaDeviceConfig::slotDeleteDevice() //slot
{
    DEBUG_BLOCK
    emit deleteDevice( m_udi );
}

void
MediaDeviceConfig::slotDetailsActivated( const QString &link ) //slot
{
    Q_UNUSED( link );
    QWhatsThis::showText( QCursor::pos(), "<qt>" + m_details + "</qt>", Amarok::mainWindow() );
}

QString
MediaDeviceConfig::plugin()
{
    return MediaBrowser::instance()->getInternalPluginName( m_pluginCombo->currentText() );
}

MediaDeviceVolumeMarkerDialog::MediaDeviceVolumeMarkerDialog()
        : KDialog( Amarok::mainWindow() )
        , m_mountPointBox( 0 )
        , m_table( 0 )
{
    setObjectName( "mediadevicevolumemarkerdialog" );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Mark Volumes as Media Devices" ) ) );

    m_mountPointBox = new KVBox( this );
    m_mountPointBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    setMainWidget( m_mountPointBox );

    KHBox *hbox = new KHBox( m_mountPointBox );
    m_table = new QTableWidget( 0, 2, hbox );
    QStringList headerlabels;
    headerlabels << i18n( "Mount Point:" ) << i18n( "Mark?" );
    m_table->setHorizontalHeaderLabels( headerlabels );
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_table->setShowGrid( false );

    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );

    MediaDeviceCache::instance()->refreshCache();
    QStringList udiList = MediaDeviceCache::instance()->getAll();
    foreach( const QString &udi, udiList )
    {
        if( !MediaDeviceCache::instance()->deviceType( udi ) == MediaDeviceCache::SolidVolumeType ||
                MediaDeviceCache::instance()->volumeMountPoint( udi ).isEmpty() )
        {
            debug() << "udi " << udi << " is not a volume, or mount point detected as empty";
            continue;
        }
        int row = m_table->rowCount();
        m_table->insertRow( row );
        QString mountPoint = MediaDeviceCache::instance()->volumeMountPoint( udi );
        QTableWidgetItem *item = new QTableWidgetItem( mountPoint );
        m_table->setItem( row, 0, item );
        item = new QTableWidgetItem();
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
        if( QFile::exists( mountPoint + "/.is_audio_player" ) )
            item->setCheckState( Qt::Checked );
        else
            item->setCheckState( Qt::Unchecked );
        m_table->setItem( row, 1, item );
    }
    m_table->setSortingEnabled( true );
}

MediaDeviceVolumeMarkerDialog::~MediaDeviceVolumeMarkerDialog()
{
}

void
MediaDeviceVolumeMarkerDialog::slotOk()
{
    QStringList udiList = MediaDeviceCache::instance()->getAll();
    for( int row = 0; row < m_table->rowCount(); row++ )
    {
        QString mountPoint = m_table->item( row, 0 )->text();
        bool checkedState = m_table->item( row, 1 )->checkState() == Qt::Checked ? true : false;
        bool initialState = QFile::exists( mountPoint + "/.is_audio_player" );
        if( initialState == checkedState )
            continue;
        if( initialState == false )
        {
            //attempt to write the file
            QFile file( mountPoint + "/.is_audio_player" );
            if( file.open( QIODevice::WriteOnly ) )
            {
                file.close();
                foreach( const QString &udi, udiList )
                {
                    if( MediaDeviceCache::instance()->volumeMountPoint( udi ) == mountPoint )
                        m_addedList.append( udi );
                }
            }
            else
                KMessageBox::sorry( this,
                                    i18n( "Could not create the marking file at %1.\n"
                                          "Ensure that you have the correct permissions\n"
                                          "to create that file.",
                                          QString( mountPoint + "/.is_audio_player" ) ),
                                    i18n( "Well, we tried..." )
                                  );
        }
        else
        {
            //attempt to delete the file
            bool success = QFile::remove( mountPoint + "/.is_audio_player" );
            if( !success )
            {
                KMessageBox::sorry( this,
                                    i18n( "Could not remove the marking file at %1.\n"
                                          "Ensure that you have the correct permissions\n"
                                          "to remove that file.",
                                          QString( mountPoint + "/.is_audio_player" ) ),
                                    i18n( "Well, we tried..." )
                                  );
            }
            else
            {
                foreach( const QString &udi, udiList )
                {
                    if( MediaDeviceCache::instance()->volumeMountPoint( udi ) == mountPoint )
                        m_removedList.append( udi );
                }
            }
        }
    }
    disconnect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
}

#include "MediaDevicePluginManager.moc"
