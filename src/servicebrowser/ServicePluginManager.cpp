/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *                2007 Nikolaj Hald Nielsen <nhnFreespirit@gamil.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include "ServicePluginManager.h"
#include "pluginmanager.h"

#include <kservice.h>

ServicePluginManager::ServicePluginManager( ServiceBrowser * browser )
    : QObject()
    , m_serviceBrowser( browser )
{
}


ServicePluginManager::~ServicePluginManager()
{
}

void ServicePluginManager::init()
{

    DEBUG_BLOCK
    KService::List plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'service'" );
    debug() << "Received [" << QString::number( plugins.count() ) << "] collection plugin offers";
    foreach( KService::Ptr service, plugins )
    {
        Amarok::Plugin *plugin = PluginManager::createFromService( service );
        if ( plugin )
        {
            debug() << "Got hold of a valid plugin";
            ServiceFactory* factory = dynamic_cast<ServiceFactory*>( plugin );
            if ( factory )
            {
                debug() << "Got hold of a valid factory";
                connect( factory, SIGNAL( newService( ServiceBase * ) ), this, SLOT( slotNewService( ServiceBase * ) ) );
                factory->init();
            }
            else
            {
                debug() << "Plugin has wrong factory class";
                continue;
            }
        } else {
            debug() << "bad plugin";
            continue;
        }
    }
}

void ServicePluginManager::slotNewService(ServiceBase * newService)
{
    DEBUG_BLOCK
    m_serviceBrowser->addService( newService );
}

#include "ServicePluginManager.moc"


