//
// C++ Implementation: mediadevicepluginmanager
//
// Description:
//
//
// Authors: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005, 2006
//          Martin Aumueller <aumuell@reserv.at>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "MediaDevicePluginManager.h"

#include "amarok.h"
#include "debug.h"
#include "deviceconfiguredialog.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "MediaDeviceCache.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <KApplication>
#include <KComboBox>
#include <KConfig>
#include <KIconLoader>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>
#include <KVBox>
#include <Solid/Device>

#include <q3groupbox.h>
#include <QLabel>
#include <QLayout>
#include <QTextDocument>
#include <QToolTip>
#include <QWhatsThis>

typedef QMap<QString, Medium*> MediumMap;

MediaDevicePluginManagerDialog::MediaDevicePluginManagerDialog()
        : KDialog( Amarok::mainWindow() )
        , m_detectDevices( 0 )
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

    m_location = new Q3GroupBox( 1, Qt::Vertical, i18n( "Devices" ), vbox );
    m_location->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    m_devicesBox = new KVBox( m_location );
    m_devicesBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_manager = new MediaDevicePluginManager( m_devicesBox );

    KHBox *hbox = new KHBox( vbox );
    m_detectDevices = new KPushButton( i18n( "Autodetect Devices" ), hbox);
    m_detectDevices->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_detectDevices, SIGNAL( clicked() ), m_manager, SLOT( redetectDevices() ) );

    m_addButton = new KPushButton( i18n( "Add Device..." ), hbox );
    m_addButton->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_addButton, SIGNAL( clicked() ), m_manager, SLOT( newDevice() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
}

MediaDevicePluginManagerDialog::~MediaDevicePluginManagerDialog()
{
    disconnect( m_detectDevices, SIGNAL( clicked() ), m_manager, SLOT( redetectDevices() ) );
    disconnect( m_addButton, SIGNAL( clicked() ), m_manager, SLOT( newDevice() ) );
    disconnect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    delete m_manager;
}

void
MediaDevicePluginManagerDialog::slotOk()
{
    m_manager->finished();
    slotButtonClicked( Ok );
}

MediaDevicePluginManager::MediaDevicePluginManager( QWidget *widget, const bool nographics )
        : m_widget( widget )
{
    detectDevices( false, nographics );

    connect( this, SIGNAL( selectedPlugin( const QString &, const QString & ) ), MediaBrowser::instance(), SLOT( pluginSelected( const QString &, const QString & ) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceAdded(const QString &) ), this, SLOT( slotSolidDeviceAdded(const QString &) ) );
    connect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved(const QString &) ), this, SLOT( slotSolidDeviceRemoved(const QString &) ) );
}

MediaDevicePluginManager::~MediaDevicePluginManager()
{
    foreach( MediaDeviceConfig* mdc, m_deviceList )
        disconnect( mdc, SIGNAL(deleteDevice(QString &)), this, SLOT(deleteDevice(QString &)) );
    disconnect( this, SIGNAL( selectedPlugin( const QString &, const QString & ) ), MediaBrowser::instance(), SLOT( pluginSelected( const QString &, const QString & ) ) );
    disconnect( MediaDeviceCache::instance(), SIGNAL( deviceAdded(const QString &) ), this, SLOT( slotSolidDeviceAdded(const QString &) ) );
    disconnect( MediaDeviceCache::instance(), SIGNAL( deviceRemoved(const QString &) ), this, SLOT( slotSolidDeviceRemoved(const QString &) ) );
}

bool
MediaDevicePluginManager::detectDevices( const bool redetect, const bool nographics )
{
    bool foundNew = false;
    KConfigGroup config = Amarok::config( "PortableDevices" );
    if( redetect )
        MediaDeviceCache::instance()->refreshCache();
    QStringList deviceList = MediaDeviceCache::instance()->getAll();
    foreach( QString device, deviceList )
    {
        if( !config.readEntry( device, QString() ).isEmpty() &&
                config.readEntry( device, QString() ) == "deleted" && !redetect)
        {
            debug() << "skipping: deleted";
            continue;
        }

        bool skipflag = false;

        foreach( MediaDeviceConfig* mediadevice, m_deviceList )
        {
            if( device == mediadevice->uid() )
            {
                skipflag = true;
                debug() << "skipping: already listed";
            }
        }

        if( m_deletedMap.contains( device ) &&
                !MediaDeviceCache::instance()->deviceType( device ) == MediaDeviceCache::SolidType )
        {
            skipflag = true;
            debug() << "skipping: deleted & not autodetect";
        }

        if( skipflag )
            continue;

        if( m_deletedMap.contains( device ) )
            m_deletedMap.remove( device );

        MediaDeviceConfig *dev = new MediaDeviceConfig( device, this, nographics, m_widget );
        m_deviceList.append( dev );
        connect( dev, SIGNAL(deleteDevice(QString &)), this, SLOT(deleteDevice(QString &)) );

        foundNew = true;
    }

    return foundNew;
}

void
MediaDevicePluginManager::slotSolidDeviceAdded( const QString &uid )
{   
    MediaDeviceConfig *dev = new MediaDeviceConfig( uid, this, false, m_widget );
    m_deviceList.append( dev );
    //deleteDevice not needed...
}

void
MediaDevicePluginManager::slotSolidDeviceRemoved( const QString &uid )
{
    DEBUG_BLOCK
    debug() << "Trying to remove uid " << uid;
    for( int i = 0; i < m_deviceList.size(); ++i )
    {
        if( i < m_deviceList.size() && m_deviceList[i] && m_deviceList[i]->uid() == uid )
        {
            MediaDeviceConfig* config = m_deviceList[i];
            m_deviceList.removeAt( i );
            delete config;
            --i;
        }
    }
}

void
MediaDevicePluginManager::redetectDevices()
{
    if( !detectDevices( true ) )
    {
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n("No new media devices were found. If you feel this is an\n"
                                                                   "error, ensure that the DBUS and HAL daemons are running\n"
                                                                   "and KDE was built with support for them. You can test this\n"
                                                                   "by running\n"
                                                                   "     \"qdbus org.kde.kded /modules/mediamanager fullList\"\n"
                                                                   "in a Konsole window.") );
    }
}

void
MediaDevicePluginManager::deleteDevice( const QString &uid  )
{
    for( int i = 0; i < m_deviceList.size(); ++i )
    {
        if( m_deviceList[i]->uid() == uid )
        m_deletedMap[uid] = true;
    }
}

void
MediaDevicePluginManager::finished()
{
    foreach( MediaDeviceConfig* mediadevice, m_deviceList )
    {
        if( mediadevice->plugin() != mediadevice->oldPlugin() )
        {
            mediadevice->setOldPlugin( mediadevice->plugin() );
            emit selectedPlugin( mediadevice->uid(), mediadevice->plugin() );
        }
        mediadevice->configButton()->setEnabled( mediadevice->pluginCombo()->currentText() != i18n( "Do not handle" ) );
    }

    KConfigGroup config = Amarok::config( "MediaBrowser" );
    for( DeletedMap::Iterator dit = m_deletedMap.begin();
            dit != m_deletedMap.end();
            ++dit )
    {
        if( MediaDeviceCache::instance()->deviceType( dit.key() ) == MediaDeviceCache::SolidType )
            config.writeEntry( dit.key(), "deleted" );
        else
            config.deleteEntry( dit.key() );
        MediaDeviceCache::instance()->refreshCache();
    }
    m_deletedMap.clear();
}

void
MediaDevicePluginManager::newDevice()
{
    DEBUG_BLOCK
    ManualDeviceAdder* mda = new ManualDeviceAdder( this );
    int accepted = mda->exec();
    if( accepted == QDialog::Accepted && mda->successful() )
    {
        if( !Amarok::config( "PortableDevices" ).readEntry( mda->getId(), QString() ).isNull() )
        {
            //abort!  Can't have the same device defined twice...should never
            //happen due to name checking earlier...right?
            Amarok::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, you cannot define two devices\n"
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

ManualDeviceAdder::ManualDeviceAdder( MediaDevicePluginManager* mpm )
: KDialog( Amarok::mainWindow() )
{
    setObjectName( "manualdeviceadder" );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );


    m_mpm = mpm;
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
    m_mdaCombo->addItem( i18n( "Do not handle" ) );
    for( KService::List::ConstIterator it = MediaBrowser::instance()->getPlugins().begin();
            it != MediaBrowser::instance()->getPlugins().end();
            ++it )
        m_mdaCombo->addItem( (*it)->name() );

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

    connect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( comboChanged(const QString&) ) );
}

ManualDeviceAdder::~ManualDeviceAdder()
{
    delete m_mdaName;
    delete m_mdaMountPoint;
    disconnect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( comboChanged(const QString&) ) );
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
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, every device must have a name and\n"
                                                                   "you cannot define two devices with the\n"
                                                                   "same name. These names must be unique\n"
                                                                   "across autodetected devices as well.\n") );
    }
}

void
ManualDeviceAdder::comboChanged( const QString &string )
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
        m_mdaMountPoint->setText( "No mount point needed" );
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
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n( "The device name cannot contain the '|' character" ) );
        return QString();
    }
    m_newId = "manual|" + m_selectedPlugin + '|' +
            m_mdaName->text() + '|' +
            ( m_mdaMountPoint->text().isNull() ||
                m_mdaMountPoint->isEnabled() == false ?
                "(null)" : m_mdaMountPoint->text() );
    debug() << "returning id = " << m_newId;
    return m_newId;
}

MediaDeviceConfig::MediaDeviceConfig( QString uid, MediaDevicePluginManager *mgr, const bool nographics, QWidget *parent, const char *name )
    : KHBox( parent )
    , m_manager( mgr )
    , m_uid( uid )
    , m_name( MediaDeviceCache::instance()->deviceName( uid ) )
    , m_pluginCombo( 0 )
    , m_configButton( 0 )
    , m_removeButton( 0 )
    , m_label_details( 0 )
    , m_new( true )
{
    DEBUG_BLOCK
    setObjectName( name );

    KConfigGroup config = Amarok::config( "PortableDevices" );
    m_oldPlugin = config.readEntry( m_uid, QString() );
    if( !m_oldPlugin.isEmpty() )
        m_new = false;

    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    setSpacing( 5 );

    const QString labelTextNone = i18n( "(none)" );
    QString row = "<tr><td>%1</td><td>%2</td></tr>";
    QString table;
    table += row.arg( Qt::escape( i18n( "Autodetected:" ) ),
            Qt::escape( MediaDeviceCache::instance()->deviceType( m_uid ) == MediaDeviceCache::SolidType ? i18n("Yes") : i18n("No") ) );
    table += row.arg( Qt::escape( i18n( "Unique ID:" ) ),
            Qt::escape( m_uid ) );
    if( MediaDeviceCache::instance()->deviceType( m_uid ) == MediaDeviceCache::SolidType )
    {
        Solid::Device device( m_uid );
        if( device.isValid() )
        {
            if( !device.vendor().isEmpty() )
                table += row.arg( Qt::escape( i18n( "Vendor:" ) ),
                    Qt::escape( device.vendor() ) );
            if( !device.product().isEmpty() )
                table += row.arg( Qt::escape( i18n( "Product:" ) ),
                    Qt::escape( device.product() ) );
        }
    }

    QString title = Qt::escape( i18n( "Device information for " ) ) + "<b>" + m_uid + "</b>";
    m_details = QString( "<em>%1</em><br />" "<table>%2</table>" ).arg( title, table );

    QLabel* label_name = new QLabel( i18n("Name: "), this );
    Q_UNUSED( label_name );
    QLabel* label_devicename = new QLabel( m_name, this );
    Q_UNUSED( label_devicename );
    QLabel* m_label_details = new QLabel( "<qt>(<a href='details'>" + i18n( "Details" ) + "</a>)</qt>", this );
    m_label_details->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    connect( m_label_details, SIGNAL( linkActivated( const QString & ) ), this, SLOT( detailsActivated( const QString & ) ) );

    QLabel* label_plugin = new QLabel( i18n("Plugin:"), this );
    Q_UNUSED( label_plugin );
    m_pluginCombo = new KComboBox( false, this );
    m_pluginCombo->addItem( i18n( "Do not handle" ) );

    if( MediaDeviceCache::instance()->deviceType( m_uid ) == MediaDeviceCache::SolidType )
    {
        QString name = MediaBrowser::instance()->getDisplayPluginName( QString( MediaBrowser::instance()->deviceFromId( m_uid )->deviceType() + "-mediadevice" ) );
        debug() << "name of protocol from Solid is: " << name;
        if( name.isEmpty() )
        {
            m_pluginCombo->addItem( i18n( "Unknown" ) );
            m_pluginCombo->setCurrentIndex( m_pluginCombo->count() );
        }
        else
        {
            m_pluginCombo->addItem( name );
            m_pluginCombo->setCurrentIndex( m_pluginCombo->count() );
        }
    }
    for( KService::List::ConstIterator it = MediaBrowser::instance()->getPlugins().begin();
            it != MediaBrowser::instance()->getPlugins().end();
            ++it ){
        m_pluginCombo->addItem( (*it)->name() );
        if ( (*it)->property( "X-KDE-Amarok-name" ).toString() == config.readEntry( m_uid, QString() ) )
            m_pluginCombo->setCurrentItem( (*it)->name() );
    }
    m_pluginCombo->setEnabled( MediaDeviceCache::instance()->deviceType( m_uid ) == MediaDeviceCache::ManualType );

    m_configButton = new KPushButton( KIcon(KIcon( Amarok::icon( "configure" ) )), QString(), this );
    connect( m_configButton, SIGNAL(clicked()), this, SLOT(configureDevice()) );
    m_configButton->setEnabled( !m_new && m_pluginCombo->currentText() != i18n( "Do not handle" ) );
    m_configButton->setToolTip( i18n( "Configure device settings" ) );

    m_removeButton = new KPushButton( i18n( "Remove" ), this );
    connect( m_removeButton, SIGNAL(clicked()), this, SLOT(deleteDevice()) );
    m_removeButton->setToolTip( i18n( "Remove entries corresponding to this device from configuration file" ) );
    m_removeButton->setEnabled( MediaDeviceCache::instance()->deviceType( m_uid ) == MediaDeviceCache::ManualType );

    if( !nographics )
        show();
}

MediaDeviceConfig::~MediaDeviceConfig()
{
    disconnect( m_label_details, SIGNAL( linkActivated( const QString & ) ), this, SLOT( detailsActivated( const QString & ) ) );
    disconnect( m_configButton, SIGNAL(clicked()), this, SLOT(configureDevice()) );
    disconnect( m_removeButton, SIGNAL(clicked()), this, SLOT(deleteDevice()) );
}

void
MediaDeviceConfig::configureDevice() //slot
{
    MediaDevice *device = MediaBrowser::instance()->deviceFromId( m_uid );
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
MediaDeviceConfig::deleteDevice() //slot
{
    emit deleteDevice( m_uid );
    delete this;
}

void
MediaDeviceConfig::detailsActivated( const QString &link ) //slot
{
    Q_UNUSED( link );
    QWhatsThis::showText( QCursor::pos(), "<qt>" + m_details + "</qt>", Amarok::mainWindow() );
}

QString
MediaDeviceConfig::plugin()
{
    return MediaBrowser::instance()->getInternalPluginName( m_pluginCombo->currentText() );
}


#include "MediaDevicePluginManager.moc"
