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

bool ScriptableServiceManager::createService( QString name, QString listHeader ) {
    
    debug() << "ScriptableServiceManager::CreateService, name: " << name << ", header: "<< listHeader <<  endl;

    if ( m_serviceMap.contains( name ) ) {
        //service name taken
        return false;
    }

    ScriptableService * service = new ScriptableService ( name );
    service->setIcon( KIcon( Amarok::icon( "download" ) ) );
    
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


void ScriptableServiceManager::updateComplete( ) {

// uhm... do stuff

}

#include "scriptableservicemanager.moc"


