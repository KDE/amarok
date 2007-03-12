/*
Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#include "debug.h"
#include "scriptableservicemanager.h"
#include <scriptableservicemanageradaptor.h>

#include <kiconloader.h> 
 

ScriptableServiceManager::ScriptableServiceManager(QObject* parent)  
: QObject(parent){

    


    new ScriptableServiceManagerAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/ScriptableServiceManager", this);


}

bool ScriptableServiceManager::createService( QString name, QString listHeader, QString rootHtml) {
    
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
    service->setModel( new ScriptableServiceContentModel( this, listHeader ) ); 
    emit( addService ( service ) );
    
    return true;
}


int ScriptableServiceManager::insertElement( QString name, QString url, QString infoHtml, int parentId, QString serviceName) {

     debug() << "ScriptableServiceManager::insertElement, name: " << name << ", url: "<< url << ", info: " << infoHtml << ", parentId: " << parentId << ", Service name: " << serviceName << endl;

    //get the service
    
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    ScriptableServiceContentModel * model = static_cast< ScriptableServiceContentModel *> ( m_serviceMap[serviceName]->getModel() );
    
    return model->insertItem( name, url, infoHtml, parentId );

}


int ScriptableServiceManager::insertDynamicElement( QString name, QString callbackScript, QString callbackArgument, QString infoHtml, int parentId, QString serviceName){

     debug() << "ScriptableServiceManager::insertDynamicElement, name: " << name << ", callbackScript: "<< callbackScript << ", callbackArgument: "<< callbackArgument <<  ", info: " << infoHtml << ", parentId: " << parentId << ", Service name: " << serviceName << endl;

    //get the service
    
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    ScriptableServiceContentModel * model = static_cast< ScriptableServiceContentModel *> ( m_serviceMap[serviceName]->getModel() );
    
    return model->insertDynamicItem( name, callbackScript, callbackArgument, infoHtml, parentId );

}


bool ScriptableServiceManager::updateComplete( QString serviceName ) {

    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return false;
    }

    ScriptableServiceContentModel * model = static_cast< ScriptableServiceContentModel *> ( m_serviceMap[serviceName]->getModel() );
    model->resetModel();

    return true;

}

#include "scriptableservicemanager.moc"


