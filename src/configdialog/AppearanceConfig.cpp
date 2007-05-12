/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "AppearanceConfig.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "contextbrowser.h"
#include "debug.h"

#include <KApplication>
#include <KFileDialog>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTar>


AppearanceConfig::AppearanceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

    updateStyleComboBox();
    uninstallPushButton->setEnabled ( styleComboBox->currentText() != "Default" );

    connect( styleComboBox, SIGNAL( activated( int ) ), parent, SLOT( updateButtons() ) );
}

AppearanceConfig::~AppearanceConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
AppearanceConfig::hasChanged()
{
    return styleComboBox->currentText() != AmarokConfig::contextBrowserStyleSheet();
}

bool
AppearanceConfig::isDefault()
{
    return false;
}

void
AppearanceConfig::updateSettings()
{
    if ( styleComboBox->currentText() != AmarokConfig::contextBrowserStyleSheet() ) {
        //can't use kconfigxt for the style comboxbox's since we need the string, not the index
        AmarokConfig::setContextBrowserStyleSheet( styleComboBox->currentText() );
        ContextBrowser::instance()->reloadStyleSheet();
    }
}


///////////////////////////////////////////////////////////////
// SLOTS 
///////////////////////////////////////////////////////////////

// This method is basically lifted from ScriptManager::slotInstallScript()
void
AppearanceConfig::installPushButton_clicked() //SLOT
{
    KFileDialog dia( KUrl(), "*.tar *.tar.bz2 *.tar.gz|" + i18n( "Style Packages (*.tar, *.tar.bz2, *.tar.gz)" ), 0, 0 );
    kapp->setTopWidget( &dia );
    dia.setCaption( KDialog::makeStandardCaption( i18n( "Select Style Package" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    if ( !dia.exec() ) return;

    KTar archive( dia.selectedUrl().path() );

    if ( !archive.open( QIODevice::ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return;
    }

    const QString destination = Amarok::saveLocation( "themes/" );
    debug() << "copying to " << destination << endl;
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo( destination, true );

    updateStyleComboBox();
}

void
AppearanceConfig::retrievePushButton_clicked() //SLOT
{
#if 0 //FIXME: KNS2
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
#endif
}

void
AppearanceConfig::uninstallPushButton_clicked() //SLOT
{
    const QString name = styleComboBox->currentText();

    if ( name == "Default" )
        return;

    if( KMessageBox::warningContinueCancel( 0,
        i18n( "<p>Are you sure you want to uninstall the theme <strong>%1</strong>?</p>", name ),
        i18n( "Uninstall Theme" ) ) == KMessageBox::Cancel )
        return;

    if ( name == AmarokConfig::contextBrowserStyleSheet() ) {
        AmarokConfig::setContextBrowserStyleSheet( "Default" );
        ContextBrowser::instance()->reloadStyleSheet();
    }

    KUrl themeDir( KUrl( Amarok::saveLocation( "themes/" ) ) );
    themeDir.addPath( name );

    if( !KIO::NetAccess::del( themeDir, 0 ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this theme.</p>"
            "<p>You may not have sufficient permissions to delete the folder <strong>%1<strong></p>."
            ).arg( themeDir.isLocalFile() ? themeDir.path() : themeDir.url() ) );
        return;
    }

    updateStyleComboBox();
}

void
AppearanceConfig::styleComboBox_activated(const QString& s) //SLOT
{
    bool disable = false;
    QDir dir( Amarok::saveLocation( "themes/" ) + s );
    if( !dir.exists() )
        disable = true;

    uninstallPushButton->setEnabled ( !disable );
}

void
AppearanceConfig::updateStyleComboBox() //SLOT
{
    DEBUG_BLOCK

    styleComboBox->clear();

    const QStringList styleList = KGlobal::dirs()->findAllResources("data","amarok/themes/*/stylesheet.css", false);
    QStringList sortedList;
    oldForeach (styleList) sortedList.append(QFileInfo( *it ).dir().dirName());
    sortedList.append( "Default" );
    sortedList.sort();
    oldForeach(sortedList) styleComboBox->addItem(*it);

    styleComboBox->setCurrentItem(AmarokConfig::contextBrowserStyleSheet());
}

#include "AppearanceConfig.moc"

