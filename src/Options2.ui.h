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
#include "amarokconfig.h"
#include "debug.h"
#include "contextbrowser.h"

#include <kapplication.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <knewstuff/downloaddialog.h> // knewstuff theme fetching
#include <knewstuff/engine.h>         // "
#include <knewstuff/knewstuff.h>      // "
#include <knewstuff/provider.h>       // "
#include <kstandarddirs.h>
#include <ktar.h>
#include <kio/netaccess.h>

#include <qdir.h>
#include <qfileinfo.h>
#include <qtimer.h>


////////////////////////////////////////////////////////////////////////////////
// class AmarokThemeNewStuff
////////////////////////////////////////////////////////////////////////////////

/**
 * GHNS Customised Download implementation.
 */
class AmarokThemeNewStuff : public KNewStuff
{
    public:
    AmarokThemeNewStuff(const QString &type, QWidget *parentWidget=0)
             : KNewStuff( type, parentWidget )
    {}

    bool install( const QString& fileName )
    {
        KTar archive( fileName );

        if ( !archive.open( IO_ReadOnly ) ) {
            KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
            return false;
        }

        const QString destination = Amarok::saveLocation( "themes/" );
        debug() << "copying to " << destination << endl;
        const KArchiveDirectory* archiveDir = archive.directory();
        archiveDir->copyTo( destination, true );

        return true;
    }

    virtual bool createUploadFile( const QString& ) { return false; }
};


////////////////////////////////////////////////////////////////////////////////
// class Options2
////////////////////////////////////////////////////////////////////////////////

void Options2::init()
{
    updateStyleComboBox();
    uninstallPushButton->setEnabled ( styleComboBox->currentText() != "Default" );
}


// This method is basically lifted from ScriptManager::slotInstallScript()
void Options2::installPushButton_clicked()
{
    KFileDialog dia( QString::null, "*.tar *.tar.bz2 *.tar.gz|" + i18n( "Style Packages (*.tar, *.tar.bz2, *.tar.gz)" ), 0, 0, true );
    kapp->setTopWidget( &dia );
    dia.setCaption( kapp->makeStdCaption( i18n( "Select Style Package" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    if ( !dia.exec() ) return;

    KTar archive( dia.selectedURL().path() );

    if ( !archive.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return;
    }

    const QString destination = Amarok::saveLocation( "themes/" );
    debug() << "copying to " << destination << endl;
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo( destination, true );

    updateStyleComboBox();
}



void Options2::retrievePushButton_clicked()
{
    // Delete KNewStuff's configuration entries. These entries reflect which styles
    // are already installed. As we cannot yet keep them in sync after uninstalling
    // styles, we deactivate the check marks entirely.
    Amarok::config()->deleteGroup( "KNewStuffStatus" );

    // we need this because KNewStuffGeneric's install function isn't clever enough
    AmarokThemeNewStuff *kns = new AmarokThemeNewStuff( "amarok/themes", this );
    KNS::Engine *engine = new KNS::Engine( kns, "amarok/theme", this );
    KNS::DownloadDialog* d = new KNS::DownloadDialog( engine, this );
    d->setType( "amarok/theme" );
    // you have to do this by hand when providing your own Engine
    KNS::ProviderLoader *p = new KNS::ProviderLoader( this );
    connect( p, SIGNAL( providersLoaded(Provider::List*) ), d, SLOT( slotProviders(Provider::List *) ) );
    p->load( "amarok/theme", "http://amarok.kde.org/knewstuff/amarokthemes-providers.xml" );

    connect( d, SIGNAL( finished() ), d, SLOT( delayedDestruct() ) );
    connect( d, SIGNAL( finished() ), this, SLOT( updateStyleComboBox() ) );

    // Due to kdelibs idiocy, KNS::DownloadDialog is /always/ non-modal. So we have to
    // ensure that closing the settings dialog before the DownloadDialog doesn't crash.
    QTimer::singleShot( 0, d, SLOT( exec() ) );
}


void Options2::uninstallPushButton_clicked()
{
    const QString name = styleComboBox->currentText();

    if ( name == "Default" )
        return;

    if( KMessageBox::warningContinueCancel( 0,
        i18n( "<p>Are you sure you want to uninstall the theme <strong>%1</strong>?</p>" ).arg( name ),
        i18n("Uninstall Theme"), i18n("Uninstall") ) == KMessageBox::Cancel )
        return;

    if ( name == AmarokConfig::contextBrowserStyleSheet() ) {
        AmarokConfig::setContextBrowserStyleSheet( "Default" );
        ContextBrowser::instance()->reloadStyleSheet();
    }

    KURL themeDir( KURL::fromPathOrURL( Amarok::saveLocation( "themes/" ) ) );
    themeDir.addPath( name );

    if( !KIO::NetAccess::del( themeDir, 0 ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this theme.</p>"
            "<p>You may not have sufficient permissions to delete the folder <strong>%1<strong></p>."
            ).arg( themeDir.isLocalFile() ? themeDir.path() : themeDir.url() ) );
        return;
    }

    updateStyleComboBox();
}


void Options2::styleComboBox_activated(const QString& s)
{
    bool disable = false;
    QDir dir( Amarok::saveLocation( "themes/" ) + s );
    if( !dir.exists() )
        disable = true;

    uninstallPushButton->setEnabled ( !disable );
}


void Options2::updateStyleComboBox()
{
    DEBUG_BLOCK

    styleComboBox->clear();

    const QStringList styleList = kapp->dirs()->findAllResources("data","amarok/themes/*/stylesheet.css", false);
    QStringList sortedList;
    foreach (styleList) sortedList.append(QFileInfo( *it ).dir().dirName());
    sortedList.append( "Default" );
    sortedList.sort();
    foreach(sortedList) styleComboBox->insertItem(*it);

    styleComboBox->setCurrentItem(AmarokConfig::contextBrowserStyleSheet());
}

