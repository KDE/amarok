/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "ScriptsConfig"

#include "ScriptsConfig.h"

#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ScriptSelector.h"
#include "ScriptManager.h"
#include "ui_ScriptsConfig.h"

#include <KArchiveDirectory>
#include <KFileDialog>
#include <KIO/NetAccess>
#include <KMessageBox>
#include <KPluginInfo>
#include <KPluginSelector>
#include <KTar>
#include <KNS3/DownloadDialog>

#include <QVBoxLayout>

ScriptsConfig::ScriptsConfig( QWidget *parent )
    : ConfigDialogBase( parent )
    , m_configChanged( false )
{
    DEBUG_BLOCK
    Ui::ScriptsConfig gui;
    gui.setupUi( this );

    // Load config
    gui.kcfg_AutoUpdateScripts->setChecked( AmarokConfig::autoUpdateScripts() );
    gui.installButton  ->setIcon( KIcon( "folder-amarok" ) );
    gui.retrieveButton ->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    gui.uninstallButton->setIcon( KIcon( "edit-delete-amarok" ) );
    connect( gui.installButton,   SIGNAL(clicked()), SLOT(slotInstallScript()) );
    connect( gui.retrieveButton,  SIGNAL(clicked()), SLOT(slotRetrieveScript()) );
    connect( gui.uninstallButton, SIGNAL(clicked()), SLOT(slotUninstallScript()) );

    m_selector = gui.scriptSelector;

    QString key = QLatin1String( "Generic" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Generic"), key );

    key = QLatin1String( "Lyrics" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Lyrics"), key );

    key = QLatin1String( "Scriptable Service" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Scriptable Service"), key );

    connect( m_selector, SIGNAL(changed(bool)), SLOT(slotConfigChanged(bool)) );
    connect( m_selector, SIGNAL(changed(bool)), parent, SLOT(updateButtons()) );
}

ScriptsConfig::~ScriptsConfig()
{}

bool
ScriptsConfig::slotInstallScript( const QString& path )
{
    DEBUG_BLOCK

    QString _path = path;

    if( path.isNull() )
    {
        _path = KFileDialog::getOpenFileName( KUrl(),
            "*.amarokscript.tar *.amarokscript.tar.bz2 *.amarokscript.tar.gz|"
            + i18n( "Script Packages (*.amarokscript.tar, *.amarokscript.tar.bz2, *.amarokscript.tar.gz)" )
            , this );
        if( _path.isNull() )
            return false;
    }

    KTar archive( _path );
    if( !archive.open( QIODevice::ReadOnly ) )
    {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return false;
    }

    QString destination = Amarok::saveLocation( "scripts/" );
    const KArchiveDirectory* const archiveDir = archive.directory();

    // Prevent installing a script that's already installed
    const QString scriptFolder = destination + archiveDir->entries().first();
    if( QFile::exists( scriptFolder ) )
    {
        KMessageBox::error( 0, i18n( "A script with the name '%1' is already installed. "
                                     "Please uninstall it first.", archiveDir->entries().first() ) );
        return false;
    }

    archiveDir->copyTo( destination );
    bool installSuccess = false;
    installSuccess = recurseInstall( archiveDir, destination );

    if( installSuccess )
    {
        KMessageBox::information( 0, i18n( "<p>Script successfully installed.</p>"
                                           "<p>Please restart Amarok to start the script.</p>" ) );
        return true;
    }
    else
    {
        KMessageBox::sorry( 0, i18n( "<p>Script installation failed.</p>"
                                     "<p>Please inform the package maintainer about this error.</p>" ) );

        // Delete directory recursively
        KIO::NetAccess::del( KUrl( scriptFolder ), 0 );
    }
    return false;
}

bool
ScriptsConfig::recurseInstall( const KArchiveDirectory *archiveDir, const QString &destination )
{
    DEBUG_BLOCK

    const QStringList entries = archiveDir->entries();
    bool success = false;

    foreach( const QString &entry, entries )
    {
        const KArchiveEntry* const archEntry = archiveDir->entry( entry );

        if( archEntry->isDirectory() )
        {
            const KArchiveDirectory* const dir = static_cast<const KArchiveDirectory*>( archEntry );
            success = recurseInstall( dir, destination + entry + '/' );
            continue;
        }
        success = true;
    }
    return success;
}


void
ScriptsConfig::slotRetrieveScript()
{
    KNS3::DownloadDialog dialog("amarok.knsrc", this);
    dialog.exec();

    if (!dialog.installedEntries().isEmpty()) {
        KMessageBox::information( 0, i18n( "<p>Script successfully installed.</p>"
                                            "<p>Please restart Amarok to start the script.</p>" ) );
    } else if (!dialog.changedEntries().isEmpty()) {
        KMessageBox::information( 0, i18n( "<p>Script successfully uninstalled.</p>"
                                            "<p>Please restart Amarok to totally remove the script.</p>" ) );
    }
}

void
ScriptsConfig::slotUninstallScript()
{
    DEBUG_BLOCK
    const QString name = m_selector->currentItem();
    if( name.isEmpty() )
        return;

    if( KMessageBox::warningContinueCancel( this, i18n( "Are you sure you want to uninstall the script '%1'?", name ), i18n("Uninstall Script"), KGuiItem( i18n("Uninstall") ) ) == KMessageBox::Cancel )
        return;

    bool success = ScriptManager::instance()->uninstallScript( name );
    if( success )
    {
        KMessageBox::information( 0, i18n( "<p>Script successfully uninstalled.</p>"
                                           "<p>Please restart Amarok to totally remove the script.</p>" ) );
    }
    else
    {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p>"
                                     "<p>The ScriptManager can only uninstall scripts"
                                     "which have been installed as packages.</p>" ) );
    }
}

void ScriptsConfig::updateSettings()
{
    DEBUG_BLOCK
    if( m_configChanged )
    {
        m_selector->save();
        ScriptManager::instance()->configChanged( true );
    }
}

bool ScriptsConfig::hasChanged()
{
    return m_configChanged;
}

bool ScriptsConfig::isDefault()
{
    return false;
}

void ScriptsConfig::slotConfigChanged( bool changed )
{
    m_configChanged = changed;
    if( changed )
        debug() << "config changed";
}

#include "ScriptsConfig.moc"
