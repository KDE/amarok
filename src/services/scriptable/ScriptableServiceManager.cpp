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

#include "ScriptableServiceManager.h"


#include "core-impl/collections/support/MemoryCollection.h"
#include "core/support/Debug.h"
#include "ScriptableServiceCollection.h"
#include "ScriptableServiceMeta.h"
#include "scripting/scriptmanager/ScriptManager.h"
#include "../ServiceMetaBase.h"
#include "browsers/servicebrowser/ServiceBrowser.h"


using namespace Meta;

ScriptableServiceManager * ScriptableServiceManager::s_instance = nullptr;


ScriptableServiceManager::ScriptableServiceManager()
{}


ScriptableServiceManager::~ScriptableServiceManager()
{
    DEBUG_BLOCK
}  


bool ScriptableServiceManager::initService( const QString &name, int levels, const QString &shortDescription,  const QString &rootHtml, bool showSearchBar ) {

    DEBUG_BLOCK

    debug() << "initializing scripted service: " << name;

    ScriptableService * service = new ScriptableService ( name );
    m_serviceMap[name] = service;

    service->setIcon( QIcon::fromTheme( QStringLiteral("view-services-scripted-amarok") ) );
    service->setShortDescription( shortDescription );
    service->init( levels, rootHtml, showSearchBar );
    m_rootHtml = rootHtml;

    debug() << "emitting scripted service " << name;
    Q_EMIT addService( service );

    return true;
}


int ScriptableServiceManager::insertItem( const QString &serviceName, int level, int parentId, const QString &name, const QString &infoHtml, const QString &callbackData, const QString &playableUrl,
                                          const QString & albumOverride, const QString & artistOverride, const QString & genreOverride,
                                          const QString & composerOverride, int yearOverride, const QString &coverUrl)
{
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    return m_serviceMap[serviceName]->insertItem( level, parentId, name, infoHtml, callbackData, playableUrl, albumOverride, artistOverride, genreOverride, composerOverride, yearOverride, coverUrl );

    //return -1; // FIXME: what should this return?
}

void ScriptableServiceManager::setCurrentInfo( const QString &serviceName, const QString & info )
{
    DEBUG_BLOCK
    if ( !m_serviceMap.contains( serviceName ) ) {
    //invalid service name
        return;
    }

    m_serviceMap[serviceName]->setCurrentInfo( info );
}


void ScriptableServiceManager::donePopulating(const QString & serviceName, int parentId)
{
    DEBUG_BLOCK
    debug() << "Service name: " << serviceName << ", parent id: " << parentId;
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return;
    }

    m_serviceMap[serviceName]->donePopulating( parentId );
}

void ScriptableServiceManager::removeRunningScript(const QString & name)
{
    if ( !m_serviceMap.contains( name ) ) {
        debug() << "no such service to remove";
        return;
    }

    //service gets deleted by serviceBrowser
    ServiceBrowser::instance()->removeCategory( m_serviceMap.take( name ) );
}

void ScriptableServiceManager::setIcon( const QString & serviceName, const QPixmap & icon )
{
    DEBUG_BLOCK
            debug() << "service: " << serviceName;
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        debug() << "does not exist.... ";
        return;
    }


    m_serviceMap[serviceName]->setIcon( QIcon( icon ) );
    Q_EMIT( serviceUpdated( m_serviceMap[serviceName] ) );
}

void ScriptableServiceManager::setEmblem( const QString & serviceName, const QPixmap & emblem )
{
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return;
    }
    
    m_serviceMap[serviceName]->setCustomEmblem( emblem );
    Q_EMIT( serviceUpdated( m_serviceMap[serviceName] ) );
}


void ScriptableServiceManager::setScalableEmblem ( const QString& serviceName, const QString& emblemPath )
{
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return;
    }
    
    m_serviceMap[serviceName]->setCustomScalableEmblem( emblemPath );
    Q_EMIT( serviceUpdated( m_serviceMap[serviceName] ) );
}


ScriptableService * ScriptableServiceManager::service(const QString &name)
{
    
    if ( !m_serviceMap.contains( name ) ) {
        return nullptr;
    }

    return m_serviceMap[name];
}


namespace The {
    ScriptableServiceManager*
    scriptableServiceManager()
    {
        if ( ScriptableServiceManager::s_instance == nullptr )
            ScriptableServiceManager::s_instance = new ScriptableServiceManager();

        return ScriptableServiceManager::s_instance;
    }
}












