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


