/*THIS IS NOT BUILT YET BECAUSE IT's PRETTY MUCH ONLY AN IDEA AT THIS POINT*/


//
// C++ Implementation: mediumpluginchooser
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

#include <qlabel.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kwin.h>

typedef QMap<Medium*, KComboBox*> ComboMap;

MediumPluginManager::MediumPluginManager( )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginmanagerdialog", true, QString("Manage Device Plugins"), Ok|Cancel, Ok, false )
{
    DEBUG_BLOCK

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Manage Device Plugins" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    MediumMap mmap = DeviceManager::instance()->getMediumMap();
    MediumMap::Iterator it;

    QHBox* hbox;
    QString* currtext;
    QLabel* currlabel;
    KComboBox* currcombo;

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    KTrader::OfferList::ConstIterator plugit;

    KConfig *config = amaroK::config( "MediaBrowser" );

    for ( it = mmap.begin(); it != mmap.end(); it++ )
    {
        hbox = new QHBox( vbox );
        new QLabel( i18n("  Device Name:  "), hbox );
        currtext = new QString( (*it)->name() );
        currlabel = new QLabel( *currtext, hbox );
        new QLabel( i18n("  Plugin Selected:  "), hbox );

        currcombo = new KComboBox( false, hbox, currtext->latin1() );
        currcombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );
        currcombo->insertItem( i18n( "Do not handle" ) );
        for( plugit = offers.begin(); plugit != end; ++plugit ){
            currcombo->insertItem( (*plugit)->name() );
            if ( (*plugit)->property( "X-KDE-amaroK-name" ).toString() == config->readEntry( (*it)->id() ) )
                currcombo->setCurrentItem( (*plugit)->name() );
        }

        m_cmap[(*it)] = currcombo;

    }

    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );

    this->exec();
}

MediumPluginManager::~MediumPluginManager()
{
    DEBUG_BLOCK
}

void
MediumPluginManager::slotCancel( )
{
    DEBUG_BLOCK
    KDialogBase::slotCancel( );
}

void
MediumPluginManager::slotOk( )
{
    DEBUG_BLOCK
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

#include "mediumpluginmanager.moc"
