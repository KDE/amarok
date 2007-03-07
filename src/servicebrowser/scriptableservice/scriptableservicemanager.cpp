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

bool ScriptableServiceManager::createService( QString name ) {
    ServiceBase * service = new ServiceBase ( name );
    service->setIcon( KIcon( Amarok::icon( "download" ) ) );
    
    m_model = new ScriptableServiceContentModel( this, "SomeHeader" );
    service->setModel( m_model ); 
    emit( addService ( service ) );
}


int ScriptableServiceManager::insertElement( QString name, QString url, QString infoHtml, int parentId) {

     debug() << "ScriptableServiceManager::insertElement, name: " << name << ", url: "<< url << ", info: " << infoHtml << ", parentId: " << parentId << endl;

    return m_model->insertItem(name, url, infoHtml, parentId );
}


void ScriptableServiceManager::updateComplete( ) {

// uhm... do stuff

}

#include "scriptableservicemanager.moc"


