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
#include "mediabrowser.h"
#include "mediumpluginchooser.h"
#include "medium.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qlabel.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kwin.h>

MediumPluginManager::MediumPluginManager( )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginmanagerdialog", true, QString("Manage Device Plugins"), Ok|Cancel, Ok, false )
{
    DEBUG_BLOCK

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Manage Device Plugins" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    // BEGIN Manager
    QHBox* chooserBox = new QHBox( vbox );
    chooserBox->setSpacing( KDialog::spacingHint() );

    m_table = new KTable( , "chooserCombo" );
    m_table->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

}

MediumPluginManager::~MediumPluginManager()
{
    DEBUG_BLOCK
}

void
MediumPluginManager::slotCancel( )
{
    DEBUG_BLOCK
    const QString empty;
    emit selectedPlugin( m_medium, empty );
    KDialogBase::slotCancel( );
}

void
MediumPluginManager::slotOk( )
{
    DEBUG_BLOCK
    emit selectedPlugin( m_medium, QString(m_chooserCombo->currentText()) );
    KDialogBase::slotOk( );
}

#include "mediumpluginchooser.moc"
