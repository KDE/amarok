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
#include "scriptmanager/ScriptManager.h"
#include "ui_ScriptsConfig.h"

#include <KFileDialog>
#include <KMessageBox>
#include <KPluginInfo>
#include <KPluginSelector>
#include <KNS3/DownloadDialog>
#include <KPushButton>
#include <KStandardDirs>
#include <KTar>

#include <QFileSystemWatcher>
#include <QTimer>
#include <QTemporaryFile>

ScriptsConfig::ScriptsConfig( QWidget *parent )
    : ConfigDialogBase( parent )
    , m_configChanged( false )
{
    DEBUG_BLOCK
    Ui::ScriptsConfig gui;
    gui.setupUi( this );

    m_timer = new QTimer(this);
    connect( m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateScripts()) );
    m_timer->setInterval( 200 );

    // Load config
    gui.kcfg_AutoUpdateScripts->setChecked( AmarokConfig::autoUpdateScripts() );
    gui.manageButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    connect( gui.manageButton, SIGNAL(clicked()), SLOT(slotManageScripts()) );
    connect( gui.installButton, SIGNAL(clicked(bool)), SLOT(installLocalScript()) );

    m_selector = gui.scriptSelector;
    slotReloadScriptSelector();

    connect( m_selector, SIGNAL(changed(bool)), SLOT(slotConfigChanged(bool)) );
    connect( m_selector, SIGNAL(changed(bool)), parent, SLOT(updateButtons()) );

    connect( gui.reloadButton, SIGNAL(clicked(bool)), m_timer, SLOT(start()) );

    connect( ScriptManager::instance(), SIGNAL(scriptsChanged()), SLOT(slotReloadScriptSelector()) );

    this->setEnabled( AmarokConfig::enableScripts() );
}

ScriptsConfig::~ScriptsConfig()
{
    ScriptManager::instance()->updateAllScripts();
}

void
ScriptsConfig::slotManageScripts()
{
    QStringList updateScriptsList;
    ScriptManager::instance()->stopScript( m_selector->currentItem() );
    KNS3::DownloadDialog dialog("amarok.knsrc", this);
    dialog.exec();

    QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
    watcher->addPath( KStandardDirs::locate( "data", "amarok/scripts/" ) );
    //connect( watcher, SIGNAL(directoryChanged(QString)), m_timer, SLOT(start()) );

    if( !dialog.installedEntries().isEmpty() || !dialog.changedEntries().isEmpty() )
    {
        // show a dialog?
        // KMessageBox::information( 0, i18n( "<p>Script successfully installed.</p>" ) );
        m_timer->start();
    }
}

void
ScriptsConfig::updateSettings()
{
    DEBUG_BLOCK
    if( m_configChanged )
    {
        m_selector->save();
        ScriptManager::instance()->configChanged( true );
    }
}

bool
ScriptsConfig::hasChanged()
{
    return m_configChanged;
}

bool
ScriptsConfig::isDefault()
{
    return false;
}

void
ScriptsConfig::slotConfigChanged( bool changed )
{
    m_configChanged = changed;
    if( changed )
        debug() << "config changed";
}

void
ScriptsConfig::installLocalScript()
{
    DEBUG_BLOCK
    // where's this config stored anyway, use amarokconfig?
    // the script can actually be updated if the extracted folder's name is the same
    // as archive name on kde download website
    int response =  KMessageBox::warningContinueCancel( this, i18n( "Manually installed scripts "
                                        "cannot be automatically updated, continue?" ), QString(), KStandardGuiItem::cont()
                                        , KStandardGuiItem::cancel(), "manualScriptInstallWarning" );
    if( response == KMessageBox::Cancel )
    {
        return;
    }

    QString filePath = KFileDialog::getOpenFileName( KUrl(), QString(), this, i18n( "Select Archived Script" ) );
    if( filePath.isEmpty() )
        return;

    QString fileName = QFileInfo( filePath ).fileName();
    KTar archive( filePath );
    if( !archive.open( QIODevice::ReadOnly ) )
    {
        KMessageBox::error( this, i18n( "Invalid Archive!" ) );
        return;
    }

    QString destination = KGlobal::dirs()->saveLocation( "data", QString("amarok/scripts/") + fileName + "/"  , false );
    const KArchiveDirectory* const archiveDir = archive.directory();
    const QDir dir( destination );
    const KArchiveFile *specFile = findSpecFile( archiveDir );
    if( !specFile )
    {
        KMessageBox::error( this, i18n( "Invalid Script File!" ) );
        return;
    }

    QTemporaryFile tempFile;
    tempFile.open();
    QIODevice *device = specFile->createDevice();
    tempFile.write( device->readAll() );
    delete device;
    tempFile.close();

    KPluginInfo newScriptInfo( tempFile.fileName() );
    if( !newScriptInfo.isValid() )
    {
        KMessageBox::error( this, i18n( "Invalid Script File!" ) );
        return;
    }

    if( ScriptManager::instance()->m_scripts.contains( newScriptInfo.pluginName() ) )
    {
        QString existingVersion = ScriptManager::instance()->m_scripts[ newScriptInfo.pluginName() ]->info().version();
        QString message = i18n( "Another script with the name %1 already exists\nExisting Script's "
                                "Version: %1\nSelected Script's Version: %2", newScriptInfo.pluginName()
                                , existingVersion, newScriptInfo.version() );
        KMessageBox::error( this, message );
        return;
    }

    int i = 1;
    while( dir.exists( destination ) )
    {
        destination += i;
        ++i;
    }
    dir.mkpath( destination );
    archiveDir->copyTo( destination );
    KMessageBox::information( this, i18n( "The script %1 was successfully installed!", newScriptInfo.name() ) );
    m_timer->start();
}

void
ScriptsConfig::slotReloadScriptSelector()
{
    m_selector->clear();
    QString key = QLatin1String( "Generic" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Generic"), key );

    key = QLatin1String( "Lyrics" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Lyrics"), key );

    key = QLatin1String( "Scriptable Service" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            KPluginSelector::ReadConfigFile, i18n("Scriptable Service"), key );
}

void
ScriptsConfig::slotUpdateScripts()
{
    m_timer->stop();
    ScriptManager::instance()->updateAllScripts();
}

void
ScriptsConfig::slotUninstallScript()
{
    if( !ScriptManager::instance()->m_scripts.contains( m_selector->currentItem() ) )
    {
        KMessageBox::error( this, i18n( "Please select a script!" ) );
        return;
    }

    ScriptItem *item = ScriptManager::instance()->m_scripts.value( m_selector->currentItem() );
    const KPluginInfo selectedScriptInfo = item->info();
}

const KArchiveFile*
ScriptsConfig::findSpecFile( const KArchiveDirectory *dir ) const
{
    foreach( const QString &entry, dir->entries() )
    {
        if( dir->entry( entry )->isFile() )
        {
            if( entry == "script.spec" )
                return static_cast<const KArchiveFile*>( dir->entry( entry ) );
        }
        else
        {
            if( entry != "." && entry != ".." )
            {
                const KArchiveDirectory *subDir = static_cast<const KArchiveDirectory*>( dir->entry( entry ) );
                if( subDir )
                {
                    const KArchiveFile *file = findSpecFile( subDir );
                    if( !file )
                        continue;
                    return file;
                }
            }
        }
    }
    return 0;
}

#include "ScriptsConfig.moc"
