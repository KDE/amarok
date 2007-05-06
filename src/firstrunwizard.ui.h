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

#include "amarok.h"
#include "config-amarok.h"
#include "directorylist.h"

#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <QPushButton>
//Added by qt3to4:
#include <QPixmap>
#include <ktoolinvocation.h>

namespace Amarok
{
    extern QPixmap getPNG( const QString& );
    extern QPixmap getJPG( const QString& );

    extern KConfig *config( const QString& );
}

void
FirstRunWizard::init()
{
    using namespace Amarok;

    //aesthetics
    helpButton()->hide();

    picture1->setPixmap( getJPG( "amarok_rocks" ) );
    picture4->setPixmap( *picture1->pixmap() );

    WizardPageLayout_2->addWidget( m_folderSetup = new CollectionSetup( WizardPage_2 ) );

    text4->disconnect( SIGNAL(linkClicked( const QString& )) );
    connect( text4, SIGNAL(linkClicked( const QString& )), SLOT(invokeHandbook()) );
    dbActiveLabel->disconnect( SIGNAL(linkClicked( const QString& )) );
    connect( dbActiveLabel, SIGNAL(linkClicked( const QString& )), SLOT(openLink( const QString &)) );
    setFinishEnabled ( WizardPage_4, true );
#if !defined(USE_MYSQL) && !defined(USE_POSTGRESQL)
        removePage(WizardPage_3);
#endif

}

void
FirstRunWizard::showPage( QWidget *w ) //virtual
{
    Q3Wizard::showPage( w );

    cancelButton()->setText( w == WizardPage ? i18n("&Skip") : i18n("&Cancel") );
}

inline void
FirstRunWizard::invokeHandbook() //SLOT
{
    // Show handbook
    KToolInvocation::invokeHelp( QString(), QString(), 0 );
}

void
FirstRunWizard::writeCollectionConfig()
{
    m_folderSetup->writeConfig();
}

void
FirstRunWizard::openLink(const QString& s)
{
    Amarok::invokeBrowser(s);
}
