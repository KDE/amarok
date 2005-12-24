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
    setCaption( kapp->makeStdCaption( i18n( "Plugin Chooser" ) ) );

    KWin::setState( winId(), NET::SkipTaskbar );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    // BEGIN Chooser
    QHBox* chooserBox = new QHBox( vbox );
    chooserBox->setSpacing( KDialog::spacingHint() );

    new QLabel( i18n("Choose appropriate plugin:"), chooserBox );

    m_chooserCombo = new KComboBox( chooserBox );
    m_chooserCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

}

MediumPluginChooser::~MediumPluginChooser()
{
    DEBUG_BLOCK
}


#include "mediumpluginchooser.moc"
