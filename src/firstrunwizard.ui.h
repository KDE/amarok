/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include "directorylist.h"
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <qpushbutton.h>

namespace amaroK
{
    extern QPixmap getPNG( const QString& );
    extern QPixmap getJPG( const QString& );

    extern KConfig *config( const QString& );
}
using namespace amaroK;

void
FirstRunWizard::init()
{
    CollectionSetup::s_dirs = config( "Collection Browser" )->readListEntry( "Folders" );

    cancelButton()->setFixedWidth( cancelButton()->width() );

    //option_xmms->setPixmap( getPNG( "wizard_xmms" ) );
    //option_compact->setPixmap( getPNG( "wizard_compact" ) );
    picture1->setPixmap( getJPG( "amarok_rocks" ) );
    picture4->setPixmap( *picture1->pixmap() );

    WizardPageLayout_3->addWidget( m_folderSetup = new CollectionSetup( WizardPage_3 ) );

    text4->disconnect( SIGNAL(linkClicked( const QString& )) );
    connect( text4, SIGNAL(linkClicked( const QString& )), SLOT(invokeHandbook()) );

    setFinishEnabled ( WizardPage_4, true );
}

void
FirstRunWizard::showPage( QWidget *w )
{
    QWizard::showPage( w );

    cancelButton()->setText( w == WizardPage ? i18n("&Skip") : i18n("&Cancel") );
}

void
FirstRunWizard::invokeHandbook() //SLOT
{
    // Show handbook
    kapp->invokeHelp( QString::null, QString::null, 0 );
}

FirstRunWizard::Interface
FirstRunWizard::interface()
{
    return option_xmms->isChecked() ? XMMS : Compact;
}

void
FirstRunWizard::writeCollectionConfig()
{
    KConfig *config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Folders", m_folderSetup->dirs() );
    config->writeEntry( "Scan Recursively", m_folderSetup->recursive() );
    config->writeEntry( "Monitor Changes", m_folderSetup->monitor() );
}
