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

ScriptsConfig::ScriptsConfig( QWidget *parent )
    : ConfigDialogBase( parent )
    , m_configChanged( false )
{
    DEBUG_BLOCK
    Ui::ScriptsConfig gui;
    gui.setupUi( this );

    // Load config
    gui.kcfg_AutoUpdateScripts->setChecked( AmarokConfig::autoUpdateScripts() );
    gui.manageButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    connect( gui.manageButton, SIGNAL(clicked()), SLOT(slotManageScripts()) );

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
    this->setEnabled( AmarokConfig::enableScripts() );
}

ScriptsConfig::~ScriptsConfig()
{}

void
ScriptsConfig::slotManageScripts()
{
    ScriptManager::instance()->stopScript( m_selector->currentItem() );
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
