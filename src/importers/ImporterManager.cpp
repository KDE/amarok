/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "ImporterManager.h"

#include "ImporterProvider.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Config.h"
#include "statsyncing/Controller.h"

#include <KConfigGroup>

#include <QStringList>
#include <QVariant>

namespace StatSyncing
{

ImporterManager::ImporterManager()
    : ProviderFactory()
{
}

ImporterManager::~ImporterManager()
{
}

void
ImporterManager::init()
{
    for( const QString &providerId : managerConfig().groupList() )
    {
        KConfigGroup group = providerConfig( providerId );

        QVariantMap config;
        for( const QString &key : group.keyList() )
            config.insert( key, group.readEntry( key, QVariant( QString() ) ) );

        ProviderPtr provider = createProvider( config );
        m_providers.insert( provider->id(), provider );
    }

    if( Controller *controller = Amarok::Components::statSyncingController() )
        if( Config *config = controller->config() )
            connect( config, &Config::providerForgotten,
                     this, &ImporterManager::slotProviderForgotten );

    m_initialized = true;
}

ProviderConfigWidget*
ImporterManager::createConfigWidget()
{
    return configWidget( QVariantMap() );
}

ProviderPtr
ImporterManager::createProvider( const QVariantMap &config )
{
    Controller *controller = Amarok::Components::statSyncingController();

    // First, get rid of the old provider instance. Note: the StatSyncing::Config
    // remembers the provider by the id, even when it's unregistered. After this
    // block, old instance should be destroyed, its destructor called.
    if( config.contains( QStringLiteral("uid") ) )
    {
        const QString providerId = config.value( QStringLiteral("uid") ).toString();
        if( m_providers.contains( providerId ) )
        {
            ProviderPtr oldProvider = m_providers.take( providerId );
            if( controller )
                controller->unregisterProvider( oldProvider );
        }
    }

    // Create a concrete provider using the config. The QueuedConnection in connect()
    // is important, because on reconfigure we *destroy* the old provider instance
    ImporterProviderPtr provider = newInstance( config );
    if( !provider )
    {
        warning() << __PRETTY_FUNCTION__ << "created provider is null!";
        return provider;
    }

    connect( provider.data(), &StatSyncing::ImporterProvider::reconfigurationRequested,
             this, &ImporterManager::createProvider, Qt::QueuedConnection);
    m_providers.insert( provider->id(), provider );

    // Register the provider
    if( controller )
    {
        controller->registerProvider( provider );

        // Set provider to offline
        if( Config *config = controller->config() )
        {
            config->updateProvider( provider->id(), provider->prettyName(),
                                    provider->icon(), /*online*/ false );
            config->save();
        }
    }

    // Save the settings
    KConfigGroup group = providerConfig( provider );
    group.deleteGroup();
    for( const QString &key : provider->m_config.keys() )
        group.writeEntry( key, provider->m_config.value( key ) );
    group.sync();

    return provider;
}

KConfigGroup
ImporterManager::managerConfig() const
{
    return Amarok::config( QStringLiteral("Importers") ).group( type() );
}

KConfigGroup
ImporterManager::providerConfig( const QString &providerId ) const
{
    return managerConfig().group( providerId );
}

KConfigGroup
ImporterManager::providerConfig( const ProviderPtr &provider ) const
{
    return providerConfig( provider->id() );
}

void
ImporterManager::slotProviderForgotten( const QString &providerId )
{
    // Check if the provider is managed by this ImporterManager
    if( !m_providers.contains( providerId ) )
        return;

    ProviderPtr provider = m_providers.take( providerId );
    if( Controller *controller = Amarok::Components::statSyncingController() )
        controller->unregisterProvider( provider );

    // Remove configuration
    KConfigGroup group = providerConfig( providerId );
    group.deleteGroup();
    group.sync();
}

} // namespace StatSyncing
