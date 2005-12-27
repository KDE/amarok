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
#include "mediumpluginchooser.h"
#include "medium.h"

#include <qlabel.h>
#include <qvbox.h>

#include <kapplication.h>
#include <klocale.h>
#include <kwin.h>

MediumPluginChooser::MediumPluginChooser( const Medium *medium )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginchooserdialog", true, QString("Select Plugin for " + medium->name()), Ok|Cancel, Ok, false )
{
    DEBUG_BLOCK
    m_medium = medium;
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Removable Medium Plugin Chooser" ) ) );

    KWin::setState( winId(), NET::SkipTaskbar );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    // BEGIN Chooser
    QHBox* chooserBox = new QHBox( vbox );
    chooserBox->setSpacing( KDialog::spacingHint() );

    QString labelTextNone = i18n( "(none)" );

    QString fullLabel( i18n( "amaroK has detected what appears to be a removable music player.\n"
            "This player is known to the system as: %1, and its label (if any) is: %2\n"
            "Its mount point (if any) is: %3\n"
            "Please choose a plugin to handle it and press \"Ok\" or press \"Cancel\" to be prompted again next time.\n"
            "If you do not want amaroK to handle this device, choose \"Do not handle\"." )
                .arg(medium->name())
                .arg(medium->label().isEmpty() ? labelTextNone : medium->label())
                .arg(medium->mountPoint().isEmpty() ? labelTextNone : medium->mountPoint()));

    new QLabel( fullLabel, chooserBox );

    QHBox* chooser2Box = new QHBox( vbox );
    chooser2Box->setSpacing( KDialog::spacingHint() );

    m_chooserCombo = new KComboBox( false, chooser2Box, "chooserCombo" );
    m_chooserCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

    m_chooserCombo->insertItem( "Do not handle" );
}

MediumPluginChooser::~MediumPluginChooser()
{
    DEBUG_BLOCK
}

void
MediumPluginChooser::slotCancel( )
{
    DEBUG_BLOCK
    QString empty;
    emit selectedPlugin( m_medium, empty );
    KDialogBase::slotCancel( );
}

void
MediumPluginChooser::slotOk( )
{
    DEBUG_BLOCK
    emit selectedPlugin( m_medium, QString(m_chooserCombo->currentText()) );
    KDialogBase::slotOk( );
}

#include "mediumpluginchooser.moc"
