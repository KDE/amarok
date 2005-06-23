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

#include <kapplication.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <knewstuff/downloaddialog.h> // knewstuff script fetching
#include <knewstuff/engine.h>         // "
#include <knewstuff/knewstuff.h>      // "
#include <knewstuff/provider.h>       // "
#include <kstandarddirs.h>
#include <ktar.h>

#include <qdir.h>
#include <qfileinfo.h>


static Options2* s_Options2_instance = 0;

////////////////////////////////////////////////////////////////////////////////
// class AmarokScriptNewStuff
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

        const QString destination = amaroK::saveLocation( "themes/" );
        debug() << "copying to " << destination << endl;
        const KArchiveDirectory* archiveDir = archive.directory();
        archiveDir->copyTo( destination, true );

        s_Options2_instance->styleComboBox->clear();
        return true;
    }

    virtual bool createUploadFile( const QString& ) { return false; }
};


////////////////////////////////////////////////////////////////////////////////
// class Options2
////////////////////////////////////////////////////////////////////////////////

void Options2::init()
{
    s_Options2_instance = this;

    updateStyleComboBox();
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

    const QString destination = amaroK::saveLocation( "themes/" );
    debug() << "copying to " << destination << endl;
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo( destination, true );

    styleComboBox->clear();
    updateStyleComboBox();
}


//function assumes emtpy styleComboBox
void Options2::updateStyleComboBox()
{
    const QStringList styleList = kapp->dirs()->findAllResources("data","amarok/themes/*/stylesheet.css", false);
    QStringList sortedList;
    foreach (styleList) sortedList.append(QFileInfo( *it ).dir().dirName());
    sortedList.append( "Default" );
    sortedList.sort();
    foreach(sortedList) styleComboBox->insertItem(*it);

    styleComboBox->setCurrentItem(AmarokConfig::contextBrowserStyleSheet());
}


void Options2::retrievePushButton_clicked()
{
    // we need this because KNewStuffGeneric's install function isn't clever enough
    AmarokThemeNewStuff *kns = new AmarokThemeNewStuff( "amarok/themes", this );
    KNS::Engine *engine = new KNS::Engine( kns, "amarok/theme", this );
    KNS::DownloadDialog *d = new KNS::DownloadDialog( engine, this );
    d->setType( "amarok/theme" );
    // you have to do this by hand when providing your own Engine
    KNS::ProviderLoader *p = new KNS::ProviderLoader( this );
    QObject::connect( p, SIGNAL( providersLoaded(Provider::List*) ), d, SLOT( slotProviders (Provider::List *) ) );
    p->load( "amarok/theme", "http://amarok.kde.org/knewstuff/amarokthemes-providers.xml" );

    d->exec();

    styleComboBox->clear();
    updateStyleComboBox();
}
