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

#include <KMessageBox>
#include <KPluginInfo>
#include <KPluginSelector>
#include <KNS3/DownloadDialog>
#include <KPushButton>
#include <KStandardDirs>

#include <QFileSystemWatcher>
#include <QTimer>

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
    connect( ScriptManager::instance(), SIGNAL(scriptsRemoved(QStringList)), SLOT(slotRemoveScripts(QStringList)) );

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

}

void
ScriptsConfig::slotReloadScriptSelector()
{
    // ANM-TODO clear???
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
ScriptsConfig::slotRemoveScripts( const QStringList &scriptNames )
{
    foreach( const QString &scriptName, scriptNames )
    {
        m_selector->removeScript( scriptName );
    }
}

#include "ScriptsConfig.moc"
