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
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktar.h>
class KArchiveDirectory;


#include <qdir.h>
#include <qfileinfo.h>

void Options2::init()
{
  updateStyleComboBox();
}


void Options2::installPushButton_clicked()
{//this code is basically a slotInstallScript
    KFileDialog dia( QString::null, "*.tar *.tar.bz2 *.tar.gz|" + i18n( "Script Packages (*.tar, *.tar.bz2, *.tar.gz)" ), 0, 0, true );
    kapp->setTopWidget( &dia );
    dia.setCaption( kapp->makeStdCaption( i18n( "Select Style Package" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    if ( !dia.exec() ) return;

    KTar archive( dia.selectedURL().path() );

    if ( !archive.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return;
    }

    QString destination = amaroK::saveLocation( "themes/" );
    debug() << "copying to " << destination << endl;
    const KArchiveDirectory* archiveDir = archive.directory();

    archiveDir->copyTo( destination, true );
   // KIO::NetAccess::move(destination + archiveDir->entries().first()
    //                ,destination + archiveDir->entries().first().lower());
    styleComboBox->clear();
    updateStyleComboBox();
}

//function assumes emtpy styleComboBox
void Options2::updateStyleComboBox()
{
    QStringList styleList = kapp->dirs()->findAllResources("data","amarok/themes/*/stylesheet.css", false);
    QStringList sortedList;
    foreach (styleList)
        {
            sortedList.append(QFileInfo( *it ).dir().dirName());
        }
    sortedList.sort();
    foreach(sortedList)
      styleComboBox->insertItem(*it);
    styleComboBox->setCurrentItem(AmarokConfig::contextBrowserStyleSheet());
}
