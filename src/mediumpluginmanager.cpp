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

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kpushbutton.h>

typedef QMap<QString, Medium*> MediumMap;

MediumPluginManager::MediumPluginManager( )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginmanagerdialog", true, QString::null, Ok|Cancel, Ok )
{
    //TODO: make this a member function, so that hboxes can be rebuilt if user selects to rescan
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Manage Device Plugins" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    MediumMap mmap = DeviceManager::instance()->getMediumMap();
    MediumMap::Iterator it;

    m_siginfomap = new QSignalMapper( this );
    m_sigdelmap = new QSignalMapper( this );

    QHBox* hbox;
    QString* currtext;
    QLabel* currlabel;
    KComboBox* currcombo;
    KPushButton* currbutton;
    KPushButton* deletebutton;
    int buttonnum = 0;

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    KTrader::OfferList::ConstIterator plugit;

    KConfig *config = amaroK::config( "MediaBrowser" );

    QGroupBox *location = new QGroupBox( 1, Qt::Vertical, i18n( "Devices" ), vbox );
    QVBox* devicesBox = new QVBox( location );
    QSpacerItem *spacer;
    QLayout *hlayout;

    for ( it = mmap.begin(); it != mmap.end(); it++ )
    {
        hbox = new QHBox( devicesBox );
        m_hmap[buttonnum] = hbox;

        if ( config->readEntry( (*it)->id() ).isEmpty() )
            new QLabel( i18n("  (NEW!)  Device Name: "), hbox );
        else
            new QLabel( i18n("          Device Name: "), hbox );

        currtext = new QString( (*it)->name() );
        currlabel = new QLabel( *currtext, hbox );

        currbutton = new KPushButton( i18n("(Detail)"), hbox );
        m_bmap[buttonnum] = (*it);
        m_siginfomap->setMapping( currbutton, buttonnum );
        connect(currbutton, SIGNAL( clicked() ), m_siginfomap, SLOT( map() ) );

        new QLabel( i18n(", Plugin Selected:  "), hbox );
        currcombo = new KComboBox( false, hbox, currtext->latin1() );
        currcombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
        currcombo->insertItem( i18n( "Do not handle" ) );
        for( plugit = offers.begin(); plugit != end; ++plugit ){
            currcombo->insertItem( (*plugit)->name() );
            if ( (*plugit)->property( "X-KDE-amaroK-name" ).toString() == config->readEntry( (*it)->id() ) )
                currcombo->setCurrentItem( (*plugit)->name() );
        }

        //spacer = new QSpacerItem( 100, 0, QSizePolicy::Fixed, QSizePolicy::Fixed );
        //hlayout = hbox->layout();
        //if( hlayout )
        //    hlayout->addItem( spacer );

        new QLabel( "          ", hbox);
        deletebutton = new KPushButton( i18n( "Remove" ) , hbox );
        m_sigdelmap->setMapping( deletebutton, buttonnum );
        connect( deletebutton, SIGNAL( clicked() ), m_sigdelmap, SLOT( map() ) );

        m_cmap[(*it)] = currcombo;

        buttonnum++;
    }

    //Obsolete with manual addition of devices?
    //if ( buttonnum == 0 ) {
    //    new QLabel( i18n( "You do not have any devices that can be managed by amaroK."), vbox );
    //    showButtonCancel( false );
    //}

    hbox = new QHBox( vbox );

    //KPushButton *addButton = new KPushButton( i18n( "Add Device..." ), hbox );

    connect( m_siginfomap, SIGNAL( mapped( int ) ), this, SLOT( infoRequested ( int ) ) );
    connect( m_sigdelmap, SIGNAL( mapped( int ) ), this, SLOT( deleteMedium( int ) ) );
    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );

    exec();
}

void
MediumPluginManager::slotOk( )
{
    ComboMap::Iterator it;
    for ( it = m_cmap.begin(); it != m_cmap.end(); ++it )
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
    delete m_hmap[buttonId];
    Medium *tmp = m_bmap[buttonId];
    //TODO: maybe don't remove, but mark somehow, so that they have to hit OK to remember deleting it?
    m_cmap.remove(tmp);
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
    new QLabel( medium->autodetected(), vbox2 );
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


#include "mediumpluginmanager.moc"
