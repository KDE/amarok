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
#include "mediabrowser.h"
#include "mediumpluginchooser.h"
#include "mediumpluginmanager.h"
#include "medium.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qlabel.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kwin.h>

MediumPluginChooser::MediumPluginChooser( const Medium *medium )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginchooserdialog", true, QString("Select Plugin for " + medium->name()), Ok|Cancel, Ok, false )
{
    m_medium = medium;
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Removable Medium Plugin Chooser" ) ) );

    //KWin::setState( winId(), NET::SkipTaskbar );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    QString labelTextNone = i18n( "(none)" );

    QString firstLabel( i18n( "amaroK has detected what appears to be a removable\n"
                              "music player. The device appears to the system as %1.\n" )
                    .arg(medium->name()) );

    QString secondLabel( i18n( "\nPlease choose a plugin to handle it and press \"Ok\" or\n"
                               "press \"Cancel\" to be prompted again next time.\n\n"
                               "If you do not want amaroK to handle this device, choose \"Do not handle\"." ) );

    new QLabel( firstLabel, vbox );

    KPushButton* detail = new KPushButton( i18n( "Press here for details about this device." ), vbox );
    connect( detail, SIGNAL( clicked() ), this, SLOT( detailPushed() ) );

    new QLabel( secondLabel, vbox);

    QHBox* chooser2Box = new QHBox( vbox );
    chooser2Box->setSpacing( KDialog::spacingHint() );

    m_chooserCombo = new KComboBox( false, chooser2Box, "chooserCombo" );
    m_chooserCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

    m_chooserCombo->insertItem( i18n( "Do not handle" ) );

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'mediadevice'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != end; ++it )
        m_chooserCombo->insertItem( (*it)->name() );

    connect( this, SIGNAL( selectedPlugin( const Medium*, const QString ) ), MediaBrowser::instance(), SLOT( pluginSelected( const Medium*, const QString ) ) );
}

MediumPluginChooser::~MediumPluginChooser()
{
}

void
MediumPluginChooser::slotCancel()
{
    const QString empty;
    emit selectedPlugin( m_medium, empty );
    KDialogBase::slotCancel( );
}

void
MediumPluginChooser::slotOk()
{
    if( m_chooserCombo->currentText() == i18n( "Do not handle" ) )
    {
        emit selectedPlugin( m_medium, QString( "ignore" ) );
    }
    else
    {
        emit selectedPlugin( m_medium, MediaBrowser::instance()->m_pluginName[m_chooserCombo->currentText() ] );
    }
    KDialogBase::slotOk();
}

void
MediumPluginChooser::detailPushed()
{
    MediumPluginDetailView* mpdv = new MediumPluginDetailView( m_medium );
    mpdv->exec();
}
#include "mediumpluginchooser.moc"
