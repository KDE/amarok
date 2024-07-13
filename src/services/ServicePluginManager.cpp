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

#include "browsers/servicebrowser/ServiceBrowser.h"
#include "core/support/Debug.h"
#include "services/ServiceBase.h"

#include <QSet>
#include <QCoreApplication>

ServicePluginManager *ServicePluginManager::s_instance = nullptr;

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
        s_instance = nullptr;
    }
}


ServicePluginManager::ServicePluginManager()
    : QObject()
{
    DEBUG_BLOCK
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    setObjectName( QStringLiteral("ServicePluginManager") );
}


ServicePluginManager::~ServicePluginManager()
{
}


void
ServicePluginManager::setFactories( const QList<QSharedPointer<Plugins::PluginFactory> > &factories )
{
    QSet<QSharedPointer<Plugins::PluginFactory> > newFactories(factories.begin(), factories.end());
    QSet<QSharedPointer<Plugins::PluginFactory> > oldFactories(m_factories.begin(), m_factories.end());

    // remove old factories
    for( const auto &pFactory : oldFactories - newFactories )
    {
        auto factory = qobject_cast<ServiceFactory*>( pFactory );
        if( !factory )
            continue;

        for( ServiceBase * service : factory->activeServices() )
            ServiceBrowser::instance()->removeCategory( service );
        factory->clearActiveServices();

        disconnect( factory.data(), &ServiceFactory::newService, this, &ServicePluginManager::slotNewService );
        disconnect( factory.data(), &ServiceFactory::removeService, this, &ServicePluginManager::slotRemoveService );
    }

    // create new factories
    for( const auto &pFactory : newFactories - oldFactories )
    {
        auto factory = qobject_cast<ServiceFactory*>( pFactory );
        if( !factory )
            continue;

        connect( factory.data(), &ServiceFactory::newService, this, &ServicePluginManager::slotNewService );
        connect( factory.data(), &ServiceFactory::removeService, this, &ServicePluginManager::slotRemoveService );
    }

    m_factories = factories;
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
    for( const auto &pFactory : m_factories )
    {
        auto factory = qobject_cast<ServiceFactory*>( pFactory );
        if( !factory )
            continue;

        for( ServiceBase *service : factory->activeServices() )
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

    if ( service == nullptr )
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

    if ( service == nullptr )
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

    if ( service == nullptr )
        return QString();

    return service->sendMessage( message );
}

