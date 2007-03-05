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
    emit( addService ( service ) );
}

#include "scriptableservicemanager.moc"


