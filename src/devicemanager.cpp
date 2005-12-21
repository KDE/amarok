#include <dcopobject.h>
#include <kapplication.h>
#include <dcopclient.h>
#include "medium.h"
#include <qptrlist.h>
#include "devicemanager.h"
#include "debug.h"

typedef Medium::List MediumList;

DeviceManager* DeviceManager::instance()
{
    static DeviceManager dw;
    return &dw;
}

DeviceManager::DeviceManager()
{
    DEBUG_BLOCK
    m_dc = KApplication::dcopClient();
    m_dc->setNotifications(true);
    m_valid = true;

    if (!m_dc->isRegistered())
    {
        m_valid = false;
        debug() << "DeviceManager:  DCOP Client not registered!" << endl;
    }

    if (!m_dc->connectDCOPSignal("kded", "mediamanager", "mediumAdded(QString)", "devices", "mediumAdded(QString)", false) ||
        !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumRemoved(QString)", "devices", "mediumRemoved(QString)", false) ||
        !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumChanged(QString)", "devices", "mediumChanged(QString)", false))
    {
        debug() << "DeviceManager:  Could not connect to signal mediumAdded!" << endl;
    }
    else
    {
        debug() << "DeviceManager:  connectDCOPSignal returned sucessfully!" << endl;
    }

}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::mediumAdded( QString name )
{
    DEBUG_BLOCK
    Medium* addedMedium;
    if ( m_mediumMap.contains(name) )
        addedMedium = m_mediumMap[name];
    else
        addedMedium = getDevice(name);
    if ( addedMedium != NULL )
        debug() << "[DeviceManager::mediumAdded] Obtained medium name is " << name << ", id is: " << addedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumAdded] Obtained medium is null; name was " << name << endl;
    emit mediumAdded( addedMedium, name, deviceKind(addedMedium) );
}


void DeviceManager::mediumRemoved( QString name )
{
    DEBUG_BLOCK
    Medium* removedMedium = NULL;
    if ( m_mediumMap.contains(name) )
        removedMedium = m_mediumMap[name];
    if ( removedMedium != NULL )
        debug() << "[DeviceManager::mediumRemoved] Obtained medium name is " << name << ", id is: " << removedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumRemoved] Medium was unknown and is null; name was " << name << endl;
    //if you get a null pointer from this signal, it means we did not know about the device
    //before it was removed, i.e. the removal was the first event for the device received while amarok
    //has been running
    //There is no point in calling getDevice, since it will not be in the list anyways
    emit mediumRemoved( removedMedium, name, deviceKind(removedMedium) );
    if ( m_mediumMap.contains(name) )
        m_mediumMap.remove(name);
}


void DeviceManager::mediumChanged( QString name )
{
    DEBUG_BLOCK
    Medium *changedMedium;
    if ( m_mediumMap.contains(name) )
        changedMedium = m_mediumMap[name];
    else
        changedMedium = getDevice(name);
    if ( changedMedium != NULL )
        debug() << "[DeviceManager::mediumChanged] Obtained medium name is " << name << ", id is: " << changedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumChanged] Obtained medium is null; name was " << name << endl;
    emit mediumChanged( changedMedium, name, deviceKind(changedMedium) );
}


/*
BIG FAT WARNING:
Values returned from the below function should not be counted on being unique!
For instance, there may be a Medium object in the QMap that can be accessed through
other functions that has the same data as the Medium object returned, but is a different object.
As you should not be writing to this object, this is okay, however:

Use the Medium's name or id, not the pointer value, for equality comparison!!!

This function does rebuild the map every time it is called, however this should be rare enough
that it is not a problem.
*/
Medium* DeviceManager::getDevice( QString name )
{
    DEBUG_BLOCK
    debug() << "DeviceManager: getDevice called with name argument = " << name << endl;
    Medium* returnedMedium = NULL;
    Medium* tempMedium = NULL;
    MediumList currMediumList;
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    arg << 5;
    if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData))
        debug() << "Error during DCOP call" << endl;
    else
    {
        QDataStream reply(replyData, IO_ReadOnly);
        QStringList result;
        while(!reply.atEnd())
            reply >> result;
        currMediumList = Medium::createList( result );
        Medium::List::iterator it;
        QString mountwhere, halid;
        for ( it = currMediumList.begin(); it != currMediumList.end(); it++ )
        {
            if ( (*it).name() == name )
            {
                debug() << "ID of name argument = " << (*it).id() << endl;
                returnedMedium = new Medium(*it);
            }
            //the following code is currently simply as proof of concept, will likely not be relevant in the end
            if ( (*it).mimeType().contains( "removable", false ) )
            {
                if ( (*it).isMounted() )
                {
                    mountwhere = (*it).mountPoint();
                    debug() << "Removable device " << (*it).id() << " detected, and is mounted at: " << mountwhere << endl;
                }
                else
                    debug() << "Removable device " << (*it).id() << " detected but not mounted" << endl;
            }
            if(m_mediumMap.contains(name))
            {
                tempMedium = m_mediumMap[(*it).name()];
                m_mediumMap.remove((*it).name());
                delete tempMedium;
            }
            m_mediumMap[(*it).name()] = new Medium(*it);
        }
    }
    return returnedMedium;
}

uint DeviceManager::deviceKind(Medium* device)
{
    //detect ipod, ifp, or generic (or other types that get defined in devicemanager.h
    if ( device != NULL)
        return 1;
    else
        return 1;
}


#include "devicemanager.moc"

