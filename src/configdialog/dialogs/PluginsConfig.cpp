/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "PluginsConfig"

#include "PluginsConfig.h"

#include "core/support/Debug.h"
#include "services/ServicePluginManager.h"

#include <KPluginSelector>

#include <QVBoxLayout>

PluginsConfig::PluginsConfig( QWidget *parent )
    : ConfigDialogBase( parent )
    , m_configChanged( false )
{
    DEBUG_BLOCK
    m_selector = new KPluginSelector( this );
    m_selector->setSizePolicy( QSizePolicy:: Expanding, QSizePolicy::Expanding );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( m_selector );

    connect( m_selector, SIGNAL(changed(bool)), SLOT(slotConfigChanged(bool)) );
    connect( m_selector, SIGNAL(changed(bool)), parent, SLOT(updateButtons()) );
    connect( m_selector, SIGNAL(configCommitted(QByteArray)), SLOT(slotConfigComitted(QByteArray)) );

    QList<ServiceFactory*> serviceFactories = ServicePluginManager::instance()->factories().values();

    QList<KPluginInfo> pluginInfoList;
    foreach( ServiceFactory *factory, serviceFactories )
        pluginInfoList.append( factory->info() );

    m_selector->addPlugins( pluginInfoList, KPluginSelector::ReadConfigFile, i18n("Internet Services") );
}

PluginsConfig::~PluginsConfig()
{}

void PluginsConfig::updateSettings()
{
    DEBUG_BLOCK
    if( m_configChanged )
    {
        m_selector->save();
        foreach( const QString &name, m_changedPlugins )
            ServicePluginManager::instance()->settingsChanged( name );

        // check if any services were disabled and needs to be removed, or any
        // that are hidden needs to be enabled
        ServicePluginManager::instance()->checkEnabledStates();
    }
}

bool PluginsConfig::hasChanged()
{
    DEBUG_BLOCK
    return m_configChanged;
}

bool PluginsConfig::isDefault()
{
    DEBUG_BLOCK
    return false;
}

void PluginsConfig::slotConfigChanged( bool changed )
{
    DEBUG_BLOCK
    m_configChanged = changed;
}

void PluginsConfig::slotConfigComitted( const QByteArray & name )
{
    DEBUG_BLOCK
    debug() << "config comitted for: " << name;
    m_configChanged = true;
    m_changedPlugins << QString( name );
}

#include "PluginsConfig.moc"
