//
// C++ Implementation: mediumpluginmanager
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "amarok.h"
#include "debug.h"
#include "deviceconfiguredialog.h"
#include "devicemanager.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginmanager.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qsignalmapper.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kactivelabel.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

using amaroK::escapeHTML;
using amaroK::escapeHTMLAttr;

typedef QMap<QString, Medium*> MediumMap;

MediumPluginManager::MediumPluginManager()
        : KDialogBase( amaroK::mainWindow(), "mediumpluginmanagerdialog", false, QString::null, Ok|Cancel, Ok )
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Manage Devices and Plugins" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );
    vbox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_sigdelmap = new QSignalMapper( this );
    m_sigconfmap = new QSignalMapper( this );
    m_buttonnum = 0;
    m_redetect = false;

    m_offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    m_offersEnd = m_offers.end();

    m_location = new QGroupBox( 1, Qt::Vertical, i18n( "Devices" ), vbox );
    m_location->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    m_devicesBox = new QVBox( m_location );
    m_devicesBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    detectDevices();

    m_hbox = new QHBox( vbox );

    KPushButton *detectDevices = new KPushButton( i18n( "Autodetect Devices" ), m_hbox);
    detectDevices->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    KPushButton *addButton = new KPushButton( i18n( "Add Device..." ), m_hbox );
    addButton->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( detectDevices, SIGNAL( clicked() ), this, SLOT( reDetectDevices() ) );
    connect( addButton, SIGNAL( clicked() ), this, SLOT( newDevice() ) );

    connect( m_sigdelmap, SIGNAL( mapped( int ) ), this, SLOT( deleteDevice( int ) ) );
    connect( m_sigconfmap, SIGNAL( mapped( int ) ), this, SLOT( configureDevice( int ) ) );
    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );

}

MediumPluginManager::~MediumPluginManager()
{
}

void
MediumPluginManager::detectDevices()
{
    KConfig *config = amaroK::config( "MediaBrowser" );

    MediumMap mmap = DeviceManager::instance()->getMediumMap();
    for ( MediumMap::Iterator it = mmap.begin(); it != mmap.end(); it++ )
    {
        if( !config->readEntry( (*it)->id() ).isEmpty() &&
                config->readEntry( (*it)->id() ) == "deleted" && !m_redetect)
        {
            debug() << "skipping: deleted" << endl;
            continue;
        }

        bool skipflag = false;

        if( !m_bmap.empty() )
        {
            ButtonMap::Iterator bit;
            for( bit = m_bmap.begin(); bit != m_bmap.end(); ++bit )
            {
                if( (*it)->id() == (*bit)->id() )
                {
                    skipflag = true;
                    debug() << "skipping: already listed" << endl;
                }
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

        renderDevice( m_devicesBox, *it );

    }
}

void
MediumPluginManager::renderDevice( QWidget *parent, Medium *medium )
{
    KConfig *config = amaroK::config( "MediaBrowser" );
    QHBox *hbox = new QHBox( parent );
    hbox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    hbox->setSpacing( 5 );
    m_hmap[m_buttonnum] = hbox;

    //debug() << "[MediumPluginManager] medium->id() = " << medium->id() << ", config->readEntry = " << config->readEntry( medium->id() ) << endl;

    const QString labelTextNone = i18n( "(none)" );
    QString row = "<tr><td>%1</td><td>%2</td></tr>";
    QString table;
    table += row.arg( escapeHTML( i18n( "Autodetected:" ) ),
            escapeHTML( medium->isAutodetected() ? i18n("Yes") : i18n("No") ) );
    table += row.arg( escapeHTML( i18n( "ID:" ) ),
            escapeHTML( medium->id() ) );
    table += row.arg( escapeHTML( i18n( "Name:" ) ),
            escapeHTML( medium->name() ) );
    table += row.arg( escapeHTML( i18n( "Label:" ) ),
            escapeHTML( medium->label().isEmpty() ? labelTextNone : medium->label() ) );
    table += row.arg( escapeHTML( i18n( "User Label:" ) ),
            escapeHTML( medium->userLabel().isEmpty() ? labelTextNone : medium->userLabel() ) );
    table += row.arg( escapeHTML( i18n( "Device Node:" ) ),
            escapeHTML( medium->deviceNode().isEmpty() ? labelTextNone : medium->deviceNode() ) );
    table += row.arg( escapeHTML( i18n( "Mount Point:" ) ),
            escapeHTML( medium->mountPoint().isEmpty() ? labelTextNone : medium->mountPoint() ) );
    table += row.arg( escapeHTML( i18n( "Mime Type:" ) ),
            escapeHTML( medium->mimeType().isEmpty() ? labelTextNone : medium->mimeType() ) );

    QString title = escapeHTML( i18n( "Device information for %1").arg(medium->name() ) );
    QString details = QString( "<em>%1</em><br />" "<table>%2</table>" ).arg( title, table );

    QLabel *label = new QLabel( hbox );
    label->setFixedWidth( IconSize( KIcon::Small ) + 10 );
    if ( config->readEntry( medium->id() ).isEmpty() )
        label->setPixmap( kapp->iconLoader()->loadIcon( amaroK::icon( "new" ), KIcon::Toolbar, KIcon::SizeSmall ) );
    (void)new QLabel( i18n("Device Name: "), hbox );
    (void)new QLabel( medium->name(), hbox );
    (void)new KActiveLabel( i18n( "(<a href='whatsthis:%1'>Details</a>)" )
                            .arg( amaroK::escapeHTMLAttr( details ) ), hbox );

    (void)new QLabel( i18n("Plugin:"), hbox );
    KComboBox *combo = new KComboBox( false, hbox );
    combo->insertItem( i18n( "Do not handle" ) );
    m_dmap[("Do not handle")] = "ignore";

    for( m_plugit = m_offers.begin(); m_plugit != m_offersEnd; ++m_plugit ){
        m_dmap[(*m_plugit)->name()] = (*m_plugit)->property( "X-KDE-amaroK-name" ).toString();
        combo->insertItem( (*m_plugit)->name() );
        if ( (*m_plugit)->property( "X-KDE-amaroK-name" ).toString() == config->readEntry( medium->id() ) )
            combo->setCurrentItem( (*m_plugit)->name() );
    }

    KPushButton *button = 0;

    button = new KPushButton( SmallIconSet( amaroK::icon( "configure" ) ), QString::null, hbox );
    if( m_newDevMap.contains( medium->id() ) )
        button->setEnabled( false );
    m_sigconfmap->setMapping( button, m_buttonnum );
    connect( button, SIGNAL( clicked() ), m_sigconfmap, SLOT( map() ) );
    //button->setToolTip( "Configure device settings" );

    button = new KPushButton( i18n( "Remove" ), hbox );
    m_sigdelmap->setMapping( button, m_buttonnum );
    connect( button, SIGNAL( clicked() ), m_sigdelmap, SLOT( map() ) );
    //button->setToolTip( "Remove entries corresponding to this device from configuration file" );

    m_bmap[m_buttonnum] = medium;
    m_cmap[medium] = combo;
    m_omap[medium] = config->readEntry( medium->id() );

    hbox->show();

    m_buttonnum++;
}

void
MediumPluginManager::reDetectDevices()
{
    m_redetect = true;
    uint currsize = m_bmap.count();
    detectDevices();
    if( currsize == m_bmap.count() )
    {
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n("No new media devices were found. If you feel this is an\n"
                                                                   "error, ensure that the DBUS and HAL daemons are running\n"
                                                                   "and KDE was built with support for them. You can test this\n"
                                                                   "by running\n"
                                                                   "     \"dcop kded mediamanager fullList\"\n"
                                                                   "in a Konsole window.") );
    }
    m_redetect = false;
}

void
MediumPluginManager::slotOk( )
{
    ComboMap::Iterator it;
    for( it = m_cmap.begin(); it != m_cmap.end(); ++it )
    {
        QString plugin( "ignore" );
        if( it.data()->currentText() != i18n( "Do not handle" ) )
        {
            plugin = MediaBrowser::instance()->getPluginName( it.data()->currentText() );
        }

        if( plugin != m_omap[it.key()] )
            emit selectedPlugin( it.key(), plugin );
    }
    KConfig *config = amaroK::config( "MediaBrowser" );
    DeletedMap::Iterator dit;
    for( dit = m_deletedMap.begin(); dit != m_deletedMap.end(); ++dit )
    {
        if( dit.data()->isAutodetected() )
            config->writeEntry( dit.data()->id(), "deleted" );
        else
            config->deleteEntry( dit.data()->id() );
        DeviceManager::instance()->removeManualDevice( dit.data() );
    }
    KDialogBase::slotOk( );
}

void
MediumPluginManager::configureDevice( int buttonId )
{
    Medium* medium = m_bmap[buttonId];
    DeviceConfigureDialog* dcd = new DeviceConfigureDialog( medium );
    dcd->exec();
    delete dcd;
}

void
MediumPluginManager::deleteDevice( int buttonId )
{
    //TODO:save something in amarokrc such that it's not shown again until hit autoscan
    QHBox *box = m_hmap[buttonId];
    m_hmap.remove(buttonId);
    delete box;
    Medium *medium = m_bmap[buttonId];
    m_bmap.remove(buttonId);
    m_deletedMap[medium->id()] = medium;
    //TODO: maybe don't remove, but mark somehow, so that they have to hit OK to remember deleting it?
    m_cmap.remove(medium);
    m_omap.remove(medium);
}

void
MediumPluginManager::newDevice()
{
    DEBUG_BLOCK
    ManualDeviceAdder* mda = new ManualDeviceAdder( this );
    mda->exec();
    if( mda->successful() && mda->getMedium() != 0 )
    {
        if( amaroK::config( "MediaBrowser" )->readEntry( mda->getMedium()->id() ) != QString::null )
        {
            //abort!  Can't have the same device defined twice...should never
            //happen due to name checking earlier...right?
            amaroK::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, you cannot define two devices\n"
                                                                       "with the same name and mountpoint!") );
        }
        else
        {
            Medium *newdev = mda->getMedium();
            amaroK::config( "MediaBrowser" )->writeEntry( newdev->id(), mda->getPlugin() );
            DeviceManager::instance()->addManualDevice( newdev );
            m_newDevMap[newdev->id()] = newdev;
            detectDevices();
        }
    }
    delete mda;
}

/////////////////////////////////////////////////////////////////////

ManualDeviceAdder::ManualDeviceAdder( MediumPluginManager* mpm )
: KDialogBase( amaroK::mainWindow(), "manualdeviceadder", true, QString::null, Ok, Ok )
{
    m_mpm = mpm;
    m_successful = false;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Add New Device") ) );

    QHBox* hbox = makeHBoxMainWidget();
    hbox->setSpacing( KDialog::spacingHint() );

    QVBox* vbox1 = new QVBox( hbox );

    m_mdaOffers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    m_mdaOffersEnd = m_mdaOffers.end();

    new QLabel( i18n( "Select the plugin to use with this device:"), vbox1 );
    m_mdaCombo = new KComboBox( false, vbox1, "m_mdacombo" );
    m_mdaCombo->insertItem( i18n( "Do not handle" ) );
    for( m_mdaPlugit = m_mdaOffers.begin(); m_mdaPlugit != m_mdaOffersEnd; ++m_mdaPlugit )
        m_mdaCombo->insertItem( (*m_mdaPlugit)->name() );

    new QLabel( "", vbox1 );
    QLabel* nameLabel = new QLabel( vbox1 );
    nameLabel->setText( i18n( "Enter a &name for this device (required):" ) );
    m_mdaName = new HintLineEdit( QString::null, vbox1);
    nameLabel->setBuddy( m_mdaName );
    m_mdaName->setHint( i18n( "Example: My_Ipod" ) );
    QToolTip::add( m_mdaName, i18n( "Enter a name for the device.  The name must be unique across all devices, including autodetected devices.  It must not contain the pipe ( | ) character." ) );

    new QLabel( "", vbox1 );
    QLabel* mpLabel = new QLabel( vbox1 );
    mpLabel->setText( i18n( "Enter the &mount point of the device, if applicable:" ) );
    m_mdaMountPoint = new HintLineEdit( QString::null, vbox1);
    mpLabel->setBuddy( m_mdaMountPoint );
    m_mdaMountPoint->setHint( i18n( "Example: /mnt/ipod" ) );
    QToolTip::add( m_mdaMountPoint, i18n( "Enter the device's mount point.  Some devices (such as iRiver iFP devices) may not have a mount point and this can be ignored.  All other devices (iPods, UMS/VFAT devices) should enter the mount point here." ) );

    connect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( comboChanged(const QString&) ) );
}

ManualDeviceAdder::~ManualDeviceAdder()
{
    delete m_mdaName;
    delete m_mdaMountPoint;
}

void
ManualDeviceAdder::slotCancel()
{
    KDialogBase::slotCancel( );
}

void
ManualDeviceAdder::slotOk()
{
    debug() << "medium: " << getMedium() << endl;
    if( getMedium() )
    {
       debug() << "name: " << getMedium()->name() << ", managed: " << DeviceManager::instance()->getDevice( getMedium()->name() ) << endl;
    }
    if( getMedium() != NULL && DeviceManager::instance()->getDevice( getMedium()->name() ) == NULL )
    {
        m_successful = true;
        KDialogBase::slotOk( );
    }
    else
    {
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, every device must have a name and\n"
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
    if( m_mpm->getPluginName( string ) == "ifp-mediadevice" )
    {
        m_comboOldText = m_mdaMountPoint->text();
        m_mdaMountPoint->setText( i18n("(not applicable)") );
        m_mdaMountPoint->setEnabled(false);
    }
    else if( m_mdaMountPoint->isEnabled() == false )
    {
        m_mdaMountPoint->setText( m_comboOldText );
        m_mdaMountPoint->setEnabled(true);
    }
    m_selectedPlugin = m_mpm->getPluginName( string );
    debug() << "selected plugin: " << m_selectedPlugin << endl;
}

Medium*
ManualDeviceAdder::getMedium()
{
    if( m_mdaMountPoint->isEnabled() == false &&
            m_mdaName->text() == QString::null )
        return NULL;
    if( m_mdaMountPoint->text() == QString::null &&
            m_mdaName->text() == QString::null )
        return NULL;
    QString id = "manual|" + m_mdaName->text() + '|' +
            ( m_mdaMountPoint->text() == QString::null ||
                m_mdaMountPoint->isEnabled() == false ?
                "(null)" : m_mdaMountPoint->text() );
    Medium* added = new Medium( id, m_mdaName->text() );
    added->setAutodetected( false );
    added->setMountPoint( m_mdaMountPoint->text() );
    return added;
}

#include "mediumpluginmanager.moc"
