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

ImporterManager::ImporterManager( QObject *parent, const QVariantList &args )
    : ProviderFactory( parent, args )
{
    m_type = Importer;
}

ImporterManager::~ImporterManager()
{
}

void
ImporterManager::init()
{
    m_info = pluginInfo();

    KConfigGroup group = Amarok::config( "Importers" );
    const QStringList providersIds = group.readEntry( id(), QStringList() );

    foreach( const QString &providerUid, providersIds )
    {
        group = Amarok::config( "Importers." + id() + "." + providerUid );

        QVariantMap providerConfig;
        foreach( const QString &key, group.keyList() )
            providerConfig.insert( key, group.readEntry( key, QVariant(QString()) ) );

        if( !providerConfig["uid"].toString().isEmpty() )
        {
            ProviderPtr provider = createProvider( providerConfig );
            m_providers.insert( provider->id(), provider );
        }
    }

    if( Controller *controller = Amarok::Components::statSyncingController() )
        if( Config *config = controller->config() )
            connect( config, SIGNAL(providerForgotten(QString)),
                                                  SLOT(slotProviderForgotten(QString)) );

    m_initialized = true;
}

ProviderConfigWidget*
ImporterManager::createConfigWidget()
{
    return configWidget( QVariantMap() );
}

ProviderPtr
ImporterManager::createProvider( QVariantMap config )
{
    Controller *controller = Amarok::Components::statSyncingController();

    // First, get rid of the old provider instance. Note: the StatSyncing::Config
    // remembers the provider by the id, even when it's unregistered. After this
    // block, old instance should be destroyed, its destructor called.
    if( config.contains( "uid" ) )
    {
        const QString providerId = config["uid"].toString();
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

    connect( provider.data(), SIGNAL(reconfigurationRequested(QVariantMap)),
                                SLOT(createProvider(QVariantMap)), Qt::QueuedConnection);
    m_providers[provider->id()] = provider;

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
    KConfigGroup group = Amarok::config( "Importers" );
    QStringList providersIds = group.readEntry( id(), QStringList() );
    if( !providersIds.toSet().contains( provider->id() ) )
    {
        providersIds.append( provider->id() );
        group.writeEntry( id(), providersIds );
        group.sync();
    }

    group = Amarok::config( "Importers." + id() + "." + provider->id() );
    group.deleteGroup();
    foreach( const QString &key, provider->m_config.keys() )
        group.writeEntry( key, provider->m_config[key] );
    group.sync();

    return provider;
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
    KConfigGroup group = Amarok::config( "Importers" );
    QStringList providersIds = group.readEntry( id(), QStringList() );
    providersIds.removeAll( providerId );
    providersIds.empty() ? group.deleteGroup() : group.writeEntry( id(), providersIds );
    group.sync();

    group = Amarok::config( "Importers." + id() + "." + providerId );
    group.deleteGroup();
    group.sync();
}

} // namespace StatSyncing
