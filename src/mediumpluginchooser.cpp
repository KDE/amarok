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

MediumPluginChooser::MediumPluginChooser( const Medium *medium, const KGuiItem ignoreButton )
        : KDialogBase( amaroK::mainWindow(), "mediumpluginchooserdialog", true, QString("Select Plugin for " + medium->name()), Ok|Cancel, Ok, false, ignoreButton )
{
    DEBUG_BLOCK
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Removable Medium Plugin Chooser" ) ) );

    KWin::setState( winId(), NET::SkipTaskbar );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    // BEGIN Chooser
    QHBox* chooserBox = new QHBox( vbox );
    chooserBox->setSpacing( KDialog::spacingHint() );

    QString labelTextFirstLine = i18n( "amaroK has detected what appears to be a removable music player." );
    QString labelTextSecondLineA = i18n( "\nThis player is known to the system as: " );
    QString labelTextSecondLineB = i18n( ", and its label (if any) is: " );
    QString labelTextThirdLine = i18n( "\nIts mount point (if any) is: " );
    QString labelTextFourthLine = i18n( "\nPlease choose a plugin to handle it, or press \"Ignore\" to be prompted again next time." );
    QString labelTextFifthLine = i18n( "\nIf you do not want amaroK to handle this media, choose \"Do not handle\"." )
    QString labelTextNone = i18n( "(none)" );

    QString fullLabel( labelTextFirstLine + labelTextSecondLineA + medium->name() + labelTextSecondLineB + (medium->label().isEmpty() ? labelTextNone : medium->label()) + labelTextThirdLine + (medium->mountPoint().isEmpty() ? labelTextNone : medium->mountPoint()) + labelTextFourthLine + labelTextFifthLine );

    new QLabel( fullLabel, chooserBox );

    QHBox* chooser2Box = new QHBox( vbox );
    chooser2Box->setSpacing( KDialog::spacingHint() );

    m_chooserCombo = new KComboBox( chooser2Box );
    m_chooserCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

}

MediumPluginChooser::~MediumPluginChooser()
{
    DEBUG_BLOCK
}


#include "mediumpluginchooser.moc"
