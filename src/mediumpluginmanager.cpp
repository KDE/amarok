//
// C++ Implementation: mediumpluginmanager
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
#include "amarok.h"
#include "debug.h"
#include "deviceconfiguredialog.h"
#include "mediadevicemanager.h"
#include "devicemanager.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginmanager.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <q3groupbox.h>
#include <QLabel>
#include <QLayout>
#include <QSignalMapper>
#include <QToolTip>
#include <kvbox.h>
#include <QAbstractButton>

#include <kapplication.h>
#include <k3activelabel.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kvbox.h>

using Amarok::escapeHTMLAttr;

typedef QMap<QString, Medium*> MediumMap;

MediumPluginManagerDialog::MediumPluginManagerDialog()
        : KPageDialog( Amarok::mainWindow() )
{
    setObjectName( "mediumpluginmanagerdialog" );
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

    m_manager = new MediumPluginManager( m_devicesBox );

    KHBox *hbox = new KHBox( vbox );
    KPushButton *detectDevices = new KPushButton( i18n( "Autodetect Devices" ), hbox);
    detectDevices->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( detectDevices, SIGNAL( clicked() ), m_manager, SLOT( redetectDevices() ) );

    KPushButton *addButton = new KPushButton( i18n( "Add Device..." ), hbox );
    addButton->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( addButton, SIGNAL( clicked() ), m_manager, SLOT( newDevice() ) );
    connect( this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}

MediumPluginManagerDialog::~MediumPluginManagerDialog()
{
    delete m_manager;
}

void
MediumPluginManagerDialog::slotOk()
{
    m_manager->finished();
    //KPageDialog::slotOk();
    slotButtonClicked( Ok );
}

MediumPluginManager::MediumPluginManager( QWidget *widget, const bool nographics )
: m_widget( widget )
{
    detectDevices( false, nographics );

    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );
}

MediumPluginManager::~MediumPluginManager()
{
}

bool
MediumPluginManager::hasChanged()
{
    bool temp = m_hasChanged;
    m_hasChanged = false;
    return temp;
}

void
MediumPluginManager::slotChanged()//slot
{
    m_hasChanged = true;
    emit changed();
}

bool
MediumPluginManager::detectDevices( const bool redetect, const bool nographics )
{
    bool foundNew = false;
    KSharedConfigPtr config = Amarok::config( "MediaBrowser" );
    if( redetect )
        DeviceManager::instance()->reconcileMediumMap();
    MediumMap mmap = MediaDeviceManager::instance()->getMediumMap();
    for( MediumMap::Iterator it = mmap.begin(); it != mmap.end(); it++ )
    {
        if( !config->readEntry( (*it)->id(), QString() ).isEmpty() &&
                config->readEntry( (*it)->id(), QString() ) == "deleted" && !redetect)
        {
            debug() << "skipping: deleted" << endl;
            continue;
        }

        bool skipflag = false;

        for( DeviceList::Iterator dit = m_deviceList.begin();
                dit != m_deviceList.end();
                dit++ )
        {
            if( (*it)->id() == (*dit)->medium()->id() )
            {
                skipflag = true;
                debug() << "skipping: already listed" << endl;
            }
        }

        if( m_deletedMap.contains( (*it)->id() ) && !(*it)->isAutodetected() )
        {
            skipflag = true;
            debug() << "skipping: deleted & not autodetect" << endl;
        }

        if( skipflag )
            continue;

        if( m_deletedMap.contains( (*it)->id() ) )
            m_deletedMap.remove( (*it)->id() );

        MediaDeviceConfig *dev = new MediaDeviceConfig( *it, this, nographics, m_widget );
        m_deviceList.append( dev );
        connect( dev, SIGNAL(deleteMedium(Medium *)), SLOT(deleteMedium(Medium *)) );

        foundNew = true;
    }

    return foundNew;
}

void
MediumPluginManager::redetectDevices()
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
    else
        slotChanged();
}

void
MediumPluginManager::deleteMedium( Medium *medium )
{
    for( DeviceList::Iterator it = m_deviceList.begin();
            it != m_deviceList.end();
            it++ )
    {
        if( (*it)->medium() == medium )
        {
            m_deletedMap[medium->id()] = medium;
            m_deviceList.remove( *it );
            break;
        }
    }
    slotChanged();
}

void
MediumPluginManager::finished()
{
    for( DeviceList::Iterator it = m_deviceList.begin();
            it != m_deviceList.end();
            it++ )
    {
        if( (*it)->plugin() != (*it)->oldPlugin() )
        {
            (*it)->setOldPlugin( (*it)->plugin() );
            emit selectedPlugin( (*it)->medium(), (*it)->plugin() );
        }
        (*it)->configButton()->setEnabled( (*it)->pluginCombo()->currentText() != i18n( "Do not handle" ) );
    }

    KSharedConfig::Ptr config = Amarok::config( "MediaBrowser" );
    for( DeletedMap::Iterator dit = m_deletedMap.begin();
            dit != m_deletedMap.end();
            ++dit )
    {
        if( dit.data()->isAutodetected() )
            config->writeEntry( dit.data()->id(), "deleted" );
        else
            config->deleteEntry( dit.data()->id() );
        MediaDeviceManager::instance()->removeManualDevice( dit.data() );
    }
    m_deletedMap.clear();
}

void
MediumPluginManager::newDevice()
{
    DEBUG_BLOCK
    ManualDeviceAdder* mda = new ManualDeviceAdder( this );
    if( mda->exec() == QDialog::Accepted && mda->successful() )
    {
        if( !Amarok::config( "MediaBrowser" )->readEntry( mda->getMedium()->id(), QString() ).isNull() )
        {
            //abort!  Can't have the same device defined twice...should never
            //happen due to name checking earlier...right?
            Amarok::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, you cannot define two devices\n"
                                                                       "with the same name and mountpoint!") );
        }
        else
        {
            Medium *newdev = new Medium( mda->getMedium() );
            Amarok::config( "MediaBrowser" )->writeEntry( newdev->id(), mda->getPlugin() );
            MediaDeviceManager::instance()->addManualDevice( newdev );
            detectDevices();
        }
    }
    delete mda;
    slotChanged();
}

/////////////////////////////////////////////////////////////////////

ManualDeviceAdder::ManualDeviceAdder( MediumPluginManager* mpm )
: KPageDialog( Amarok::mainWindow() )
{
    setObjectName( "manualdeviceadder" );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );


    m_mpm = mpm;
    m_successful = false;
    m_newMed = 0;

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
    m_mdaName = new HintLineEdit( QString::null, vbox1);
    nameLabel->setBuddy( m_mdaName );
    m_mdaName->setHint( i18n( "Example: My_Ipod" ) );
    m_mdaName->setToolTip( i18n( "Enter a name for the device.  The name must be unique across all devices, including autodetected devices.  It must not contain the pipe ( | ) character." ) );

    new QLabel( "", vbox1 );
    QLabel* mpLabel = new QLabel( vbox1 );
    mpLabel->setText( i18n( "Enter the &mount point of the device, if applicable:" ) );
    m_mdaMountPoint = new HintLineEdit( QString::null, vbox1);
    mpLabel->setBuddy( m_mdaMountPoint );
    m_mdaMountPoint->setHint( i18n( "Example: /mnt/ipod" ) );
    m_mdaMountPoint->setToolTip( i18n( "Enter the device's mount point.  Some devices (such as iRiver iFP devices) may not have a mount point and this can be ignored.  All other devices (iPods, UMS/VFAT devices) should enter the mount point here." ) );

    connect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( comboChanged(const QString&) ) );
}

ManualDeviceAdder::~ManualDeviceAdder()
{
    delete m_newMed;
    delete m_mdaName;
    delete m_mdaMountPoint;
}

void
ManualDeviceAdder::slotButtonClicked( KDialog::ButtonCode button)
{
    if( button != KDialog::Ok )
        KDialog::slotButtonClicked( button );
    if( getMedium( true ) && !getMedium()->name().isEmpty() &&
            MediaDeviceManager::instance()->getDevice( getMedium()->name() ) == NULL )
    {
        m_successful = true;
        slotButtonClicked( Ok );
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
    //best thing to do here would be to find out if the plugin selected
    //has m_hasMountPoint set to false...but any way to do this
    //without instantiating it?  This way will suffice for now...
    if( MediaBrowser::instance()->getInternalPluginName( string ) == "ifp-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "daap-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "mtp-mediadevice" ||
            MediaBrowser::instance()->getInternalPluginName( string ) == "njb-mediadevice" )
    {
        m_comboOldText = m_mdaMountPoint->text();
        m_mdaMountPoint->setText( QString::null );
        m_mdaMountPoint->setEnabled(false);
    }
    else if( m_mdaMountPoint->isEnabled() == false )
    {
        m_mdaMountPoint->setText( m_comboOldText );
        m_mdaMountPoint->setEnabled(true);
    }
    m_selectedPlugin = MediaBrowser::instance()->getInternalPluginName( string );
}

Medium*
ManualDeviceAdder::getMedium( bool recreate )
{
    if( !recreate )
        return m_newMed;

    if( m_newMed && recreate )
    {
        delete m_newMed;
        m_newMed = 0;
    }

    if( m_mdaMountPoint->isEnabled() == false &&
            m_mdaName->text().isNull() )
        return NULL;
    if( m_mdaMountPoint->text().isNull() &&
            m_mdaName->text().isNull() )
        return NULL;
    QString id = "manual|" + m_mdaName->text() + '|' +
            ( m_mdaMountPoint->text().isNull() ||
                m_mdaMountPoint->isEnabled() == false ?
                "(null)" : m_mdaMountPoint->text() );
    m_newMed = new Medium( id, m_mdaName->text() );
    m_newMed->setAutodetected( false );
    m_newMed->setMountPoint( m_mdaMountPoint->text() );
    return m_newMed;
}

MediaDeviceConfig::MediaDeviceConfig( Medium *medium, MediumPluginManager *mgr, const bool nographics, QWidget *parent, const char *name )
: KHBox( parent )
, m_manager( mgr )
, m_medium( medium )
, m_configButton( 0 )
, m_removeButton( 0 )
, m_new( true )
{
    if( !m_medium )
        return;

    KSharedConfig::Ptr config = Amarok::config( "MediaBrowser" );
    m_oldPlugin = config->readEntry( m_medium->id(), QString() );
    if( !m_oldPlugin.isEmpty() )
        m_new = false;

    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    setSpacing( 5 );

    const QString labelTextNone = i18n( "(none)" );
    QString row = "<tr><td>%1</td><td>%2</td></tr>";
    QString table;
    table += row.arg( Qt::escape( i18n( "Autodetected:" ) ),
            Qt::escape( medium->isAutodetected() ? i18n("Yes") : i18n("No") ) );
    table += row.arg( Qt::escape( i18n( "ID:" ) ),
            Qt::escape( medium->id() ) );
    table += row.arg( Qt::escape( i18n( "Name:" ) ),
            Qt::escape( medium->name() ) );
    table += row.arg( Qt::escape( i18n( "Label:" ) ),
            Qt::escape( medium->label().isEmpty() ? labelTextNone : medium->label() ) );
    table += row.arg( Qt::escape( i18n( "User Label:" ) ),
            Qt::escape( medium->userLabel().isEmpty() ? labelTextNone : medium->userLabel() ) );
    table += row.arg( Qt::escape( i18n( "Device Node:" ) ),
            Qt::escape( medium->deviceNode().isEmpty() ? labelTextNone : medium->deviceNode() ) );
    table += row.arg( Qt::escape( i18n( "Mount Point:" ) ),
            Qt::escape( medium->mountPoint().isEmpty() ? labelTextNone : medium->mountPoint() ) );
    table += row.arg( Qt::escape( i18n( "Mime Type:" ) ),
            Qt::escape( medium->mimeType().isEmpty() ? labelTextNone : medium->mimeType() ) );

    QString title = Qt::escape( i18n( "Device information for %1").arg(medium->name() ) );
    QString details = QString( "<em>%1</em><br />" "<table>%2</table>" ).arg( title, table );

    (void)new QLabel( i18n("Name: "), this );
    (void)new QLabel( medium->name(), this );
    (void)new K3ActiveLabel( i18n( "(<a href='whatsthis:%1'>Details</a>)" )
                            .arg( Amarok::escapeHTMLAttr( details ) ), this );

    (void)new QLabel( i18n("Plugin:"), this );
    m_pluginCombo = new KComboBox( false, this );
    m_pluginCombo->addItem( i18n( "Do not handle" ) );

    for( KService::List::ConstIterator it = MediaBrowser::instance()->getPlugins().begin();
            it != MediaBrowser::instance()->getPlugins().end();
            ++it ){
        m_pluginCombo->addItem( (*it)->name() );
        if ( (*it)->property( "X-KDE-Amarok-name" ).toString() == config->readEntry( medium->id(), QString() ) )
            m_pluginCombo->setCurrentItem( (*it)->name() );
    }

    m_configButton = new KPushButton( KIcon(KIcon( Amarok::icon( "configure" ) )), QString::null, this );
    connect( m_configButton, SIGNAL(clicked()), SLOT(configureDevice()) );
    m_configButton->setEnabled( !m_new && m_pluginCombo->currentText() != i18n( "Do not handle" ) );
    m_configButton->setToolTip( i18n( "Configure device settings" ) );

    m_removeButton = new KPushButton( i18n( "Remove" ), this );
    connect( m_removeButton, SIGNAL(clicked()), SLOT(deleteDevice()) );
    m_removeButton->setToolTip( i18n( "Remove entries corresponding to this device from configuration file" ) );

    connect( m_pluginCombo, SIGNAL(activated(const QString&)), m_manager, SLOT(slotChanged()) );
    connect( this, SIGNAL(changed()), m_manager, SLOT(slotChanged()) );

    if( !nographics )
        show();
}

MediaDeviceConfig::~MediaDeviceConfig()
{
}

bool
MediaDeviceConfig::isNew()
{
    return m_new;
}

Medium *
MediaDeviceConfig::medium()
{
    return m_medium;
}

QString
MediaDeviceConfig::plugin()
{
    return MediaBrowser::instance()->getInternalPluginName( m_pluginCombo->currentText() );
}

QString
MediaDeviceConfig::oldPlugin()
{
    return m_oldPlugin;
}

void
MediaDeviceConfig::setOldPlugin( const QString &oldPlugin )
{
    m_oldPlugin = oldPlugin;
}

QAbstractButton *
MediaDeviceConfig::configButton()
{
    return m_configButton;
}

QAbstractButton *
MediaDeviceConfig::removeButton()
{
    return m_removeButton;
}

KComboBox *
MediaDeviceConfig::pluginCombo()
{
    return m_pluginCombo;
}

void
MediaDeviceConfig::configureDevice() //slot
{
    DeviceConfigureDialog* dcd = new DeviceConfigureDialog( m_medium );
    dcd->exec();
    delete dcd;
}

void
MediaDeviceConfig::deleteDevice() //slot
{
    //TODO:save something in amarokrc such that it's not shown again until hit autoscan
    //m_deletedMap[medium->id()] = medium;
    emit deleteMedium( medium() );
    delete this;
}

#include "mediumpluginmanager.moc"
