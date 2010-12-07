/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#define DEBUG_PREFIX "ServicePluginManager"
 
#include "ServicePluginManager.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "PluginManager.h"
#include "ServiceBase.h"
#include "ServiceBrowser.h"

#include <KService>

ServicePluginManager::ServicePluginManager( QObject *parent )
    : QObject( parent )
    , m_serviceBrowser( ServiceBrowser::instance() )
{
    DEBUG_BLOCK
    setObjectName( "ServicePluginManager" );
}


ServicePluginManager::~ServicePluginManager()
{
}

ServiceBrowser *
ServicePluginManager::browser() const
{
    return m_serviceBrowser;
}

void
ServicePluginManager::setBrowser( ServiceBrowser * browser )
{
    m_serviceBrowser = browser;
}

void
ServicePluginManager::init( const QList<ServiceFactory*> &factories )
{
    DEBUG_BLOCK
    foreach( ServiceFactory* factory, factories )
    {
        if( !factory )
            continue;
        if( factory->isInitialized() )
            continue;

        //check if this service is enabled
        QString pluginName = factory->info().pluginName();
        debug() << "PLUGIN CHECK:" << pluginName;
        if( Amarok::config( "Plugins" ).readEntry( pluginName + "Enabled", true ) )
            initFactory( factory );
    }
}

void
ServicePluginManager::checkEnabledStates( const QList<ServiceFactory*> &factories )
{
    DEBUG_BLOCK
    foreach( ServiceFactory* factory, factories )
    {
        //check if this service is enabled
        const QString pluginName = factory->info().pluginName();
        bool enabledInConfig = Amarok::config( "Plugins" ).readEntry( pluginName + "Enabled", true );
        bool isLastActive = !factory->activeServices().isEmpty();
        if( enabledInConfig == isLastActive )
            continue;

        debug() << "PLUGIN CHECK:" << pluginName << enabledInConfig << isLastActive;
        if( enabledInConfig && !isLastActive )
        {
            initFactory( factory );
        }
        else if( !enabledInConfig && isLastActive )
        {
            foreach( ServiceBase * service, factory->activeServices() )
                m_serviceBrowser->removeCategory( service->name() );
            factory->clearActiveServices();
        }
    }
}

void
ServicePluginManager::initFactory( ServiceFactory *factory )
{
    DEBUG_BLOCK
    debug() << "initializing:" << factory->info().pluginName();
    disconnect( factory, 0, this, 0 );
    connect( factory, SIGNAL(newService(ServiceBase*)), SLOT(slotNewService(ServiceBase*)) );
    connect( factory, SIGNAL(removeService(ServiceBase*)), SLOT(slotRemoveService(ServiceBase*)) );
    factory->init();
}

void
ServicePluginManager::slotNewService( ServiceBase *newService )
{
    DEBUG_BLOCK
    debug() << "new service:" << newService->name();
    m_serviceBrowser->addCategory( newService );
}

void
ServicePluginManager::slotRemoveService( ServiceBase *removedService )
{
    DEBUG_BLOCK
    debug() << "removed service:" << removedService->name();
    m_serviceBrowser->removeCategory( removedService->name() );
}

QStringList
ServicePluginManager::loadedServices() const
{
    QStringList names;
    foreach( ServiceFactory *factory, The::pluginManager()->serviceFactories() )
    {
        foreach( ServiceBase *service, factory->activeServices() )
            names << service->name();
    }
    return names;
}

QStringList
ServicePluginManager::loadedServiceNames() const
{
    return m_serviceBrowser->categories().keys();
}

QString
ServicePluginManager::serviceDescription( const QString & serviceName )
{
    //get named service
    if ( !m_serviceBrowser->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( m_serviceBrowser->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();
    
    return service->shortDescription();
}

QString
ServicePluginManager::serviceMessages( const QString & serviceName )
{
    //get named service
    if ( !m_serviceBrowser->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( m_serviceBrowser->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();

    return service->messages();
}

QString
ServicePluginManager::sendMessage( const QString & serviceName, const QString & message )
{
    //get named service
    if ( !m_serviceBrowser->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( m_serviceBrowser->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();

    return service->sendMessage( message );
}

#include "ServicePluginManager.moc"
