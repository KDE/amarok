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

#include "PluginManager.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "services/ServiceBase.h"

#include <KService>

ServicePluginManager *ServicePluginManager::s_instance = 0;

ServicePluginManager *
ServicePluginManager::instance()
{
    if( !s_instance ) {
        s_instance = new ServicePluginManager();
    }

    return s_instance;
}


void
ServicePluginManager::destroy()
{
    if( s_instance ) {
        delete s_instance;
        s_instance = 0;
    }
}


ServicePluginManager::ServicePluginManager()
    : QObject()
{
    DEBUG_BLOCK
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    setObjectName( "ServicePluginManager" );
}


ServicePluginManager::~ServicePluginManager()
{
}

void
ServicePluginManager::init( const QList<Plugins::PluginFactory*> &factories )
{
    DEBUG_BLOCK
    foreach( Plugins::PluginFactory* pFactory, factories )
    {
        ServiceFactory *factory = qobject_cast<ServiceFactory*>( pFactory );
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
ServicePluginManager::checkEnabledStates( const QList<Plugins::PluginFactory*> &factories )
{
    DEBUG_BLOCK
    foreach( Plugins::PluginFactory* pFactory, factories )
    {
        ServiceFactory *factory = qobject_cast<ServiceFactory*>( pFactory );
        if( !factory )
            continue;

        //check if this service is enabled
        const QString pluginName = factory->info().pluginName();
        bool enabledInConfig = Amarok::config( "Plugins" ).readEntry( pluginName + "Enabled", true );
        bool isLastActive = !factory->activeServices().isEmpty();

        debug() << "PLUGIN CHECK:" << pluginName << enabledInConfig << isLastActive;
        if( enabledInConfig == isLastActive ) // nothing to do
        {
            continue;
        }
        else if( enabledInConfig && !isLastActive ) // add new plugins
        {
            initFactory( factory );
        }
        else if( !enabledInConfig && isLastActive ) // remove old plugins
        {
            foreach( ServiceBase * service, factory->activeServices() )
                ServiceBrowser::instance()->removeCategory( service );
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
    ServiceBrowser::instance()->addCategory( newService );
}

void
ServicePluginManager::slotRemoveService( ServiceBase *removedService )
{
    DEBUG_BLOCK
    debug() << "removed service:" << removedService->name();
    ServiceBrowser::instance()->removeCategory( removedService );
}

QStringList
ServicePluginManager::loadedServices() const
{
    QStringList names;
    foreach( Plugins::PluginFactory *pFactory, The::pluginManager()->factories(QLatin1String("Service")) )
    {
        ServiceFactory *factory = qobject_cast<ServiceFactory*>( pFactory );
        if( !factory )
            continue;

        foreach( ServiceBase *service, factory->activeServices() )
            names << service->name();
    }
    return names;
}

QStringList
ServicePluginManager::loadedServiceNames() const
{
    return ServiceBrowser::instance()->categories().keys();
}

QString
ServicePluginManager::serviceDescription( const QString & serviceName )
{
    //get named service
    if ( !ServiceBrowser::instance()->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( ServiceBrowser::instance()->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();

    return service->shortDescription();
}

QString
ServicePluginManager::serviceMessages( const QString & serviceName )
{
    //get named service
    if ( !ServiceBrowser::instance()->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( ServiceBrowser::instance()->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();

    return service->messages();
}

QString
ServicePluginManager::sendMessage( const QString & serviceName, const QString & message )
{
    //get named service
    if ( !ServiceBrowser::instance()->categories().contains( serviceName ) )
    {
        return i18n( "No service named %1 is currently loaded", serviceName );
    }

    ServiceBase * service = dynamic_cast<ServiceBase *>( ServiceBrowser::instance()->categories().value( serviceName ) );

    if ( service == 0 )
        return QString();

    return service->sendMessage( message );
}

#include "ServicePluginManager.moc"
