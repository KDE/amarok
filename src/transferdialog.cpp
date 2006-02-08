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
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "transferdialog.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

TransferDialog::TransferDialog( MediaDevice * mdev )
        : KDialogBase( amaroK::mainWindow(), "transferdialog", true, QString::null, Ok|Cancel, Ok )
{
    m_accepted = false;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Transfer Queue to Device" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    QString transferDir = mdev->getTransferDir();

    new QLabel( i18n( "Enter the directory to use as the transfer operation's root directory.\n"
                                       "%1 will be prepended automatically.\n"
                                       "If the directory does not exist, it will be created.\n" )
                    .arg( transferDir ), vbox );

    KLineEdit *directoryBox = NULL;
    directoryBox = new KLineEdit( QString::null, vbox, "transferdir_lineedit" );

    KComboBox *sort1, *sort2, *sort3 = NULL;

    new QLabel( i18n( "You can have your music automatically grouped in a variety of ways.\n"
                      "Each grouping will create directories based upon the specified criteria.\n" ), vbox );

    new QLabel( i18n( "Select first grouping:\n" ), vbox );
    sort1 = new KComboBox( vbox );
    new QLabel( i18n( "Select second grouping:\n" ), vbox );
    sort2 = new KComboBox( vbox );
    new QLabel( i18n( "Select third grouping:\n" ), vbox ); 
    sort3 = new KComboBox( vbox );

    /*
    MediumMap mmap = DeviceManager::instance()->getMediumMap();
    MediumMap::Iterator it;

    m_sigmap = new QSignalMapper( this );

    QHBox* hbox;
    QString* currtext;
    QLabel* currlabel;
    KComboBox* currcombo;
    KPushButton* currbutton;
    int buttonnum = 0;

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    KTrader::OfferList::ConstIterator plugit;

    KConfig *config = amaroK::config( "MediaBrowser" );

    for ( it = mmap.begin(); it != mmap.end(); it++ )
    {
        hbox = new QHBox( vbox );

        if ( config->readEntry( (*it)->id() ).isEmpty() )
            new QLabel( i18n("  (NEW!)  Device Name: "), hbox );
        else
            new QLabel( i18n("          Device Name: "), hbox );

        currtext = new QString( (*it)->name() );
        currlabel = new QLabel( *currtext, hbox );

        currbutton = new KPushButton( i18n("(Detail)"), hbox );
        m_bmap[buttonnum] = (*it);
        m_sigmap->setMapping( currbutton, buttonnum );
        buttonnum++;
        connect(currbutton, SIGNAL( clicked() ), m_sigmap, SLOT( map() ) );

        new QLabel( i18n(", Plugin Selected:  "), hbox );
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

    if ( buttonnum == 0 ) {
        new QLabel( i18n( "You do not have any devices that can be managed by amaroK."), vbox );
        showButtonCancel( false );
    }

    connect( m_sigmap, SIGNAL( mapped( int ) ), this, SLOT( infoRequested ( int ) ) );
    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );
    */

}

void
TransferDialog::slotOk()
{
    m_accepted = true;
    KDialogBase::slotOk();
}

void
TransferDialog::slotCancel()
{
    m_accepted = false;
    KDialogBase::slotCancel();
}


#include "transferdialog.moc"
