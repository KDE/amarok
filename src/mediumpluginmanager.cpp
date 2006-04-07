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
#include "devicemanager.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mediumpluginchooser.h"
#include "mediumpluginmanager.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qsignalmapper.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

typedef QMap<QString, Medium*> MediumMap;

MediumPluginManager::MediumPluginManager()
        : KDialogBase( amaroK::mainWindow(), "mediumpluginmanagerdialog", false, QString::null, Ok|Cancel, Ok )
{
    //TODO: make this a member function, so that hboxes can be rebuilt if user selects to rescan
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Manage Device Plugins" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    m_siginfomap = new QSignalMapper( this );
    m_sigdelmap = new QSignalMapper( this );
    m_buttonnum = 0;
    m_redetect = false;

    m_offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    m_offersEnd = m_offers.end();

    m_config = amaroK::config( "MediaBrowser" );

    m_location = new QGroupBox( 1, Qt::Vertical, i18n( "Devices" ), vbox );
    m_location->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    m_devicesBox = new QVBox( m_location );
    m_devicesBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

    detectDevices();
    //Obsolete with manual addition of devices?
    //if ( buttonnum == 0 ) {
    //    new QLabel( i18n( "You do not have any devices that can be managed by amaroK."), vbox );
    //    showButtonCancel( false );
    //}

    m_hbox = new QHBox( vbox );

    KPushButton *detectDevices = new KPushButton( i18n( "Autodetect Devices" ), m_hbox);
    KPushButton *addButton = new KPushButton( i18n( "Add Device..." ), m_hbox );
    connect( detectDevices, SIGNAL( clicked() ), this, SLOT( reDetectDevices() ) );
    connect( addButton, SIGNAL( clicked() ), this, SLOT( newDevice() ) );

    connect( m_siginfomap, SIGNAL( mapped( int ) ), this, SLOT( infoRequested ( int ) ) );
    connect( m_sigdelmap, SIGNAL( mapped( int ) ), this, SLOT( deleteMedium( int ) ) );
    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );

    exec();
}

MediumPluginManager::~MediumPluginManager()
{
}

void
MediumPluginManager::detectDevices()
{
    MediumMap mmap = DeviceManager::instance()->getMediumMap();
    MediumMap::Iterator it;

    int skipflag;

    for ( it = mmap.begin(); it != mmap.end(); it++ )
    {
        if( !m_config->readEntry( (*it)->id() ).isEmpty() && m_config->readEntry( (*it)->id() ) == "deleted" && !m_redetect)
            continue;

        skipflag = 0;

        if( !m_bmap.empty() )
        {
            ButtonMap::Iterator bit;
            for( bit = m_bmap.begin(); bit != m_bmap.end(); ++bit )
            {
                if( (*it)->id() == (*bit)->id() )
                    skipflag = 1;
            }
        }

        if( skipflag == 1 )
            continue;

        if( m_deletedList.contains( (*it)->id() ) )
            m_deletedList.remove( (*it)->id() );

        m_hbox = new QHBox( m_devicesBox );
        m_hbox->show();
        m_hmap[m_buttonnum] = m_hbox;

        //debug() << "[MediumPluginManager] (*it)->id() = " << (*it)->id() << ", config->readEntry = " << m_config->readEntry( (*it)->id() ) << endl;

        if ( m_config->readEntry( (*it)->id() ).isEmpty() )
            m_currlabel = new QLabel( i18n("  (NEW!)  Device Name: "), m_hbox );
        else
            m_currlabel = new QLabel( i18n("          Device Name: "), m_hbox );

        m_currlabel->show();

        m_currtext = new QString( (*it)->name() );

        m_currlabel = new QLabel( *m_currtext, m_hbox );
        m_currlabel->show();

        m_currbutton = new KPushButton( i18n("(Detail)"), m_hbox );
        m_currbutton->show();
        m_bmap[m_buttonnum] = (*it);
        m_siginfomap->setMapping( m_currbutton, m_buttonnum );
        connect( m_currbutton, SIGNAL( clicked() ), m_siginfomap, SLOT( map() ) );

        m_currlabel = new QLabel( i18n(", Plugin Selected:  "), m_hbox );
        m_currlabel->show();
        m_currcombo = new KComboBox( false, m_hbox, m_currtext->latin1() );
        m_currcombo->show();
        m_currcombo->insertItem( i18n( "Do not handle" ) );
        m_dmap[("Do not handle")] = "ignore";

        for( m_plugit = m_offers.begin(); m_plugit != m_offersEnd; ++m_plugit ){
            m_dmap[(*m_plugit)->name()] = (*m_plugit)->property( "X-KDE-amaroK-name" ).toString();
            m_currcombo->insertItem( (*m_plugit)->name() );
            if ( (*m_plugit)->property( "X-KDE-amaroK-name" ).toString() == m_config->readEntry( (*it)->id() ) )
                m_currcombo->setCurrentItem( (*m_plugit)->name() );
        }

        m_currlabel = new QLabel( "          ", m_hbox);
        m_currlabel->show();
        m_deletebutton = new KPushButton( i18n( "Remove" ) , m_hbox );
        m_deletebutton->show();
        m_sigdelmap->setMapping( m_deletebutton, m_buttonnum );
        connect( m_deletebutton, SIGNAL( clicked() ), m_sigdelmap, SLOT( map() ) );

        m_cmap[(*it)] = m_currcombo;

        m_buttonnum++;
    }
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
        if( it.data()->currentText() == i18n( "Do not handle" ) )
        {
            emit selectedPlugin( it.key(), QString( "ignore" ) );
        }
        else
        {
            emit selectedPlugin( it.key(), MediaBrowser::instance()->getPluginName( it.data()->currentText() ) );
        }
    }
    KConfig *config = amaroK::config( "MediaBrowser" );
    DeletedList::Iterator dit;
    for( dit = m_deletedList.begin(); dit != m_deletedList.end(); ++dit )
    {
        if( (*dit)->isAutodetected() )
            config->writeEntry( (*dit)->id(), "deleted" );
        else
            config->deleteEntry( (*dit)->id() );
    }
    KDialogBase::slotOk( );
}

void
MediumPluginManager::infoRequested( int buttonId )
{
    Medium* medium = m_bmap[buttonId];
    MediumPluginDetailView* mpdv = new MediumPluginDetailView( medium );
    mpdv->exec();
}

void
MediumPluginManager::deleteMedium( int buttonId )
{
    //TODO:save something in amarokrc such that it's not shown again until hit autoscan
    QHBox *box = m_hmap[buttonId];
    m_hmap.remove(buttonId);
    delete box;
    Medium *medium = m_bmap[buttonId];
    m_bmap.remove(buttonId);
    m_deletedList[medium->id()] = medium;
    //TODO: maybe don't remove, but mark somehow, so that they have to hit OK to remember deleting it?
    m_cmap.remove(medium);
}

void
MediumPluginManager::newDevice()
{
    ManualDeviceAdder* mda = new ManualDeviceAdder( this );
    mda->exec();
    if( mda->successful() && mda->getMedium() != 0 )
    {
        if( m_config->readEntry( mda->getMedium()->id() ) != QString::null )
        {
            //abort!  Can't have the same device defined twice...should never
            //happen due to name checking earlier...right?
            amaroK::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, you cannot define two devices\n"
                                                                       "with the same name and mountpoint!") );
        }
        else
        {
            DeviceManager::instance()->addManualDevice( mda->getMedium() );
            m_config->writeEntry( mda->getMedium()->id(), mda->getPlugin() );
            detectDevices();
        }
    }
    delete mda;
}

/////////////////////////////////////////////////////////////////////

MediumPluginDetailView::MediumPluginDetailView( const Medium* medium )
: KDialogBase( amaroK::mainWindow(), "mediumplugindetailview", true, QString::null, Ok, Ok )
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Device information for %1").arg(medium->name() ) ) );

    QHBox* hbox = makeHBoxMainWidget();
    hbox->setSpacing( KDialog::spacingHint() );

    QVBox* vbox1 = new QVBox( hbox );
    QVBox* vbox2 = new QVBox( hbox );

    const QString labelTextNone = i18n( "(none)" );

    new QLabel( i18n( "Autodetected:"), vbox1 );
    new QLabel( medium->isAutodetected() ? i18n("Yes") : i18n("No"), vbox2 );
    new QLabel( i18n( "ID:"), vbox1 );
    new QLabel( medium->id(), vbox2 );
    new QLabel( i18n( "Name:"), vbox1 );
    new QLabel( medium->name(), vbox2 );
    new QLabel( i18n( "Label:"), vbox1 );
    new QLabel( medium->label().isEmpty() ? labelTextNone : medium->label(), vbox2 );
    new QLabel( i18n( "User Label:"), vbox1 );
    new QLabel( medium->userLabel().isEmpty() ? labelTextNone : medium->userLabel(), vbox2 );
    new QLabel( i18n( "Device Node:"), vbox1 );
    new QLabel( medium->deviceNode().isEmpty() ? labelTextNone : medium->deviceNode(), vbox2 );
    new QLabel( i18n( "Mount Point:"), vbox1 );
    new QLabel( medium->mountPoint().isEmpty() ? labelTextNone : medium->mountPoint(), vbox2 );
    new QLabel( i18n( "Mime Type:"), vbox1 );
    new QLabel( medium->mimeType().isEmpty() ? labelTextNone : medium->mimeType(), vbox2 );
}

MediumPluginDetailView::~MediumPluginDetailView()
{
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
    new QLabel( i18n( "Enter a name for this device (required):" ), vbox1 );
    m_mdaName = new KLineEdit( vbox1, "m_mdaname" );

    new QLabel( "", vbox1 );
    new QLabel( i18n( "Enter the device's mountpoint, if applicable:"), vbox1 );
    m_mdaMountPoint = new KLineEdit( vbox1, "m_mdamountpoint" );
    KCompletion *comp = m_mdaMountPoint->completionObject();
    connect( m_mdaMountPoint, SIGNAL( returnPressed(const QString&) ), comp, SLOT( addItem(const QString&) ) );
    connect( m_mdaCombo, SIGNAL( activated(const QString&) ), this, SLOT( comboChanged(const QString&) ) );
}

ManualDeviceAdder::~ManualDeviceAdder()
{
}

void
ManualDeviceAdder::slotCancel()
{
    KDialogBase::slotCancel( );
}

void
ManualDeviceAdder::slotOk()
{
    if( DeviceManager::instance()->getDevice( getMedium()->name() ) == NULL )
    {
        m_successful = true;
        KDialogBase::slotOk( );
    }
    else
    {
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n("Sorry, you cannot define two devices\n"
                                                                   "with the same name. These names must be\n"
                                                                   "unique across autodetected devices as well.\n") );
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
}

Medium*
ManualDeviceAdder::getMedium()
{
    if( m_mdaMountPoint->isEnabled() == false &&
            m_mdaName->text() == QString::null )
        return 0;
    if( m_mdaMountPoint->text() == QString::null &&
            m_mdaName->text() == QString::null )
        return 0;
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
