/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/ 

#include "debug.h"
#include "scriptableservicemanager.h"
#include <scriptableservicemanageradaptor.h>
#include "collection/support/memorycollection.h"
#include "ScriptableServiceCollection.h"
#include "servicemetabase.h"

#include <kiconloader.h> 
 

ScriptableServiceManager::ScriptableServiceManager(QObject* parent)  
: QObject(parent){


    DEBUG_BLOCK

    new ScriptableServiceManagerAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/ScriptableServiceManager", this);


}

bool ScriptableServiceManager::createService( const QString &name, const QString &listHeader, const QString &rootHtml) {
    
    debug() << "ScriptableServiceManager::CreateService, name: " << name << ", header: "<< listHeader <<  endl;

    if ( m_serviceMap.contains( name ) ) {
        //service name taken
        return false;
    }

    m_rootHtml = rootHtml;
    ScriptableService * service = new ScriptableService ( name );
    service->setIcon( KIcon( Amarok::icon( "download" ) ) );

    service->infoChanged( m_rootHtml );
    
    m_serviceMap[name] = service;


    ScriptableServiceCollection * collection = new ScriptableServiceCollection( name + "_collection" );
    service->setCollection( collection );

     QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
    levels << CategoryId::Album;

    SingleCollectionTreeItemModel * model = new SingleCollectionTreeItemModel( collection, levels );

    service->setModel( model ); 
    emit( addService ( service ) );
    
    return true;
}


int ScriptableServiceManager::insertTrack(const QString &serviceName, const QString &name, const QString &url, const QString &infoHtml, int albumId) {

     debug() << "ScriptableServiceManager::insertElement, name: " << name << ", url: "<< url << ", info: " << infoHtml << ", albumId: " << albumId << ", Service name: " << serviceName << endl;

    //get the service
    
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    int newId = m_serviceMap[serviceName]->addTrack( new ServiceTrack( name ), albumId );  

    //FIXME!!! What should be returned here?
    return newId;

     //QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
   // levels << CategoryId::Album;
    //m_serviceMap[serviceName]->getModel()->setLevels( levels );
}


int ScriptableServiceManager::insertAlbum(const QString & serviceName, const QString & name, const QString & infoHtml/*, int parentId*/)
{

     debug() << "ScriptableServiceManager::insertElement, name: " << name  << ", info: " << infoHtml << /*", parentId: " << parentId <<*/ ", Service name: " << serviceName << endl;

    //get the service
    
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }
    ServiceAlbum * album = new ServiceAlbum( name );
    album->setDescription( infoHtml );
    
    int newId = m_serviceMap[serviceName]->addAlbum( album);

     QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
    //levels << CategoryId::Album;

    //m_serviceMap[serviceName]->getModel()->setLevels( levels );

    return newId;

}


/*int ScriptableServiceManager::insertDynamicElement( const QString &name, const QString &callbackScript, const QString &callbackArgument, const QString &infoHtml, int parentId, const QString &serviceName){

     debug() << "ScriptableServiceManager::insertDynamicElement, name: " << name << ", callbackScript: "<< callbackScript << ", callbackArgument: "<< callbackArgument <<  ", info: " << infoHtml << ", parentId: " << parentId << ", Service name: " << serviceName << endl;

    //get the service
    
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    ScriptableServiceContentModel * model = static_cast< ScriptableServiceContentModel *> ( m_serviceMap[serviceName]->getModel() );
    
    return model->insertDynamicItem( name, callbackScript, callbackArgument, infoHtml, parentId );

}*/


/*bool ScriptableServiceManager::updateComplete( const QString &serviceName ) {

    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return false;
    }

    ScriptableServiceContentModel * model = static_cast< ScriptableServiceContentModel *> ( m_serviceMap[serviceName]->getModel() );
    model->resetModel();

    return true;

}*/



#include "scriptableservicemanager.moc"


