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
    //aesthetics
    cancelButton()->setFixedWidth( cancelButton()->width() );
    helpButton()->hide();

    //would be better as a KConfigXT key now
    if ( amaroK::config( "General" )->readEntry( "XMLFile", QString::null ) != "amarokui.rc" ) {
        option_xmms->setChecked( true );
        picture_modePreview->setPixmap( getPNG( "wizard_xmms" ) );
    }
    else {
        option_compact->setChecked( true );
        picture_modePreview->setPixmap( getPNG( "wizard_compact" ) );
    }
    picture_modePreview->adjustSize();

    picture1->setPixmap( getJPG( "amarok_rocks" ) );
    picture4->setPixmap( *picture1->pixmap() );

    WizardPageLayout_3->addWidget( m_folderSetup = new CollectionSetup( WizardPage_3 ) );

    text4->disconnect( SIGNAL(linkClicked( const QString& )) );
    connect( text4, SIGNAL(linkClicked( const QString& )), SLOT(invokeHandbook()) );

    setFinishEnabled ( WizardPage_4, true );
}

void
FirstRunWizard::showPage( QWidget *w ) //virtual
{
    QWizard::showPage( w );

    cancelButton()->setText( w == WizardPage ? i18n("&Skip") : i18n("&Cancel") );
}

inline void
FirstRunWizard::invokeHandbook() //SLOT
{
    // Show handbook
    kapp->invokeHelp( QString::null, QString::null, 0 );
}

inline void
FirstRunWizard::xmmsModeToggled( bool on ) //SLOT
{
    if ( on )
        picture_modePreview->setPixmap( getPNG( "wizard_xmms" ) );
    else
        picture_modePreview->setPixmap( getPNG( "wizard_compact" ) );
}

FirstRunWizard::Interface
FirstRunWizard::interface()
{
    return option_xmms->isChecked() ? XMMS : Compact;
}

void
FirstRunWizard::writeCollectionConfig()
{
    m_folderSetup->writeConfig();
}
